/* Derived from cfake.c - implementation of a simple module for a 
 * character device * can be used for testing, demonstrations, etc.
 */

/* ========================================================================
 * Copyright (C) 2012, KEDR development team
 * Copyright (C) 2010-2012, Institute for System Programming 
 *                          of the Russian Academy of Sciences (ISPRAS)
 * Authors: 
 *      Eugene A. Shatokhin <spectre@ispras.ru>
 *      Andrey V. Tsyvarev  <tsyvarev@ispras.ru>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 ======================================================================== */


/* ------------- MERCURY SYSTEMS INC IP COPYRIGHT HEADER  -------------------
 *
 * Copyright:
 *  Copyright (c) 1984-2020,  Mercury Systems, Inc.,
 *  Andover MA.,and all third party embedded software sources.
 *  All rights reserved under the Copyright laws of US. and 
 * international treaties.
 *
 * ------------- MERCURY SYSTEMS INC IP COPYRIGHT HEADER  -------------------*/

/*
 * This material is based upon work supported by the Defense Advanced 
 * Research Projects Agency (DARPA) under Contract No. HR011-19-C-0105. 
 * Any opinions, findings and conclusions or recommendations expressed 
 * in this material are those of the author(s) and do not necessarily 
 * reflect the views of the Defense Advanced Research Projects Agency 
 * (DARPA).
 */

#include <linux/version.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/err.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/spinlock.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/stat.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/seq_file.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(4,11,0)
#include <linux/signal.h>
#else
#include <linux/sched/signal.h>
#endif

#include <linux/uaccess.h>

#include "ilip_base.h"
#include "ilip_nl.h"
#include "ilip_common.h"

MODULE_AUTHOR("Michael Desrochers");
MODULE_LICENSE("GPL");

#define GAPS_ILIP_DEVICE_NAME "gaps_ilip_"

/* parameters */
unsigned int gaps_ilip_messages = GAPS_ILIP_MESSAGE_COUNT;
unsigned int gaps_ilip_ndevices = GAPS_ILIP_NDEVICES;
unsigned int gaps_ilip_total_devices = GAPS_ILIP_TOTAL_DEVICES;
unsigned long gaps_ilip_buffer_size = GAPS_ILIP_BUFFER_SIZE;
unsigned long gaps_ilip_block_size = GAPS_ILIP_BLOCK_SIZE;

#if 0
module_param(gaps_ilip_levels, int, S_IRUGO);
MODULE_PARM_DESC(gaps_ilip_levels, "The total number of levels to be created");
module_param(gaps_ilip_buffer_size, ulong, S_IRUGO);
MODULE_PARM_DESC(gaps_ilip_buffer_size, "Buffer size to use for read and write, must be a multiple of block size");
module_param(gaps_ilip_block_size, ulong, S_IRUGO);
MODULE_PARM_DESC(gaps_ilip_block_size, "Message block size, must be 256 for demonstration");
module_param(gaps_ilip_verbose_level, uint, S_IRUGO);
MODULE_PARM_DESC(gaps_ilip_verbose_level, "ilip driver verbose mode, larger is more verbose, 0 is quiet");
module_param(gaps_ilip_nt_verbose_level, uint, S_IRUGO);
MODULE_PARM_DESC(gaps_ilip_nt_verbose_level, "ilip netlink driver verbose mode, larger is more verbose, 0 is quiet");
#endif

/* ================================================================ */

static unsigned int gaps_ilip_major = 0;

static struct class *gaps_ilip_class = NULL;

static bool gaps_ilip_write_driver_initialized[GAPS_ILIP_CHANNELS] = { false };
extern bool gaps_ilip_read_driver_initialized[GAPS_ILIP_CHANNELS];

static void 
gaps_ilip_cleanup_module
(
  int devices_to_destroy
);

/**
 * Spin lock to protect root device open counting.
 */
static DEFINE_SPINLOCK(root_open_lock);

/**
 * The individual session device have a mutex or a semaphore for mutual exclusion, but 
 * we should have and may need a mutual exclusion at the ILIP DMA context level, or 
 * in the case of the loopback driver as implemented at the 'level' context. 
 */
struct mutex gaps_ilip_context_mutex[GAPS_ILIP_CHANNELS];

/**
 * @brief The number of times the open() has been called, indexed by minor 
 *        number.
 * 
 */
static unsigned int gaps_ilip_open_count[GAPS_ILIP_TOTAL_DEVICES] = { 0 };

extern unsigned int gaps_ilip_ready[GAPS_ILIP_TOTAL_DEVICES];

static char * 
gaps_ilip_devnode
(
  struct device * dev, 
  umode_t * mode
);

/**
 * @brief create the device files
 * 
 * @author mdesroch (2/17/20) 
 *  
 * @note if dev->session_id is not zero this device is created on demand as 
 *       opposed to the startup of the driver. 
 * 
 * @param dev device structure
 * @param minor minor device number to create
 * 
 * @return int 0 on success otherwise and error
 */
static int 
gaps_ilip_construct_device
(
  struct gaps_ilip_dev * dev, 
  int minor, 
  struct class * class
);

static unsigned int 
gaps_ilip_poll
(
  struct file * filp, 
  struct poll_table_struct * wait
);

static int
gaps_ilip_open
(
  struct inode * inode,
  struct file * filp
)
{
  /**
   * if minor node is zero - this is the root node of the driver instance
   * if minor node is even - this is a read device only
   * if minor node is odd  - this is a write device only
   *
   * On device creation we create one root node and pairs of
   * application nodes, one to write into and one to read out of.
   */
  unsigned int mn = iminor(inode);
  unsigned int mj = imajor(inode);
  unsigned int channel;

  struct gaps_ilip_dev * dev = NULL;

  if ((mj != gaps_ilip_major) || (mn < 0) || (mn >= gaps_ilip_total_devices))
  {
    log_warn("No device found with minor=%d, major=%d\n", mj, mn);
    return -ENODEV;
  }
  else
  {
    log_info("Found device with minor=%d, major=%d\n", mn, mj);
  }

  /*
   * Root device can only be opened by one application at a
   * time so as to compute the correct session data.
   */
  spin_lock(&root_open_lock);
  if ((mn == 0) && (gaps_ilip_open_count[mn] >= 1))
  {
    spin_unlock(&root_open_lock);
    log_warn("open: root busy\n");
    return -EBUSY;
  }

  /* Increment the open count on this device */
  gaps_ilip_open_count[mn]++;
  gaps_ilip_ready[mn]++;
  spin_unlock(&root_open_lock);

  /* Store a pointer to struct gaps_ilip_dev here for other methods */
  dev = &gaps_ilip_devices[mn];
  filp->private_data = dev;

  /* Get the channel associated with this minor number */
  channel = gaps_ilip_get_channel_from_minor(dev, mn);

  if (inode->i_cdev != &dev->cdev)
  {
    log_warn("open: internal error\n");
    return -ENODEV;
  }

  if (gaps_ilip_verbose_level > 8)
  {
    log_info("(Minor: %u) open count: %u\n", mn, gaps_ilip_open_count[mn]);
  }

  /* If opened the 1st time, allocate the buffer */
  if (dev->data == NULL)
  {
    dev->data = (unsigned char *)kzalloc(dev->buffer_size, GFP_KERNEL);
    if (dev->data == NULL)
    {
      log_warn("(Minor: %u): out of memory\n", mn);
      return -ENOMEM;
    }
  }

  if (gaps_ilip_verbose_level >= 1)
  {
    log_info("(Minor: %u) Data @ %p Length %lu\n", 
             mn, dev->data, dev->buffer_size);
  }

  /* On a root device open, zero out the buffer, we may have used it before */
  if (mn == 0)
  {
    memset(dev->data, 0, dev->buffer_size);
  }

  if (gaps_ilip_verbose_level > 8)
  {
    log_info("(Minor: %u) open count: %u\n", mn, gaps_ilip_open_count[mn]);
  }

  /* Only initialize the minor device once */
  if (gaps_ilip_open_count[mn] == 1)
  {
    gaps_ilip_devices[mn].mn = mn;

    /*
     * Setup the completion FIFOs based on the channel and access
     * mode for all non-root devices.
     */
    if (mn != 0)
    {
      if ((channel == 0xffffffffu) || (channel >= GAPS_ILIP_CHANNELS))
      {
        log_warn("Invalid channel %u in session %.8x\n",
                 channel, dev->session_id);
        return -EBADF;
      }

      if ((filp->f_flags & O_ACCMODE) == O_WRONLY)
      {
        if (gaps_ilip_verbose_level > 5)
        {
          log_info("(Minor: %u) "
                   "send channel O_WRONLY, channel: %u, Messages: %u\n", 
                   mn, channel, gaps_ilip_messages);
        }

        if (iops.init_write_counters)
        {
          iops.init_write_counters(dev, channel);
        }
      }
      else if ((filp->f_flags & O_ACCMODE) == O_RDONLY)
      {
        if (gaps_ilip_verbose_level > 5)
        {
          log_info("(Minor: %u)"
                   "receive channel O_RDONLY, channel: %u, Messages: %u\n",
                   mn, channel, gaps_ilip_messages);
        }

        if (iops.init_read_counters)
        {
          iops.init_read_counters(dev, channel);
        }
      }
      else
      {
        log_warn("(Minor: %u) open: Invalid access mode\n", mn);
        return -EACCES;
      }

      if (iops.open)
      {
        if (iops.open(dev))
        {
          log_warn("(Minor: %u) Device instance open failed\n", mn);
          return -EBADF;
        }
      }

      /* Indicate the driver is initialized for read or write operation */
      if (channel != 0xffffffffu)
      {
        if ((filp->f_flags & O_ACCMODE) == O_WRONLY)
        {
          gaps_ilip_write_driver_initialized[channel] = true;
        }
        else if ((filp->f_flags & O_ACCMODE) == O_RDONLY)
        {
          if (iops.read_init)
          {
            if (iops.read_init(dev, channel))
            {
              log_warn("(Minor: %u) Device instance read init failed\n", mn);
              return -EBADF;
            }
          }

          /* Driver initialized */
          gaps_ilip_read_driver_initialized[channel] = true;
        }
      }
    }

    if (gaps_ilip_verbose_level > 0)
    {
      log_info("(%u): channel: %x Major: %u - Minor: %u\n",
               gaps_ilip_open_count[mn], channel + 1, mj, mn);
    }
  }
  else
  {
    if (gaps_ilip_verbose_level > 5)
    {
      log_info("(%u): channel: %x, Major: %u - Minor: %u - Already Opened!\n",
               gaps_ilip_open_count[mn], channel + 1, mj, mn);
    }
  }

  return 0;
}

static int
gaps_ilip_release
(
  struct inode * inode,
  struct file * filp
)
{
  struct gaps_ilip_dev * dev = (struct gaps_ilip_dev *)filp->private_data;
  unsigned int mj = imajor(inode);
  unsigned int mn = iminor(inode);
  unsigned int good = 0;
  unsigned int bad = 0;
  unsigned int c_good = 0;
  unsigned int c_bad = 0;
  unsigned int channel;

  channel = gaps_ilip_get_channel_from_minor(dev, mn);

  if (mn == 0)
  {
    spin_lock(&root_open_lock);
  }

  /* Decrement the open count on this device */
  if (gaps_ilip_open_count[mn] > 0)
  {
    gaps_ilip_open_count[mn]--;
    gaps_ilip_ready[mn]--;
  }
  else
  {
    log_warn("(%u): channel: %x Major: %u - Minor: %u open counter error\n",
             gaps_ilip_open_count[mn], channel + 1, mj, mn);
  }

  if (mn == 0)
  {
    spin_unlock(&root_open_lock);
  }

  if (gaps_ilip_verbose_level >= 2)
  {
    log_info("(Minor: %u) Channel: %x OpenCount: %u\n",
             mn, channel, gaps_ilip_open_count[mn]);
  }

  /* Close and reset the device if all users have done the close */
  if (gaps_ilip_open_count[mn] == 0)
  {
    if (mn != 0)
    {
      /* Not root device */
      if ((filp->f_flags & O_ACCMODE) == O_WRONLY)
      {
        gaps_ilip_write_driver_initialized[channel] = false;
        if (iops.get_finalw_statistics)
        {
          iops.get_finalw_statistics(dev,
                                     &good,
                                     &bad,
                                     &c_good,
                                     &c_bad,
                                     channel);
        }
      }
      else if ((filp->f_flags & O_ACCMODE) == O_RDONLY)
      {
        gaps_ilip_read_driver_initialized[channel] = false;
        if (iops.get_finalr_statistics)
        {
          iops.get_finalr_statistics(dev,
                                     &good,
                                     &bad,
                                     &c_good,
                                     &c_bad,
                                     channel);
        }
        if (iops.read_release)
        {
          iops.read_release(dev, channel);
        }
      }
      if (iops.release)
      {
        iops.release(dev, channel);
      }
    }
    else
    {
      /* Root device */
      dev->src_level = 0;
      dev->dst_level = 0;
      dev->source_id = 0;
      dev->destination_id = 0;
      dev->session_message_count = 0;
      dev->message_data_array = NULL;

      if (gaps_ilip_verbose_level >= 2)
      {
        log_info("(Minor: %u) Channel: %x OpenCount: %u\n",
                 mn, channel, gaps_ilip_open_count[mn]);
      }
    }
  }

  /* Dump the stats */
  if (gaps_ilip_open_count[mn] == 0)
  {
    if (channel != 0xffffffffu)
    {
      if (gaps_ilip_verbose_level > 0)
      {
        log_info("(%2u): channel: %.2x Major: %3u - Minor: %3u %s "
                 "[Messages: %7u Rejects: %7u] [Copy %7u Rejects: %7u]\n",
                 gaps_ilip_open_count[mn], channel + 1, mj, mn,
                 ((mn % 2) == 0) ? "read" : "write", 
                 good, bad, c_good, c_bad);
      }
    }
  }

  return 0;
}

static ssize_t
gaps_ilip_read_root
(
  struct file * filp,
  char __user * buf,
  size_t count,
  loff_t * f_pos
)
{
  struct gaps_ilip_dev * dev = (struct gaps_ilip_dev *)filp->private_data;
  ssize_t rv = 0;

  /* Do we need to create a new session */
  bool create_session_device = false;

  /* New minor device number on session create for root device */
  unsigned int new_minor;

  /* New device structure address for root device session create */
  struct gaps_ilip_dev * new_dev = NULL;

  if (gaps_ilip_verbose_level >= 8) 
  {
    log_info("dev: %p Major: %u Minor: %u\n", dev, dev->mj, dev->mn);
  }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,3,0)
  /* Ubuntu 19.10 */
  if (access_ok(buf, count) == 0)
  {
    return -EFAULT;
  }
#else
#if LINUX_VERSION_CODE == KERNEL_VERSION(4,18,0)
  /* RedHat 8.x */
  if (access_ok(buf, count) == 0)
  {
    return -EFAULT;
  }
#else
  /* RedHat 7.x */
  if (access_ok(VERIFY_WRITE, buf, count) == 0)
  {
    return -EFAULT;
  }
#endif
#endif

  if (mutex_lock_killable(&dev->gaps_ilip_mutex))
  {
    log_warn("mutex lock failed\n");
    return -EINTR;
  }

  /* Limit to the block size, you can read less */
  if (count > dev->block_size)
  {
    count = dev->block_size;
  }

  if (gaps_ilip_verbose_level >= 8)
  {
    log_info("(copy_to_user()) Offset: %llx, Buffer length: %lu, "
             "Message Length: %lu, Address: %p\n",
             *f_pos, dev->buffer_size, count, &(dev->data[*f_pos]));
  }

  {
    uint32_t * session_id_adr = NULL;
    uint32_t source_id = 0xffffffff;
    uint32_t destination_id = 0xffffffff;

    if (*f_pos >= dev->buffer_size)
    {
      goto out;
    }

    if (*f_pos + count > dev->buffer_size)
    {
      count = dev->buffer_size - *f_pos;
    }

    if (*f_pos == 0)
    {
      if (count != sizeof(uint32_t))
      {
        rv = -EPERM;
        goto out;
      }

      session_id_adr = (uint32_t *)&(dev->data[*f_pos]);
      source_id = session_id_adr[0];
      destination_id = session_id_adr[2];

      if (gaps_ilip_verbose_level >= 4)
      {
        log_info("Session read: Src: %u Dst: %u MsgCount: %u\n",
                 source_id, destination_id, dev->session_message_count);
      }

      /* Record the session ID associated to the source ID */
      if ((source_id < 0) || (source_id >= GAPS_ILIP_NSESSIONS))
      {
        rv = -EPERM;
        goto out;
      }
      if ((destination_id < 0) || (destination_id >= GAPS_ILIP_NSESSIONS))
      {
        rv = -EPERM;
        goto out;
      }

      /* Are there messahes we have to hash to get the session ID ? */
      if (dev->session_message_count != 0)
      {
        unsigned int hash;
        size_t len;

        /* Compute the session ID */
        len = (2 + dev->session_message_count) * sizeof(uint32_t);
        hash = jenkins_one_at_a_time_hash_init(len, 0xCB2A5AE3);
        /* Skip the level/channel */
        len -= sizeof(uint32_t);
        hash = jenkins_one_at_a_time_hash_ex(hash, 
                 (const uint8_t *)&session_id_adr[2], len);
        hash = jenkins_one_at_a_time_hash_done(hash);
        /* Set the session ID */
        *session_id_adr = hash;
        /* Need to create the session device */
        create_session_device = gaps_ilip_save_session_id(*session_id_adr, 
                                  gaps_ilip_verbose_level);
              
        if (gaps_ilip_verbose_level >= 4)
        {
          log_info("(%.8x): %d {Src: %u, Dst: %u, Channel: %u}\n",
                   *session_id_adr, create_session_device, source_id,
                   destination_id, session_id_adr[1]);
        }
      }
      else
      {
        uint32_t session_index;

        (void)gaps_ilip_save_session_id(*session_id_adr,
                                        gaps_ilip_verbose_level);
        session_index = gaps_ilip_get_session_index(*session_id_adr);
        if (session_index == GAPS_ILIP_NSESSIONS)
        {
          log_warn("(%.8x): Invalid index\n", *session_id_adr);
        }
        if (gaps_ilip_verbose_level >= 2)
        {
          log_info("(%.8x): %d {Src: %u, Dst: %u, Channel: %u} [index: %u]\n",
                   *session_id_adr, false, source_id, destination_id,
                   session_id_adr[1], session_index);
        }
        if (*session_id_adr == 1)
        {
          gaps_ilip_sessions[session_index].level_src = 1;
          gaps_ilip_sessions[session_index].level_dst = 2;
          gaps_ilip_sessions[session_index].minor_src = 1;
          gaps_ilip_sessions[session_index].minor_dst = 2;
        }
        else if (*session_id_adr == 2)
        {
          gaps_ilip_sessions[session_index].level_src = 1;
          gaps_ilip_sessions[session_index].level_dst = 2;
          gaps_ilip_sessions[session_index].minor_src = 4;
          gaps_ilip_sessions[session_index].minor_dst = 3;
        }
        else
        {
          log_warn("(%.8x): Invalid\n", *session_id_adr);
        }
      }
    }
    else
    {
      /* Only allowed to read offset 0 */
      rv = -EPERM;
      goto out;
    }

    /* Need to create the device ? */
    if (create_session_device == true)
    {
      /* Loop index for write and then read device */
      unsigned int session_index;
      int i;

      session_index = gaps_ilip_get_session_index(*session_id_adr);
      if (session_index == GAPS_ILIP_NSESSIONS)
      {
        log_warn("Session ID: %.8x not found\n", *session_id_adr);
        rv = -EIO;
        goto out;
      }

      for (i=0; i < GAPS_ILIP_CHANNELS; i++)
      {
        /* We start the minor number beyond the standard devices */
        new_minor = i + (session_index * 2) + gaps_ilip_ndevices;
        new_dev = &gaps_ilip_devices[new_minor];
        /* Needed here so the device name has the session ID in it */
        new_dev->session_id = *session_id_adr;
        if (gaps_ilip_construct_device(new_dev, new_minor, gaps_ilip_class) != 0)
        {
          log_warn("Create session device [%2u]: %.8x - Failed\n",
                   new_minor, *session_id_adr);
          *session_id_adr = 0xffffffffu;
          break;
        }
        else
        {
          new_dev->src_level = session_id_adr[1];
          new_dev->dst_level = (session_id_adr[1] == 1) ? 2 : 1;
          gaps_ilip_sessions[session_index].level_src = new_dev->src_level;
          gaps_ilip_sessions[session_index].level_dst = new_dev->dst_level;

          if (((new_minor + 1) % 2) == 0)
          {
            gaps_ilip_sessions[session_index].minor_src = new_minor;
          }
          else
          {
            gaps_ilip_sessions[session_index].minor_dst = new_minor;
          }
          if (gaps_ilip_verbose_level >= 4)
          {
            log_info("Create session device [%2u]: Src: %u Dst: %u "
                     "Session: %.8x srcLevel: %u dstLevel: %u\n",
                     new_minor, source_id, destination_id, *session_id_adr,
                     new_dev->src_level, new_dev->dst_level);
          }
        }
      }
      /* Increment the session count if the devices were created ok */
      if (*session_id_adr == 0xffffffffu)
      {
        /* Reset the session ID here */
        if (gaps_ilip_remove_session_index(new_dev->session_id) == true)
        {
          new_dev->session_id = 0;
        }
      }
    }

    /* Will just be the application ID if there are no messages */
    if ((session_id_adr != NULL) && (source_id != 0xffffffffu))
    {
      if (copy_to_user(buf, (const void *)session_id_adr, count) != 0)
      {
        rv = -EFAULT;
        goto out;
      }

      if (gaps_ilip_verbose_level >= 8)
      {
        log_info("(source ID: %u): session ID 0x%x\n", 
                 source_id, *session_id_adr);
      }
    }
 
    if (dev->increment != 0)
    {
      *f_pos += dev->increment;
    }    
  }

  rv = count;

out:
  memset(dev->data, 0, dev->buffer_size);
  mutex_unlock(&dev->gaps_ilip_mutex);

  return rv;
}

static ssize_t
gaps_ilip_read_non_root
(
  struct file * filp,
  char __user * buf,
  size_t count,
  loff_t * f_pos
)
{
  struct gaps_ilip_dev * dev = (struct gaps_ilip_dev *)filp->private_data;
  ssize_t rv = 0;
  unsigned int channel;

  channel = gaps_ilip_get_channel_from_minor(dev, dev->mn);
  if (channel == 0xffffffffu)
  {
    log_warn("Session: %.8x Channel invalid for deviceMinor: %u\n",
             dev->session_id, dev->mn);
    return -EFAULT;
  }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,3,0)
  /* Ubuntu 19.10 */
  if (access_ok(buf, count) == 0)
  {
    return -EFAULT;
  }
#else
#if LINUX_VERSION_CODE == KERNEL_VERSION(4,18,0)
  /* RedHat 8.x */
  if (access_ok(buf, count) == 0)
  {
    return -EFAULT;
  }
#else
  /* RedHat 7.x */
  if (access_ok(VERIFY_WRITE, buf, count) == 0)
  {
    return -EFAULT;
  }
#endif
#endif

  if (mutex_lock_killable(&dev->gaps_ilip_mutex))
  {
    log_warn("mutex lock failed\n");
    if (channel != 0xffffffffu)
    {
      gaps_ilip_message_read_reject_count[channel]++;
    }
    return -EINTR;
  }

  if (!iops.read_data_available)
  {
    log_info("ilip operation: read_data_available is null\n");
    goto out;
  }

  while (iops.read_data_available(dev) == false)
  {
    /* Release the mutex */
    mutex_unlock(&dev->gaps_ilip_mutex);
    if ((filp->f_flags & O_NONBLOCK) != 0)
    {
      return -EAGAIN;
    }
    if (gaps_ilip_verbose_level >= 6)
    {
      log_info("(\"%s\") [Minor: %2d] Reader going to sleep\n",
               current->comm, dev->mn);
    }

    if (wait_event_interruptible(dev->read_desc_wq, 
                                 (iops.read_data_available(dev) == true)))
    {
      return -ERESTARTSYS;
    }
    if (mutex_lock_killable(&dev->gaps_ilip_mutex))
    {
      log_warn("device mutex lock failed\n");
      if (channel != 0xffffffffu)
      {
        gaps_ilip_message_read_reject_count[channel]++;
      }
      return -ERESTARTSYS;
    }
    if (gaps_ilip_verbose_level >= 6)
    {
      log_info("(\"%s\") [Minor: %2d] Reader was woken up\n",
               current->comm, dev->mn);
    }
  }

  if (iops.read)
  {
    rv = iops.read(dev, buf, count, gaps_ilip_verbose_level);
  }
  else
  {
    log_info("ilip operations read not defined\n");
    goto out;
  }

  if (rv < 0)
  {
    if (gaps_ilip_verbose_level >= 6)
    {
      log_info("returned negative status\n");
    }
    goto out;
  }

out:
  mutex_unlock(&dev->gaps_ilip_mutex);

  return rv;
}

static ssize_t
gaps_ilip_read
(
  struct file * filp,
  char __user * buf,
  size_t count,
  loff_t * f_pos
)
{
  struct gaps_ilip_dev * dev = (struct gaps_ilip_dev *)filp->private_data;
  ssize_t rv;

  /* Are we the root device ? */
  if (dev->mn == 0)
  {
    rv = gaps_ilip_read_root(filp, buf, count, f_pos);
  }
  else
  {
    rv = gaps_ilip_read_non_root(filp, buf, count, f_pos);
  }

  return rv;
}

static ssize_t
gaps_ilip_write_root
(
  struct file * filp,
  const char __user * buf,
  size_t count,
  loff_t * f_pos
)
{
  struct gaps_ilip_dev * dev = (struct gaps_ilip_dev *)filp->private_data;
  ssize_t rv;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,3,0)
  /* Ubuntu 19.10 */
  if (access_ok(buf, count) == 0)
  {
    return -EFAULT;
  }
#else
#if LINUX_VERSION_CODE == KERNEL_VERSION(4,18,0)
  /* RedHat 8.x */
  if (access_ok(buf, count) == 0)
  {
    return -EFAULT;
  }
#else
  /* RedHat 7.x */
  if (access_ok(VERIFY_READ, buf, count) == 0)
  {
    return -EFAULT;
  }
#endif
#endif

  if (mutex_lock_killable(&dev->gaps_ilip_mutex))
  {
    log_warn("mutex lock failed\n");
    return -EINTR;
  }

  if (*f_pos >= dev->buffer_size)
  {
    /* Writing beyond the end of the buffer is not allowed */
    rv = -ENOSPC;
    log_warn("Buffer full\n");
    goto out;
  }

  /* Limit count to prevent writing beyond the end of buffer */
  if (*f_pos + count > dev->buffer_size)
  {
    count = dev->buffer_size - *f_pos;
  }

  /* Limit count to the device block size */
  if (count > dev->block_size)
  {
    count = dev->block_size;
  }

  if (gaps_ilip_verbose_level >= 8)
  {
    log_info("(copy_from_user()) Offset: %llu, Buffer length: %lu, "
             "Message Length: %lu, Address: %p\n",
             *f_pos, dev->buffer_size, count, &(dev->data[*f_pos]));
  }

  /* Copy data from the user buffer to the device scratchpad memory */
  if (copy_from_user(&(dev->data[*f_pos]), buf, count) != 0)
  {
    rv = -EFAULT;
    goto out;
  }

  /* Process the specified write based on the file offset */
  if (*f_pos == (loff_t)(0u))
  {
    if (count == sizeof(uint32_t))
    {
      dev->source_id = dev->data[*f_pos];;
      log_info("(source_id): %x\n", dev->source_id);
    }
    else
    {
      rv = -EPERM;
      goto out;
    }
  }
  else if (*f_pos == (loff_t)(4u))
  {
    if (count == sizeof(uint32_t))
    {
      uint32_t channel = dev->data[*f_pos];

      if ((channel < 1) || (channel > GAPS_ILIP_CHANNELS))
      {
        log_warn("(session: %.8x) channel: %u invalid\n", 
                 dev->session_id, channel);

        rv = -EINVAL;
        goto out;
      }
      dev->src_level = channel;

      log_info("(Minor: %u session: %.8x): srcLevel: %u dstLevel: %u\n",
               dev->mn, dev->session_id, dev->src_level, dev->dst_level);
    }
    else
    {
      rv = -EPERM;
      goto out;
    }
  }
  else if (*f_pos == (loff_t)(8u))
  {
    if (count == sizeof(uint32_t))
    {
      dev->destination_id = dev->data[*f_pos];
      log_info("destination_id): %x\n", dev->destination_id);
    }
    else
    {
      rv = -EPERM;
      goto out;
    }
  }
  else if (*f_pos == (loff_t)(12u))
  {
    if ((count > 0) && ((count % sizeof(uint32_t)) == 0))
    {
      uint32_t * msg = (uint32_t *)&(dev->data[*f_pos]);
      int i;

      dev->session_message_count = count / sizeof(uint32_t);
      dev->message_data_array = (uint32_t *)&(dev->data[*f_pos]);

      if (gaps_ilip_verbose_level > 6)
      {
        log_info("(session_messages) Offset: %llu, Buffer length: %lu, "
                 "Message Length: %lu, Address: %p\n",
                 *f_pos, dev->buffer_size, count, &(dev->data[*f_pos]));
        for (i=0; i < dev->session_message_count; i++, msg++)
        {
          if (*msg != 0)
          {
            log_info("(session_messages) Index %u: MessageID: %u\n", i, *msg);
          }
        }
      }
    }
    else
    {
      rv = -EPERM;
      goto out;
    }
  }
  else
  {
    rv = -EPERM;
    goto out;
  }

  rv = count;

out:
  mutex_unlock(&dev->gaps_ilip_mutex);

  return rv;
}

static ssize_t
gaps_ilip_write_non_root
(
  struct file * filp,
  const char __user * buf,
  size_t count,
  loff_t * f_pos
)
{
  struct gaps_ilip_dev * dev = (struct gaps_ilip_dev *)filp->private_data;
  struct gaps_ilip_copy_workqueue * wq = NULL;
  struct ilip_message * msg = NULL;
  smb_t * smb = NULL;
  unsigned int channel;
  ssize_t rv = -EIO;

  /* Get the channel */
  channel = gaps_ilip_get_channel_from_minor(dev, dev->mn);
  if (channel == 0xffffffffu)
  {
    log_warn("Session: %.8x Channel invalid for device "
             "Minor: %u srcLevel: %u dstLevel: %u\n",
             dev->session_id, dev->mn, dev->src_level, dev->dst_level);
    return -EFAULT;
  }

  /* Increment the message write count */
  if (channel != 0xffffffffu)
  {
    gaps_ilip_message_write_count[channel]++;
  }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,3,0)
  /* Ubuntu 19.10 */
  if (access_ok(buf, dev->block_size) == 0)
  {
    return -EFAULT;
  }
#else
#if LINUX_VERSION_CODE == KERNEL_VERSION(4,18,0)
  /* RedHat 8.x */
  if (access_ok(buf, dev->block_size) == 0)
  {
    return -EFAULT;
  }
#else
  /* RedHat 7.x */
  if (access_ok(VERIFY_READ, buf, dev->block_size) == 0)
  {
    return -EFAULT;
  }
#endif
#endif

  if (mutex_lock_killable(&dev->gaps_ilip_mutex))
  {
    log_warn("mutex lock failed\n");
    if (channel != 0xffffffffu)
    {
      gaps_ilip_message_write_reject_count[channel]++;
    }
    return -EINTR;
  }

  if (*f_pos >= dev->buffer_size)
  {
    /* Writing beyond the end of the buffer is not allowed */
    rv = -ENOSPC;
    log_warn("Buffer full\n");
    if (channel != 0xffffffffu)
    {
      gaps_ilip_message_write_reject_count[channel]++;
    }
    goto out;
  }

  if (gaps_ilip_verbose_level >= 8)
  {
    log_info("(copy_from_user()) Offset: %llu, Buffer length: %lu, "
             "Message Length: %lu, Address: %p\n",
             *f_pos, dev->buffer_size, count, &(dev->data[*f_pos]));
  }

  /* Get a local buffer to copy the data into */
  smb = iops.get_buffer(dev, channel);
  if (smb == NULL)
  {
    log_warn("failed to allocate buffer memory\n");
    rv = -ENOMEM;
    goto out;
  }

  /* Copy the data from the user buffer */
  if (copy_from_user(smb->data, buf, dev->block_size) != 0)
  {
    rv = -EFAULT;
    if (channel != 0xffffffffu)
    {
      gaps_ilip_message_write_reject_count[channel]++;
    }
    goto out;
  }

  msg = (struct ilip_message *)smb->data;

  /* Check count is correct */
  if (channel != 0)
  {
    if (count != (dev->block_size + ntohl(msg->u.pay.payload.payload_len)))
    {
      rv = -EINVAL;
      goto out;
    }
  }
  else
  {
    if (count != dev->block_size)
    {
      rv = -EINVAL;
      goto out;
    }
  }

  /* Allocate a workqueue entry */
  wq = kzalloc(sizeof(struct gaps_ilip_copy_workqueue), GFP_KERNEL);
  if (wq == NULL)
  {
    rv = -ENOMEM;
    if (channel != 0xffffffffu)
    {
      gaps_ilip_message_write_reject_count[channel]++;
    }
    goto out;
  }

  /* Fill out the work request */
  wq->start_marker = 0x01234567;
  wq->read_minor = dev->mn + 1;
  wq->src = dev;
  wq->dst = NULL;
  wq->src_handle = smb;
  wq->dst_handle = 0;
  wq->block_size = dev->block_size;
  wq->length = count;
  wq->end_marker = jenkins_one_at_a_time_hash(wq,
    offsetof(struct gaps_ilip_copy_workqueue, length) + sizeof(size_t),
    0x76543210u);

  if (iops.write)
  {
    rv = iops.write(dev, wq, gaps_ilip_verbose_level);
  }

  if (dev->increment != 0)
  {
    *f_pos = (*f_pos + dev->increment) % dev->buffer_size;
  }

out:
  if (wq)
  {
    kfree(wq);
  }
  if (smb)
  {
    iops.free_buffer(dev, smb);
  }
  mutex_unlock(&dev->gaps_ilip_mutex);

  return rv;
}

static ssize_t
gaps_ilip_write
(
  struct file * filp,
  const char __user * buf,
  size_t count,
  loff_t * f_pos
)
{
  struct gaps_ilip_dev * dev = (struct gaps_ilip_dev *)filp->private_data;
  ssize_t rv;

  /* Are we the root device ? */
  if (dev->mn == 0)
  {
    rv = gaps_ilip_write_root(filp, buf, count, f_pos);
  }
  else
  {
    rv = gaps_ilip_write_non_root(filp, buf, count, f_pos);
  }

  return rv;
}

static loff_t 
gaps_ilip_llseek
(
  struct file * filp, 
  loff_t off, 
  int whence
)
{
  struct gaps_ilip_dev * dev = (struct gaps_ilip_dev *)filp->private_data;
  loff_t newpos = 0;

  if (dev->increment == 0)
  {
    return newpos;
  }

  switch (whence)
  {
    case SEEK_SET:
      newpos = off;
      break;

    case SEEK_CUR:
      newpos = filp->f_pos + off;
      break;

    case SEEK_END:
      newpos = dev->buffer_size + off;
      break;

    default: /* can't happen */
      return -EINVAL;
  }
  if ((newpos < 0) || (newpos > dev->buffer_size))
  {
    return -EINVAL;
  }

  filp->f_pos = newpos;

  return newpos;
}

struct file_operations gaps_ilip_fops = 
{
  .owner =    THIS_MODULE,
  .read =     gaps_ilip_read,
  .write =    gaps_ilip_write,
  .open =     gaps_ilip_open,
  .release =  gaps_ilip_release,
  .llseek =   gaps_ilip_llseek,
  .poll   =   gaps_ilip_poll,
};

/* ================================================================ */
/* Setup and register the device with specific index (the index is also
 * the minor number of the device).
 * Device class should be created beforehand.
 */
static int 
gaps_ilip_construct_device
(
  struct gaps_ilip_dev * dev, 
  int minor, 
  struct class * class
)
{
  struct device * device = NULL;
  dev_t devno = MKDEV(gaps_ilip_major, minor);
  char * gaps_ilip_stream = NULL;

  int err = 0;
  int gaps_ilip_mode;
  int gaps_ilip_application;

  BUG_ON((dev == NULL) || (class == NULL));

  /* Memory is to be allocated wen the device is opened the first time */
  dev->data = NULL;
  dev->buffer_size = gaps_ilip_buffer_size;
  dev->block_size = gaps_ilip_block_size;
  dev->session_message_count = 0;
  dev->message_data_array = NULL;
  dev->src_level = 0;
  dev->dst_level = 0;
  /* Note session_id is initialized by caller when needed. */
  dev->source_id = 0;
  dev->destination_id = 0;
  mutex_init(&dev->gaps_ilip_mutex);

  init_waitqueue_head(&dev->write_wq);
  init_waitqueue_head(&dev->read_desc_wq);
  init_waitqueue_head(&dev->read_done_wq);

  dev->mj = gaps_ilip_major;
  dev->mn = (unsigned int)minor;
  if (minor == 0)
  {
    /* Root device does not change the offset on reads and writes */
    dev->increment = 0;
  }
  else
  {
    /* We always move reads and writes by the block size */
    dev->increment = GAPS_ILIP_BLOCK_SIZE;
    if (gaps_ilip_verbose_level > 8)
    {
      log_info("(minor: %d, Increment : %lld): Dev@ %p\n",
             minor, dev->increment, dev);
    }
  }

  cdev_init(&dev->cdev, &gaps_ilip_fops);
  dev->cdev.owner = THIS_MODULE;

  err = cdev_add(&dev->cdev, devno, 1);
  if (err)
  {
    log_warn("Error %d while trying to add %s%d",
           err, GAPS_ILIP_DEVICE_NAME, minor);
    return err;
  }

  if (minor == 0)
  {
    device = device_create(class, NULL, /* no parent device */
                           devno, NULL, /* no additional data */
                           GAPS_ILIP_DEVICE_NAME "%d_root", minor);
  }
  else
  {
    gaps_ilip_application = (minor + 1) / 2;
    gaps_ilip_mode = ((minor + 1) % 2);
    if (gaps_ilip_mode == 0)
    {
      gaps_ilip_stream = "write";
    }
    else
    {
      gaps_ilip_stream = "read";
    }

    /* Simple set of devices for two levels */
    if (dev->session_id == 0)
    {
      /* Standard devices */
      device = device_create(class, NULL, /* no parent device */
                             devno, NULL, /* no additional data */
                             GAPS_ILIP_DEVICE_NAME "%d_%s", 
                             gaps_ilip_application, gaps_ilip_stream);
    }
    else
    {
      /* Session based devices */
      device = device_create(class, NULL, /* no parent device */
                             devno, NULL, /* no additional data */
                             GAPS_ILIP_DEVICE_NAME "s_%.8x_%s", 
                             dev->session_id, gaps_ilip_stream);
      log_info("Session device: %s_s_%.8x_%s\n", GAPS_ILIP_DEVICE_NAME, 
               dev->session_id, gaps_ilip_stream);
    }
  }

  if (IS_ERR(device))
  {
    err = PTR_ERR(device);
    log_warn("Error %d while trying to create %s%d",
           err, GAPS_ILIP_DEVICE_NAME, minor);
    cdev_del(&dev->cdev);
    return err;
  }
  return 0;
}

/* Destroy the device and free its buffer */
static void
gaps_ilip_destroy_device
(
  struct gaps_ilip_dev * dev, 
  int minor, 
  struct class * class
)
{
  BUG_ON((dev == NULL) || (class == NULL));

  device_destroy(class, MKDEV(gaps_ilip_major, minor));
  cdev_del(&dev->cdev);

  kfree(dev->data);
  dev->data = 0;

  mutex_destroy(&dev->gaps_ilip_mutex);

  return;
}

/* ================================================================ */

static void 
gaps_ilip_cleanup_module
(
  int devices_to_destroy
)
{
  int i;

  /* Get rid of character devices (if any exist) */
  if (gaps_ilip_devices)
  {
    for (i = 0; i < devices_to_destroy; ++i)
    {
      if (i < gaps_ilip_ndevices)
      {
        gaps_ilip_destroy_device(&gaps_ilip_devices[i], i, gaps_ilip_class);
      }
      else if (gaps_ilip_devices[i].session_id != 0)
      {
        gaps_ilip_destroy_device(&gaps_ilip_devices[i], i, gaps_ilip_class);
      }
    }
  }

  if (gaps_ilip_class)
  {
    class_destroy(gaps_ilip_class);
  }

  /* [NB] gaps_ilip_cleanup_module is never called if alloc_chrdev_region()
   * has failed. */
  unregister_chrdev_region(MKDEV(gaps_ilip_major, 0), gaps_ilip_total_devices);

  return;
}

static char * 
gaps_ilip_devnode
(
  struct device * dev, 
  umode_t * mode
)
{
  unsigned int mj;
  unsigned int mn;

  if ((!mode) || (!dev))
  {
    return NULL;
  }

  mj = MAJOR(dev->devt);
  mn = MINOR(dev->devt);

  if (mj != gaps_ilip_major)
  {
    return NULL;
  }

  /* Root device need read and write to convert application to session */
  if (dev->devt == MKDEV(gaps_ilip_major, 0))
  {
    /* root device to get session ID */
    *mode = 0666;
  }
  else if ((mn % 2) == 1)
  {
    /* Write device */
    *mode = 0222;
  }
  else if ((mn % 2) == 0)
  {
    /* Read device */
    *mode = 0444;
  }
  return NULL;
}

static int __init
gaps_ilip_init_module
(
  void
)
{
  int err = 0;
  int i = 0;
  int devices_to_destroy = 0;
  dev_t dev = 0;

  if (gaps_ilip_total_devices <= 0)
  {
    log_warn("Invalid value of gaps_ilip_total_devices: %d\n",
           gaps_ilip_total_devices);
    err = -EINVAL;
    return err;
  }

  err = ilip_nl_init();
  if (err < 0)
  {
    log_warn("ilip_nl_init() failed\n");
    return err;
  }
  else if (gaps_ilip_verbose_level >= 2)
  {
    log_info("ilip_nl_init() succeeded\n");
  }

  /* 
   * Get a range of minor numbers (starting with 0) to 
   * work with, include session count 
   */
  err = alloc_chrdev_region(&dev, 
                            0, 
                            gaps_ilip_total_devices, 
                            GAPS_ILIP_DEVICE_NAME);
  if (err < 0)
  {
    log_warn("alloc_chrdev_region() failed\n");
    return err;
  }
  gaps_ilip_major = MAJOR(dev);

  /* Create device class (before allocation of the array of devices) */
  gaps_ilip_class = class_create(THIS_MODULE, GAPS_ILIP_DEVICE_NAME);
  if (IS_ERR(gaps_ilip_class))
  {
    err = PTR_ERR(gaps_ilip_class);
    goto fail;
  }

  /* Routine to setup the correct permissions */
  gaps_ilip_class->devnode = gaps_ilip_devnode;

  /* Init session ID array */
  for (i = 0; i < GAPS_ILIP_NSESSIONS; i++)
  {
    gaps_ilip_sessions[i].session = 0xffffffff;
    gaps_ilip_sessions[i].level_src = 0xffffffff;
    gaps_ilip_sessions[i].level_dst = 0xffffffff;
    gaps_ilip_sessions[i].minor_src = 0xffffffff;
    gaps_ilip_sessions[i].minor_dst = 0xffffffff;
  }
  gaps_ilip_session_count = 0;

  /* 
   * Construct basic devices, session devices are created when 
   * requested in root device access and session ID computation. 
   */
  for (i = 0; i < gaps_ilip_ndevices; ++i)
  {
    err = gaps_ilip_construct_device(&gaps_ilip_devices[i], i, gaps_ilip_class);
    if (err)
    {
      devices_to_destroy = i;
      goto fail;
    }
  }

  if (gaps_ilip_verbose_level > 0)
  {
    log_info("gaps_ilip_verbose_level = %u\n", gaps_ilip_verbose_level);
    log_info("gaps_ilip_nt_verbose_level = %u\n", gaps_ilip_nt_verbose_level);
  }

  return 0; /* success */

fail:
  gaps_ilip_cleanup_module(devices_to_destroy);
  return err;
}

static void __exit
gaps_ilip_exit_module
(
  void
)
{
  ilip_nl_exit();
  gaps_ilip_cleanup_module(gaps_ilip_total_devices);

  return;
}

unsigned int
gaps_ilip_poll
(
  struct file * filp, 
  struct poll_table_struct * wait
)
{
  unsigned mask = 0;
  struct gaps_ilip_dev * dev = (struct gaps_ilip_dev *)filp->private_data;
  unsigned int channel;

  /* Get the channel */
  channel = gaps_ilip_get_channel_from_minor(dev, dev->mn);

  if (mutex_lock_killable(&dev->gaps_ilip_mutex))
  {
    log_warn("device mutex lock failed\n");
    return -EINTR;
  }

  /* Root device can also be read and written at any time */
  if (dev->mn == 0)
  {
    mask  = (POLLIN | POLLRDNORM);
    mask |= (POLLOUT | POLLWRNORM);
    goto quick_return;
  }

  poll_wait(filp, &dev->read_desc_wq, wait);
  poll_wait(filp, &dev->read_done_wq, wait);
  poll_wait(filp, &dev->write_wq, wait);

  /*
   * need to look at the read/write flags in file and 
   * only return the correct mask 
   */
  if ((filp->f_flags & O_ACCMODE) == O_RDONLY)
  {
    /* Read will return if data is available  */
    if (iops.read_data_available(dev) == true)
    {
      if (gaps_ilip_verbose_level >= 8)
      {
        log_info("(Minor: %u, Session: %.8x) read set\n", 
                 dev->mn, dev->session_id);
      }
      mask = (POLLIN | POLLRDNORM);
    }
  }
  else if ((filp->f_flags & O_ACCMODE) == O_WRONLY)
  {
    /* Write will return if we can write, this is always true */
    mask = (POLLOUT | POLLWRNORM);
  }

quick_return:
  mutex_unlock(&dev->gaps_ilip_mutex);
  return mask;
}

module_init(gaps_ilip_init_module);
module_exit(gaps_ilip_exit_module);

/* ================================================================ */
