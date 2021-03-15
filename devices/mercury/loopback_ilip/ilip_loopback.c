/* Derived from cfake.c - implementation of a simple module for a 
 * character device can be used for testing, demonstrations, etc.
 */

/* ========================================================================
 * Copyright (C) 2012, KEDR development team
 * Copyright (C) 2010-2012, Institute for System Programming 
 *                          of the Russian Academy of Sciences (ISPRAS)
 * Authors: 
 *      Eugene A. Shatokhin <spectre@ispras.ru>
 *      Andrey V. Tsyvarev  <tsyvarev@ispras.ru>
 *
 * This program is free software; you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License version 2 as 
 * published by the Free Software Foundation.
 ======================================================================== */


/* ------------- MERCURY SYSTEMS INC IP COPYRIGHT HEADER  -------------------
 *
 * Copyright:
 *  Copyright (c) 1984-2020,  Mercury Systems, Inc.,
 *  Andover MA.,and all third party embedded software sources.
 *  All rights reserved under the Copyright laws of US. 
 *  and international treaties.
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
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/stat.h>
#include <linux/sched.h>

#include <linux/uaccess.h>

#include "ilip_common.h"
#include "ilip_acm.h"
#include "ilip_nl.h"

MODULE_AUTHOR("Michael Desrochers");
MODULE_LICENSE("GPL");

#define GAPS_ILIP_DEVICE_NAME "gaps_ilip_"

/* parameters */
static unsigned int gaps_ilip_channels = GAPS_ILIP_CHANNELS;
static unsigned int gaps_ilip_messages = GAPS_ILIP_MESSAGE_COUNT;
static unsigned int gaps_ilip_total_devices = GAPS_ILIP_TOTAL_DEVICES;
static unsigned long gaps_ilip_buffer_size = GAPS_ILIP_BUFFER_SIZE;
static unsigned long gaps_ilip_block_size = GAPS_ILIP_BLOCK_SIZE;

module_param(gaps_ilip_buffer_size, ulong, S_IRUGO);
MODULE_PARM_DESC(gaps_ilip_buffer_size, "Buffer size to use for read and write, must be a multiple of block size");
module_param(gaps_ilip_block_size, ulong, S_IRUGO);
MODULE_PARM_DESC(gaps_ilip_block_size, "Message block size, must be 256 for demonstration" );
module_param(gaps_ilip_verbose_level, uint, S_IRUGO);
MODULE_PARM_DESC(gaps_ilip_verbose_level, "ilip driver verbose mode, larger is more verbose, 0 is quiet");
module_param(gaps_ilip_nt_verbose_level, uint, S_IRUGO);
MODULE_PARM_DESC(gaps_ilip_nt_verbose_level, "ilip netlink driver verbose mode, larger is more verbose, 0 is quiet");

/* ================================================================ */

struct gaps_ilip_copy_workqueue gaps_ilip_queues[GAPS_ILIP_CHANNELS*GAPS_ILIP_MESSAGE_COUNT];
EXPORT_SYMBOL(gaps_ilip_queues);

struct gaps_ilip_dev *gaps_ilip_devices = NULL;
EXPORT_SYMBOL(gaps_ilip_devices);

/**
 * @brief single threaded workqueues, one read/write for each level 
 * @todo Handoff from write to read.  
 * @details The idea is to have a workqueue representing each ILIP in the system  
 *          as we current understand it. So we have two levels and one ILIP 
 *          sending to a receive ILIP. Later on we will be handling multiple 
 *          levels with in the ILIP but each level has its own DMA queue that 
 *          will be used in a round robin fashion. 
 *  
 * @author mdesroch (2/13/20) 
 */
static struct workqueue_struct *gaps_ilip_wq[GAPS_ILIP_CHANNELS*2] = {NULL}; 


unsigned int gaps_ilip_ready[GAPS_ILIP_TOTAL_DEVICES] = {0}; 
EXPORT_SYMBOL(gaps_ilip_ready); 

uint gaps_ilip_verbose_level = 2;
EXPORT_SYMBOL(gaps_ilip_verbose_level); 

uint gaps_ilip_nt_verbose_level = 2;
EXPORT_SYMBOL(gaps_ilip_nt_verbose_level); 

static uintptr_t gaps_ilip_write_completions_fifo[GAPS_ILIP_CHANNELS][GAPS_ILIP_MESSAGE_COUNT]; 
static uintptr_t  gaps_ilip_read_completions_fifo[GAPS_ILIP_CHANNELS][GAPS_ILIP_MESSAGE_COUNT]; 
static unsigned int gaps_ilip_write_driver_index[GAPS_ILIP_CHANNELS] = {0}; 
static unsigned int gaps_ilip_write_user_index[GAPS_ILIP_CHANNELS] = {0}; 
static unsigned int gaps_ilip_read_driver_index[GAPS_ILIP_CHANNELS] = {0}; 
static unsigned int gaps_ilip_read_user_index[GAPS_ILIP_CHANNELS] = {0}; 

bool gaps_ilip_read_driver_initialized[GAPS_ILIP_CHANNELS] = {false}; 
EXPORT_SYMBOL(gaps_ilip_read_driver_initialized); 

unsigned int gaps_ilip_copy_write_count[GAPS_ILIP_CHANNELS]; 
unsigned int gaps_ilip_copy_write_reject_count[GAPS_ILIP_CHANNELS]; 
unsigned int gaps_ilip_copy_read_count[GAPS_ILIP_CHANNELS]; 
unsigned int gaps_ilip_copy_read_reject_count[GAPS_ILIP_CHANNELS]; 

unsigned int gaps_ilip_message_write_count[GAPS_ILIP_CHANNELS]; 
unsigned int gaps_ilip_message_read_count[GAPS_ILIP_CHANNELS]; 
unsigned int gaps_ilip_message_write_reject_count[GAPS_ILIP_CHANNELS]; 
unsigned int gaps_ilip_message_read_reject_count[GAPS_ILIP_CHANNELS]; 

EXPORT_SYMBOL(gaps_ilip_message_write_count); 
EXPORT_SYMBOL(gaps_ilip_message_read_count); 
EXPORT_SYMBOL(gaps_ilip_message_write_reject_count); 
EXPORT_SYMBOL(gaps_ilip_message_read_reject_count); 


smb_t *
ilip_loopback_alloc_buffer
(
  struct gaps_ilip_dev * gaps_dev,
  unsigned int length
);

smb_t * 
ilip_loopback_get_buffer
(
  struct gaps_ilip_dev * gaps_dev,
  unsigned int channel
);

void 
ilip_loopback_free_buffer
(
  struct gaps_ilip_dev * gaps_dev,
  smb_t * handle
);

bool 
ilip_loopback_read_data_available
(
  struct gaps_ilip_dev * gaps_dev
);

int 
ilip_loopback_write
(
  struct gaps_ilip_dev * gaps_dev,
  struct gaps_ilip_copy_workqueue * wq, 
  unsigned int verbosity
);

int 
ilip_loopback_read_init
(
  struct gaps_ilip_dev * dev, 
  unsigned int channel
);

int 
ilip_loopback_init_write_counters
(
  struct gaps_ilip_dev * gaps_dev,
  unsigned int channel
);

int 
ilip_loopback_init_read_counters
(
  struct gaps_ilip_dev * gaps_dev,
  unsigned int channel
);

int 
ilip_loopback_clear_counters
(
  unsigned int mn,
  unsigned int direction
);

int 
ilip_loopback_read
(
  struct gaps_ilip_dev * gaps_dev,
  char __user * buf,
  size_t count,
  unsigned int verbosity
);

int 
ilip_loopback_get_finalr_statistics
(
  struct gaps_ilip_dev * gaps_dev,
  unsigned int * good, 
  unsigned int * bad, 
  unsigned int * c_good,
  unsigned int * c_bad,
  unsigned int channel
);

int 
ilip_loopback_get_finalw_statistics
(
  struct gaps_ilip_dev * gaps_dev,
  unsigned int * good, 
  unsigned int * bad, 
  unsigned int * c_good,
  unsigned int * c_bad,
  unsigned int channel
);

int 
ilip_loopback_get_statistics
(
  uint32_t mn,
  uint32_t session_id,
  uint32_t session_index,
  uint32_t direction,
  struct ilip_session_statistics * stat
);

struct ilip_operations iops = 
{
  .check_status          = 0,
  .open                  = 0,
  .alloc_buffer          = ilip_loopback_alloc_buffer,
  .get_buffer            = ilip_loopback_get_buffer,
  .free_buffer           = ilip_loopback_free_buffer,
  .get_finalr_statistics = ilip_loopback_get_finalr_statistics,
  .get_finalw_statistics = ilip_loopback_get_finalw_statistics,
  .get_statistics        = ilip_loopback_get_statistics,
  .init_write_counters   = ilip_loopback_init_write_counters,
  .init_read_counters    = ilip_loopback_init_read_counters,
  .clear_counters        = ilip_loopback_clear_counters,
  .write                 = ilip_loopback_write,
  .read                  = ilip_loopback_read,
  .read_init             = ilip_loopback_read_init,
  .read_data_available   = ilip_loopback_read_data_available,
  .read_release          = 0,
  .release               = 0
};
EXPORT_SYMBOL(iops);

/*
 * @brief offset location the copy from write to read process will be placing  
 *        the data. 
 *   
 * @details The device read only looks at the completion queue FIFO to know if  
 *          there is any data to be read. The ILIP copy process is completely 
 *          responsible for the writing and update for the buffers, the offset 
 *          into the buffers and the posting to the read message FIFO data 
 *          available. 
 *  
 */
static loff_t gaps_ilip_copy_read_offset[GAPS_ILIP_CHANNELS]; 

/* ================================================================ */ 

static void gaps_ilip_copy(struct work_struct * work); 

/* 
 * @brief static declaration used in testing only 
 *  
 * @author mdesroch (2/15/20) 
 */
static struct gaps_ilip_copy_workqueue gaps_ilip_local_workqueue;
static DECLARE_WORK(gaps_ilip_workqueue, gaps_ilip_copy);

/*
 * @brief the access control matrix associated to the write of the data by the
 *        hardware ILIP.
 *   
 * @details This routine determines if the sender is allowed to write the 
 *          message  to the wire. It looks at the session ID, the message 
 *          and data tags to see if a write is allowed by this ILIP hardware 
 *          entity. 
 *  
 * @author mdesroch (2/15/20) 
 *  
 * @param cp Driver details of the message to be copied 
 * @param msg Message details of the data to be copied. 
 *  
 * @return bool True if we are allowed to write the data, False if we are not. 
 */ 
static bool 
gaps_ilip_access_write
(
  uint32_t session,
  uint32_t message,
  uint32_t desc,
  uint32_t data,
  uint32_t * pindex
); 

/* 
 * @brief the access control matrix associated to the read of the data by the  
 *        hardware ILIP 
 *   
 * @details This routine determines if the receiving ILIP is allowed to process
 *          the message coming in from the wire. It looks at the session ID, 
 *          the message and data tags to see if a read is allowed by this ILIP 
 *          hardware entity. 
 *  
 * @author mdesroch (2/15/20) 
 *
 * @param cp Driver details of the message to be copied 
 * @param msg Message details of the data to be copied. 
 *
 * @return bool True if we are allowed to read the data, False if we are not. 
 */
static bool 
gaps_ilip_access_read
(
  uint32_t session,
  uint32_t message,
  uint32_t desc,
  uint32_t data
); 

static void
gaps_ilip_redact
(
  void * p,
  uint32_t index
);

/** 
 * @todo Need a way to send a message that will be rejected on the read side as 
 *       well as the write side. 
 *  
 */ 
static bool
gaps_ilip_access_read
(
  uint32_t session,
  uint32_t message,
  uint32_t desc,
  uint32_t data
)
{
  int i, j;

  for (i=0; i < ILIP_NUM_ACM_ENTRIES; i++)
  {
    if (ntohl(ilip_acm_src[i].valid) == 0x00000001)
    {
      if ((ilip_acm_dst[i].session == session) &&
          (ilip_acm_dst[i].message == message) &&
          (ilip_acm_dst[i].data == data))
      {
        for (j=0; j < ILIP_NUM_ACM_ENTRIES; j++)
        {
          if (ntohl(ilip_acm_src[j].valid) == 0x00000001)
          {
            if ((ilip_acm_dst[j].session == session) &&
                (ilip_acm_dst[j].message == message) &&
                (ilip_acm_dst[j].data == desc))
            {
              return true;
            }
          }
        }
      }
    }
  }

  return false;
}

static bool
gaps_ilip_access_write
(
  uint32_t session,
  uint32_t message,
  uint32_t desc,
  uint32_t data,
  uint32_t * pindex
)
{
  uint32_t i, j;

  *pindex = 0xffffffffu;

  /* First find the redactor index */
  for (i=0; i < ILIP_NUM_ACM_ENTRIES; i++)
  {
    if (ntohl(ilip_acm_src[i].valid) == 0x00000001)
    {
      if ((ilip_acm_src[i].session == session) &&
          (ilip_acm_src[i].message == message) &&
          (ilip_acm_src[i].data == data))
      {
        *pindex = i;
      }
    }
  }

  if (*pindex == 0xffffffffu)
  {
    return false;
  }

  /* Next find the go/no-go entry */
  for (i=0; i < ILIP_NUM_ACM_ENTRIES; i++)
  {
    if (ntohl(ilip_acm_src[i].valid) == 0x00000001)
    {
      if ((ilip_acm_src[i].session == session) &&
          (ilip_acm_src[i].message == message) &&
          (ilip_acm_src[i].data == data))
      {
        for (j=0; j < ILIP_NUM_ACM_ENTRIES; j++)
        {
          if (ntohl(ilip_acm_src[j].valid) == 0x00000001)
          {
            if ((ilip_acm_src[j].session == session) &&
                (ilip_acm_src[j].message == message) &&
                (ilip_acm_src[j].data == desc))
            {
              return true;
            }
          }
        }
      }
    }
  }

  return false;
}

static void
gaps_ilip_redact
(
  void * p,
  uint32_t index
)
{
  struct ilip_message * msg = (struct ilip_message *)p;
  struct oft_entry * oft = &oft_table[index];
  uint32_t mask_lo;
  uint32_t mask_hi;
  uint32_t off_lo;
  uint32_t off_hi;
  uint32_t len;
  int rldispatch;
  int chan;
  int rem;

  /* Determine the channel */
  chan = (msg->u.imm.header.desc_type & 0xf0000000) >> 28;

  /* Calculate the valid offset range */
  switch (chan)
  {
    case 0:
      off_lo = 0x02;
      off_hi = 0x0f;
      mask_lo = 0xff00;
      mask_hi = 0x00ff;
      break;
    case 1:
    case 2:
    case 3:
      /* Get the payload length */
      len = ntohl(msg->u.pay.payload.payload_len);

      /* Calculate the 16-byte word offsets */
      off_lo = 0x00;
      off_hi = (len / 16) - ((len % 16) ? 0 : 1);

      /* Calculate the allowable byte masks */
      rem = len % 16;

      /* Build the low mask */
      if (len >= 16)
      {
        mask_lo = 0xffff;
      }
      else
      {
        mask_lo = (1 << (15 - rem)) - 1;
      }

      /* Build the high mask */
      if (len < 16)
      {
        mask_hi = 0;
      }
      else
      {
        mask_hi = (1 << rem) - 1;
      }
      break;
    default:
      log_warn("Invalid channel number %d\n", chan);
      return;
  }

  /* Check the offload function table for the redactor */
  if (oft->offload[0] & 0x8000)
  {
    rldispatch = oft->offload[0] & 0x7ff;

    /* Check redactor list table for valid entry */
    while (ilip_rlt[rldispatch].mask & 0x80000000)
    {
      uint32_t mask = ilip_rlt[rldispatch].mask & 0x7fffffff;
      uint32_t off = ilip_rlt[rldispatch].offset;

      /* Check the offsets and masks */
      if ((off > off_hi) || (off < off_lo))
      {
        log_info("Invalid redactor offset 0x%x\n", off);
      }
      else
      {
        if ((off == off_hi) && (mask & ~mask_hi))
        {
          log_info("Invalid redactor mask/offset combination\n");
          log_info("  adjusting mask\n");

          mask &= mask_hi;
        }
        else if ((off == off_lo) && (mask & ~mask_lo))
        {
          log_info("Invalid redactor mask/offset combination\n");
          log_info("  adjusting mask\n");

          mask &= mask_lo;
        }

        if (chan == 0)
        {
          uint8_t * p = (uint8_t *)msg;
          int i;

          /* Redact immediate message */
          for (i=15; i >= 0; i--)
          {
            if (mask & (1 << i))
            {
              *(p + (off << 4) + (15 - i)) = 0;
            }
          }
        }
        else
        {
          uint8_t * p = (uint8_t *)msg + sizeof(struct ilip_message);
          int i;

          /* Redact payload message */
          for (i=0; i < 16; i++)
          {
            if (mask & (1 << i))
            {
              *(p + (off << 4) + (15 - i)) = 0;
            }
          }
        }
      }

      rldispatch++;
    }
  }

  return;
}

static void 
gaps_ilip_copy
(
  struct work_struct * work
) 
{ 
  struct gaps_ilip_copy_workqueue * cp = NULL; 
  struct ilip_message * msg = NULL; 
  uint32_t end_marker; 
  uint32_t channel = 0xffffffffu; 
  uint32_t write_driver_index; 
  uint32_t write_driver_index_saved; 
  uint32_t read_driver_index; 
  uint32_t read_driver_index_save; 
  uint32_t index;
  bool do_copy_first_time = false; 
  u64 t; 

  /* Cannot be NULL */ 
  if (work == NULL)
  { 
    log_warn("work pointer invalid\n");
    return; 
  } 

  /* device time we start processing */ 
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,3,0) 
  /* Ubuntu 19.10 API change */ 
  t = ktime_get_boottime_ns(); 
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(4,19,0)
  ktime_get_boottime_ts64(&ts64);
  t = ts64.tv_sec * 1000000000 + ts64.tv_nsec;
#else
  /* RedHat 7.x API */ 
  t = ktime_get_boot_ns(); 
#endif 

  /* Get the work queue item we need to process */ 
  if (gaps_ilip_verbose_level > 8)
  { 
    log_info("(%p)\n", work);
  } 

  cp = container_of(work, struct gaps_ilip_copy_workqueue, workqueue); 
  if (gaps_ilip_verbose_level > 8)
  { 
    log_info("%p : EndMarker: %x)\n", cp, cp->end_marker);
  } 

  if (cp->src == NULL)
  { 
    log_warn("(Copy: %p) Source %p is invalid\n", cp, cp->src);
    return; 
  } 

  channel = gaps_ilip_get_channel_from_minor(cp->src, cp->src->mn); 
  if (channel == 0xffffffffu)
  { 
    log_warn("(%p : %x) channel invalid\n", cp, cp->src->mn);
    return; 
  }
  else
  { 
    if (channel >= (GAPS_ILIP_CHANNELS))
    { 
      log_warn("(channel index: %u) channel invalid\n", channel);
      return; 
    } 

    /* Increment the copy write count */ 
    gaps_ilip_copy_write_count[channel]++; 

    /* Post the address to the write completion FIFO */ 
    if (gaps_ilip_write_driver_index[channel] == gaps_ilip_messages)
    { 
      write_driver_index = 0; 
      /* Allow the write as this is the first time through */ 
      do_copy_first_time = true; 
    }
    else
    { 
      write_driver_index_saved = gaps_ilip_write_driver_index[channel]; 
      wmb(); /* Not sure about this */ 
      write_driver_index = (gaps_ilip_write_driver_index[channel] + 1)%gaps_ilip_messages; 
      wmb(); 
    } 
    if (write_driver_index != gaps_ilip_write_user_index[channel] || do_copy_first_time == true)
    { 
      /* There is room in the FIFO either first time of FIFO not full */ 
      do_copy_first_time = false; 
      gaps_ilip_write_completions_fifo[channel][write_driver_index] = (uintptr_t)((cp->src_handle)->data); 
      if (gaps_ilip_verbose_level >= 5)
      { 
        log_info("(FIFO (User: %2u - Driver: %2u)): %lx "
                 "write completion posted\n",
                 gaps_ilip_write_user_index[channel], 
                 gaps_ilip_write_driver_index[channel], 
                 gaps_ilip_write_completions_fifo[channel][write_driver_index]); 
      } 
      wmb(); 
      gaps_ilip_write_driver_index[channel] = write_driver_index; 
      wmb(); 
    }
    else
    { 
      /* reset the index on the failure to place completion in FIFO */ 
      wmb(); 
      gaps_ilip_write_driver_index[channel] = write_driver_index_saved; 
      wmb(); 
      log_warn("(%p) write completion FIFO overflow\n", cp);
      gaps_ilip_copy_write_reject_count[channel]++; 
      return; 
    } 
  } 

  /* Start marker has to be correct */ 
  if (cp->start_marker != 0x01234567)
  { 
    log_warn("(%p : %x) start marker invalid\n", cp, cp->start_marker);
    gaps_ilip_copy_write_reject_count[channel]++; 
    return; 
  } 

  /* End marker has to be correct */ 
  if ((end_marker=jenkins_one_at_a_time_hash(cp, offsetof(struct gaps_ilip_copy_workqueue, length)+sizeof(size_t), 0x76543210u )) != cp->end_marker)
  {
    log_warn("(%p : %x): %x end marker invalid\n", 
             cp, cp->end_marker, end_marker);
    if (gaps_ilip_verbose_level > 4)
    { 
      printk(KERN_INFO "Copy: %p (channel: %u)\n", cp, channel);
      printk(KERN_INFO "      start_marker: %.8x ilip\n", cp->start_marker); 
      printk(KERN_INFO "             minor: %d ilip\n", cp->read_minor); 
      printk(KERN_INFO "               src: %p ilip\n", cp->src); 
      printk(KERN_INFO "               dst: %p ilip\n", cp->dst); 
      printk(KERN_INFO "        src_handle: %p ilip\n", (cp->src_handle)->data);
      printk(KERN_INFO "        dst_handle: %p ilip\n", (cp->dst_handle)->data);
      printk(KERN_INFO "        block_size: %lu ilip\n", cp->block_size);
      printk(KERN_INFO "            length: %lu ilip\n", cp->length);
    } 
    gaps_ilip_copy_write_reject_count[channel]++; 
    return; 
  } 

  msg = (struct ilip_message *)((cp->src_handle)->data); 
  if (gaps_ilip_verbose_level >= 4)
  {
    printk(KERN_INFO "gaps_ilip_copy() Message@ %p ilip\n", msg); 
    printk(KERN_INFO "      session: %.8x ilip\n", 
           ntohl(msg->u.imm.header.session_tag));
    printk(KERN_INFO "      message: %u ilip\n", 
           ntohl(msg->u.imm.header.message_tag));
    printk(KERN_INFO "    desc_type: %u ilip\n", 
           ntohl(msg->u.imm.header.desc_type));
    printk(KERN_INFO "     desc_tag: %x ilip\n", 
           ntohl(msg->u.imm.header.data_tag));
    printk(KERN_INFO "    ilip_time: %lx ilip microseconds\n", 
           (long unsigned int)msg->u.imm.time.ilip_time);
    printk(KERN_INFO "   linux_time: %lx ilip\n", 
           (long unsigned int)msg->u.imm.time.linux_time);
    printk(KERN_INFO "       sizeof: %lx (long unsigned int) ilip\n", 
           sizeof(long unsigned int));
    printk(KERN_INFO "  data_length: %u (%u) ilip\n", 
           msg->u.imm.payload.imm_data_len, 
           ntohl(msg->u.imm.payload.imm_data_len));
  } 

  /* Verify the send side can process the message */ 
  if (channel == 0)
  {
    if (gaps_ilip_access_write(msg->u.imm.header.session_tag,
                               msg->u.imm.header.message_tag,
                               msg->u.imm.header.data_tag,
                               msg->u.imm.header.data_tag,
                               &index) == false)
    { 
      /* Not allowed write access, so drop the packet */ 
      log_warn("ch %d - ilip write access denied\n", channel);
      log_info("session %d, message %d, data %d\n",
               msg->u.imm.header.session_tag,
               msg->u.imm.header.message_tag,
               msg->u.imm.header.data_tag);
      gaps_ilip_copy_write_reject_count[channel]++; 
      return; 
    }
  } 
  else
  {
    if (gaps_ilip_access_write(msg->u.pay.header.session_tag,
                               msg->u.pay.header.message_tag,
                               msg->u.pay.header.desc_tag,
                               msg->u.pay.payload.data_tag,
                               &index) == false)
    { 
      /* Not allowed write access, so drop the packet */ 
      log_warn("ch %d - ilip write access denied\n", channel);
      log_info("session %d, message %d, data %d\n",
               msg->u.pay.header.session_tag,
               msg->u.pay.header.message_tag,
               msg->u.pay.payload.data_tag);
      gaps_ilip_copy_write_reject_count[channel]++; 
      return; 
    }
  }

  /* Yes it can so set the ILIP time in the outgoing message, in microseconds to match the HW */ 
  msg->u.imm.time.ilip_time = (uint64_t)t/1000ul; 
  if (gaps_ilip_verbose_level > 8)
  { 
    log_info("   ilip_time: %llx microseconds\n", msg->u.imm.time.ilip_time);
  } 

  /**
   * @note The write side of the driver buffer is still in use until the memcpy 
   * that happens later. We have already posted tot he completion queue, but due to the mutex 
   * in the write() call we are single threaded until the copy function of the 
   * work queue is completed. So there is no need for a sleep/wake processing 
   * for the send function as it is synchonous at this time. 
   */

  /** 
   * =============== Receive side of ILIP processing ===============  
   */ 
     
  /*cp->read_minor has the read device, associated to the write device */
  if (gaps_ilip_verbose_level >= 8)
  { 
    log_info("(channel: %u): Dev index: %u, ReadMinor: %d, Dev@ %p\n",
             channel, 2*(channel+1), cp->read_minor, 
             &gaps_ilip_devices[cp->read_minor]);
  }
  /* Get the read device minor number, private device control structure */
  if ((cp->read_minor < 0) || (cp->read_minor >= gaps_ilip_total_devices))
  {
    log_warn("(read minor: %d) Invalid\n", cp->read_minor);
  }
  else
  {
    if (gaps_ilip_verbose_level >= 5)
    {
      log_warn("(read minor: %d)\n", cp->read_minor);
    }
  }
  cp->dst = &gaps_ilip_devices[cp->read_minor];
  if (gaps_ilip_verbose_level >= 5)
  {
    log_info("(read minor: %d) device minor: %u target: %p\n",
             cp->read_minor, cp->dst->mn, (cp->dst_handle)->data);
  }
  if (cp->read_minor != cp->dst->mn)
  {
    log_warn("(read minor: %d/%u) Invalid\n", cp->read_minor, cp->dst->mn);
    return;
  }

  if ((cp->dst_handle) == NULL)
  { 
    if (cp->dst == NULL)
    {
      log_warn("ilip destination device is: %p\n", cp->dst);
      gaps_ilip_copy_write_reject_count[channel] ++; 
      return; 
    }
    if (channel == 0)
    {
      /* Get a fixed size buffer */
      cp->dst_handle = iops.get_buffer(cp->dst, channel);
    }
    else
    {
      unsigned int len;

      /* Calculate the length */
      len = ntohl(*(unsigned int *)((char *)((cp->src_handle)->data) + 0x3C));

      /* Add the device block size */
      len += cp->src->block_size;

      /* Allocate a buffer based on the size in the descriptor */
      cp->dst_handle = iops.alloc_buffer(cp->dst, len);
    }
  } 

  /* Are we allowed to receive this message  */
  if (channel == 0)
  {
    if (gaps_ilip_access_read(msg->u.imm.header.session_tag,
                              msg->u.imm.header.message_tag,
                              msg->u.imm.header.data_tag,
                              msg->u.imm.header.data_tag) == false)
    {
      /* Free the allocated buffer */
      iops.free_buffer(cp->dst, cp->dst_handle);

      /* Not allowed read access, so drop the packet*/
      log_warn("ilip read access denied: %x\n", cp->end_marker);
      gaps_ilip_copy_read_reject_count[channel]++;
      return;
    }
  }
  else
  {
    if (gaps_ilip_access_read(msg->u.pay.header.session_tag,
                              msg->u.pay.header.message_tag,
                              msg->u.pay.header.desc_tag,
                              msg->u.pay.payload.data_tag) == false)
    {
      /* Free the allocated buffer */
      iops.free_buffer(cp->dst, cp->dst_handle);

      /* Not allowed read access, so drop the packet*/
      log_warn("ilip read access denied: %x\n", cp->end_marker);
      gaps_ilip_copy_read_reject_count[channel]++;
      return;
    }
  }

  /* Read is going to be processed in some manner */
  gaps_ilip_copy_read_count[channel]++;

  /* Reset the copy first time as it now applies to the read side of the copy */
  do_copy_first_time = false;

  /**
   *  Is there a read side driver that has initialized the receive
   *  buffers.
   *  
   *  @note: This is using the [level_index], i.e. did an
   *       application on the read side open up the driver. This
   *       does not mean that the driver is open on the correct
   *       session, just means some sesion is open.
   */
  if (gaps_ilip_read_driver_initialized[channel] == false)
  {
    /* Free the allocated buffer */
    iops.free_buffer(cp->dst, cp->dst_handle);

    /* So we just exit, increment the failure count */
    gaps_ilip_copy_read_reject_count[channel]++;
    if (gaps_ilip_verbose_level >= 4)
    {
      log_info("(Read channel: %u): Read driver buffers not initialized\n", 
               channel);
    }
    return;
  }

  /* 
   * has the read section of the driver been opened and 
   * buffers allocated , done on first open
   * of the specific driver and ILIP session.
   */
  if (gaps_ilip_ready[cp->read_minor] == 0)
  {
    log_warn("(Read minor: %d): Read driver session not open\n", 
             cp->read_minor);
    log_warn("(Read minor: %d): Read buffers @ %p\n",
             cp->read_minor, (cp->dst_handle)->data);
    
    /* Free the allocated buffer */
    iops.free_buffer(cp->dst, cp->dst_handle);

    gaps_ilip_copy_read_reject_count[channel] ++;
    return;
  }

  /* Get the completion FIFO read side, this is where we determine if the copy to the read side is allowed */
  /* Post the address to the write completion FIFO after the copy */
  /* It is the driver open that sets the gaps_ilip_read_driver_index to gaps_ilip_messages */
  if (gaps_ilip_read_driver_index[channel] == gaps_ilip_messages)
  {
    /* First time through, we set the read driver index to zero */
    read_driver_index = 0;
    /* Allow the read copy as this is the first time through */
    do_copy_first_time = true;
  }
  else
  {
    /* Save the current state in case there is no room in the FIFO */
    read_driver_index = (gaps_ilip_read_driver_index[channel] + 1)%gaps_ilip_messages;
    read_driver_index_save = gaps_ilip_read_driver_index[channel];
    /* Location to receive the completion */
    rmb(); /* Not sure about this, will cause read to look at buffer to early */
  }

  /* Is there room in the read FIFO to accept the message from the writer. First time the read driver
     and the read user will be set to zero but there is a message to be handled. */
  if (read_driver_index != gaps_ilip_read_user_index[channel] || do_copy_first_time == true)
  {
    /* There is room in the FIFO either first time of FIFO not full */
    do_copy_first_time = false;
    /* Get the address on the read side of where the message is to be placed */

    msg = (struct ilip_message*)(cp->dst_handle)->data;
    if (gaps_ilip_verbose_level >= 4)
    {
      log_info("(Read buffer offset: %lld): DstB @ %p SrcB @ %p "
               "Driver: %u, User: %u\n", gaps_ilip_copy_read_offset[channel],
               (cp->dst_handle)->data, (cp->src_handle)->data,
               read_driver_index, gaps_ilip_read_user_index[channel]);
    }
    if (gaps_ilip_verbose_level >= 6)
    {
      log_info("memcpy(%p, %p, %lu) ...\n", (cp->dst_handle)->data, 
               (cp->src_handle)->data, cp->dst->block_size);
    }

    memcpy((cp->dst_handle)->data, (cp->src_handle)->data, cp->dst->block_size);

    if (gaps_ilip_verbose_level >= 6)
    {
      printk(KERN_INFO "... memcpy() completed\n");
    }

    if ((channel >= 1) && (channel <= 3))
    {
      void * dst_p;
      void * src_p;
      unsigned int len;

      /* Calculate the source pointer */
      src_p = (void *)(*(u64 *)(((char *)(cp->src_handle)->data) + 0x40));

      /* Calculate the destination pointer */
      dst_p = (void *)((char *)((cp->dst_handle)->data) + cp->dst->block_size);

      /* Calculate the length */
      len = ntohl(*(unsigned int *)((char *)((cp->src_handle)->data) + 0x3C));

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,3,0)
      /* Ubuntu 19.10 */
      if (access_ok(src_p, len) == 0)
      {
        log_warn("Address: %p, Count %d Invalid\n", src_p, len);
        return;
      }

      if (access_ok(dst_p, len) == 0)
      {
        log_warn("Address: %p, Count %d Invalid\n", dst_p, len);
        return;
      }
#else
#if LINUX_VERSION_CODE == KERNEL_VERSION(4,18,0)
      /* RedHat 8.x */
      if (access_ok(src_p, len) == 0)
      {
        log_warn("Address: %p, Count %d Invalid\n", src_p, len);
        return;
      }

      if (access_ok(dst_p, len) == 0)
      {
        log_warn("Address: %p, Count %d Invalid\n", dst_p, len);
        return;
      }
#else
      /* RedHat 7.x */
      if (access_ok(VERIFY_READ, src_p, len) == 0)
      {
        log_warn("Address: %p, Count %d Invalid\n", src_p, len);
        return;
      }

      if (access_ok(VERIFY_WRITE, dst_p, len) == 0)
      {
        log_warn("Address: %p, Count %d Invalid\n", dst_p, len);
        return;
      }
#endif
#endif

      /* Copy the payload data */
      if (copy_from_user(dst_p, src_p, len) != 0)
      {
        return;
      }
    }

    if (index != 0xffffffffu)
    {
      gaps_ilip_redact((cp->dst_handle)->data, index);
    }

    /**
     * @todo At this point the data has been copied out of the write side 
     *       of the driver, so it is really only at this time we can post 
     *       to the write completion queue as this is when the buffer is 
     *       free and can be returned to the write part of the driver. 
     *       However this is on the read side of the copy routine. What we 
     *       should have is additiona set of buffers in the copy part of 
     *       the driver to represent the storage of the packets in the ILIP 
     *       hardware first on the send side and then on the receive side. 
     */

    /* Has an open been called on the user receiver side,  */
    if (gaps_ilip_read_do_read_first_time[channel] == true)
    {
      /* do this in the read() routine */
      /* Why is this here ???, this plus gaps_ilip_read_do_read_first_time allows
      a read when the driver and user indices are the same in the
      read queue, this only happens the first time through on the read side. */
      gaps_ilip_read_user_index[channel] = 0;
    }
    /* Address of where we copied the data in the completion FIFO */
    gaps_ilip_read_completions_fifo[channel][read_driver_index] = (uintptr_t)(cp->dst_handle);
    if (gaps_ilip_verbose_level > 8)
    {
      log_info("(FIFO (User: %2u - Driver: %2u)): %lx "
               "read completion posted\n", 
               gaps_ilip_read_user_index[channel], 
               gaps_ilip_read_driver_index[channel],
               gaps_ilip_read_completions_fifo[channel][gaps_ilip_read_driver_index[channel]]);
    }
    rmb();
    /* Pointer is valid, so we can bump the driver pointer index to say there is something there */
    gaps_ilip_read_driver_index[channel] = read_driver_index;
    rmb();

    if (cp->dst != NULL)
    {
      if (gaps_ilip_verbose_level >= 6)
      {
        log_info("Wake any destination readers\n");
      }
      wake_up_interruptible(&cp->dst->read_desc_wq);
    }
    else
    {
      log_warn("destination device structure missing\n");
    }

    /* Update the offset, wrapping the offset based on the size. */
    if (gaps_ilip_verbose_level >= 4)
    {
      log_info("(Increment read buffer: %lld, Dev@ %p): Current offset %lld\n",
               cp->dst->increment, cp->dst, gaps_ilip_copy_read_offset[channel]);
    }
    gaps_ilip_copy_read_offset[channel] = (gaps_ilip_copy_read_offset[channel]+cp->dst->increment)%(loff_t)cp->dst->buffer_size;
    if (gaps_ilip_verbose_level >= 4)
    {
      log_info("(Increment read buffer: %lld, Dev@%p): Updated offset: %lld\n",
               cp->dst->increment, cp->dst, gaps_ilip_copy_read_offset[channel]);
    }
  }
  else
  {
    /* reset the index if there is no place to copy the data in the read completion FIFO */
    log_info("no reader waiting - dropping message\n");

    rmb();
    gaps_ilip_read_driver_index[channel] = read_driver_index_save;
    rmb();
    if (gaps_ilip_verbose_level > 3)
    {
      log_warn("(%p : Minor %u) read completion FIFO overflow D: %u U: %u\n",
               cp, cp->src->mn, gaps_ilip_read_driver_index[channel], 
               gaps_ilip_read_user_index[channel]);
    }
    gaps_ilip_copy_read_reject_count[channel]++;
    return;
  }

  return;
}

int
ilip_loopback_read_init
(
  struct gaps_ilip_dev * dev,
  unsigned int channel
) 
{
  (void)dev;

  /* There is interest in this channel so all messages to be posted */
  gaps_ilip_read_do_read_first_time[channel] = true;

  return 0;
}

static const char *wq_name_template = "gaps_ilip_wq_%d";

static int __init
gaps_loopback_init_module(void)
{
  int err = 0;
  int i = 0;
  int j = 0;
  int k = 0;
  char wq_name[32];
  
  /* Allocate the array of devices, each session has a read and a write minor device */
  gaps_ilip_devices = (struct gaps_ilip_dev *)kzalloc(
						      (gaps_ilip_total_devices) * sizeof(struct gaps_ilip_dev), GFP_KERNEL);
  if (gaps_ilip_devices == NULL)
  {
    err = -ENOMEM;
    goto fail;
  }

  /* Clear out the device array, as we only create the first set of standard devices */
  memset(gaps_ilip_devices, 0, (gaps_ilip_total_devices) * sizeof(struct gaps_ilip_dev));


  /* Allocate the array of message work queues times number of levels */
  /* Static allocation */
  for (i = 0; i < gaps_ilip_channels; ++i)
  {
    for (j = 0; j < gaps_ilip_messages; ++j)
    {
      k = (i*gaps_ilip_messages) + j;
      /* create the work queue to do the copy between the write and read instances */
      INIT_WORK(&gaps_ilip_queues[k].workqueue, gaps_ilip_copy);
      gaps_ilip_queues[i].start_marker = 0xffffffff;
      gaps_ilip_queues[i].end_marker   = 0xffffffff;
    }
  }
  
  /* Create our own work queue for the processing of the messages, has to be single threaded,
     we create a write and read for each level. */
  for (i = 0; i < (sizeof(gaps_ilip_wq)/sizeof(gaps_ilip_wq[0])); ++i)
  {
    snprintf( wq_name, sizeof(wq_name), wq_name_template, i );
    gaps_ilip_wq[i] = create_singlethread_workqueue(wq_name);
    if (gaps_ilip_wq[i] == NULL)
    {
      printk( KERN_WARNING "ilip create_singlethread_workqueue(level %d) failed\n", i+1 );
      err = -ENOMEM;
      goto fail;
    }
    if (gaps_ilip_verbose_level > 2)
    {
      printk( KERN_INFO "ilip create_singlethread_workqueue(level %d)@ %p Name: %s\n", 
	      i+1, gaps_ilip_wq[i], wq_name );
    }
  }
  
  /* create the work queue to do the copy between the write and read instances */
  INIT_WORK( &gaps_ilip_local_workqueue.workqueue, gaps_ilip_copy );
  
  for (j = 0; j < gaps_ilip_channels; j++)
  {
    for (i = 0; i < gaps_ilip_messages; i++)
    {
      gaps_ilip_write_completions_fifo[j][i] = (uintptr_t)-1;
    }
    
    gaps_ilip_write_driver_index[j] = gaps_ilip_messages;
    gaps_ilip_write_user_index[j] = gaps_ilip_messages;
    gaps_ilip_message_write_reject_count[j] = 0;
    gaps_ilip_message_write_count[j] = 0;
    gaps_ilip_copy_write_reject_count[j] = 0;
    gaps_ilip_copy_write_count[j] = 0;
                
    for (i = 0; i < gaps_ilip_messages; i++)
    {
      gaps_ilip_read_completions_fifo[j][i] = (uintptr_t)-1;
    }
    gaps_ilip_read_driver_index[j] = gaps_ilip_messages;
    gaps_ilip_read_user_index[j] = gaps_ilip_messages;
    gaps_ilip_message_read_reject_count[j] = 0;
    gaps_ilip_message_read_count[j] = 0;
    gaps_ilip_copy_read_reject_count[j] = 0;
    gaps_ilip_copy_read_count[j] = 0;
    gaps_ilip_copy_read_offset[j] = 0;
  }

  return 0; /* success */

fail:
  return err;
}

static void 
gaps_loopback_cleanup_module
(
  void
)
{
  int i;

  flush_scheduled_work();
  for (i = 0; i < (sizeof(gaps_ilip_wq)/sizeof(gaps_ilip_wq[0])); ++i)
  {
    if (gaps_ilip_wq[i] != NULL)
    {
      flush_workqueue(gaps_ilip_wq[i]);
      destroy_workqueue( gaps_ilip_wq[i]);
      gaps_ilip_wq[i] = NULL;
    }
  }

  if (gaps_ilip_devices)
  {
    kfree( gaps_ilip_devices);
  }
  return;
}

static void __exit 
gaps_loopback_exit_module
(
  void
)
{
  gaps_loopback_cleanup_module();

  return;
}

int
ilip_loopback_get_statistics
(
  uint32_t mn,
  uint32_t session_id,
  uint32_t session_index,
  uint32_t direction,
  struct ilip_session_statistics * stat
)
{
  struct gaps_ilip_dev * gaps_dev;
  int verbosity = gaps_ilip_get_nt_verbose_level();
  int channel;
  int rc = -1;
   
  if (stat == NULL)
  {
    log_warn("Invalid statistics buffer\n");
    goto error_return;
  }

  log_info("session_index %d\n", session_index);

  gaps_dev = &gaps_ilip_devices[mn];
  if (gaps_dev == NULL)
  {
    log_warn("Null device pointers\n");
    goto error_return;
  }

  channel = gaps_ilip_get_channel_from_minor(gaps_dev, mn);

  log_info("channel %d, mn %d, direction %d\n", channel, mn, direction);
  
  if (direction == 0)  /* Send device */
  {
    stat->send_count = gaps_ilip_message_write_count[channel];
    stat->send_reject_count = gaps_ilip_message_write_reject_count[channel];

    stat->receive_count = 0xffffffff;
    stat->receive_reject_count = 0xffffffff;
  
    stat->send_ilip_count = gaps_ilip_copy_write_count[channel];
    stat->send_ilip_reject_count = gaps_ilip_copy_write_reject_count[channel];

    stat->receive_ilip_count = 0xffffffff;
    stat->receive_ilip_reject_count = 0xffffffff;
  }
  else  /* Receive device */
  {
    stat->receive_count = gaps_ilip_message_read_count[channel];
    stat->receive_reject_count = gaps_ilip_message_read_reject_count[channel];

    stat->send_count = 0xffffffff;
    stat->send_reject_count = 0xffffffff;
  
    stat->receive_ilip_count = gaps_ilip_copy_read_count[channel];
    stat->receive_ilip_reject_count = gaps_ilip_copy_read_reject_count[channel];

    stat->send_ilip_count = 0xffffffff;
    stat->send_ilip_reject_count = 0xffffffff;
  }

  stat->receive_cable_count = 0xffffffff;
  stat->receive_cable_reject_count = 0xffffffff;
  
  if (verbosity >= 5)
  {
    log_info("(Session: %.8x) Index: %u\n", session_id, session_index);

    printk(KERN_INFO "                   send: %u (ilip)\n", 
           stat->send_count);
    printk(KERN_INFO "                receive: %u (ilip)\n", 
           stat->receive_count);
    printk(KERN_INFO "            send reject: %u (ilip)\n", 
           stat->send_reject_count);
    printk(KERN_INFO "         receive reject: %u (ilip)\n", 
           stat->receive_reject_count);
    printk(KERN_INFO "              send ilip: %u (ilip)\n", 
           stat->send_ilip_count);
    printk(KERN_INFO "           receive ilip: %u (ilip)\n", 
           stat->receive_ilip_count);
    printk(KERN_INFO "       send ilip reject: %u (ilip)\n", 
           stat->send_ilip_reject_count);
    printk(KERN_INFO "    receive ilip reject: %u (ilip)\n", 
           stat->receive_ilip_reject_count);
    
    log_info("(Session: %8x) Index: %u\n", session_id, session_index);

    printk(KERN_INFO "                   send: %u (ilip)\n", 
           gaps_ilip_message_write_count[channel]);
    printk(KERN_INFO "                receive: %u (ilip)\n", 
           gaps_ilip_message_read_count[channel]);
    printk(KERN_INFO "            send reject: %u (ilip)\n", 
           gaps_ilip_message_write_reject_count[channel]);
    printk(KERN_INFO "         receive reject: %u (ilip)\n", 
           gaps_ilip_message_read_reject_count[channel]);
    printk(KERN_INFO "              send ilip: %u (ilip)\n", 
           gaps_ilip_copy_write_count[channel]);
    printk(KERN_INFO "           receive ilip: %u (ilip)\n", 
           gaps_ilip_copy_read_count[channel]);
    printk(KERN_INFO "       send ilip reject: %u (ilip)\n", 
           gaps_ilip_copy_write_reject_count[channel]);
    printk(KERN_INFO "    receive ilip reject: %u (ilip)\n", 
           gaps_ilip_copy_read_reject_count[channel]);
  } 
  rc = 0;
  
error_return:
  return rc;
}

int 
ilip_loopback_init_write_counters
(
  struct gaps_ilip_dev * gaps_dev,
  unsigned int channel
)
{
  int i;
	
  for (i = 0; i < gaps_ilip_messages; i++)
  { 
    gaps_ilip_write_completions_fifo[channel][i] = (uintptr_t)-1;
  }

  gaps_ilip_write_driver_index[channel] = gaps_ilip_messages;
  gaps_ilip_write_user_index[channel] = gaps_ilip_messages;
  gaps_ilip_message_write_reject_count[channel] = 0;
  gaps_ilip_message_write_count[channel] = 0;
  gaps_ilip_copy_write_reject_count[channel] = 0;
  gaps_ilip_copy_write_count[channel] = 0;

  return 0;
}

int 
ilip_loopback_init_read_counters
(
  struct gaps_ilip_dev * gaps_dev,
  unsigned int channel
)
{
  int i;

  for (i = 0; i < gaps_ilip_messages; i++)
  {
    gaps_ilip_read_completions_fifo[channel][i] = (uintptr_t)-1;
  }

  gaps_ilip_read_driver_index[channel] = gaps_ilip_messages;
  gaps_ilip_read_user_index[channel] = gaps_ilip_messages;
  gaps_ilip_message_read_reject_count[channel] = 0;
  gaps_ilip_message_read_count[channel] = 0;
  gaps_ilip_copy_read_reject_count[channel] = 0;
  gaps_ilip_copy_read_count[channel] = 0;
  gaps_ilip_copy_read_offset[channel] = 0;

  return 0;
}

int
ilip_loopback_clear_counters
(
  unsigned int mn,
  unsigned int direction
)
{
  struct gaps_ilip_dev * gaps_dev;
  int channel;

  gaps_dev = &gaps_ilip_devices[mn];
  if (gaps_dev == NULL)
  {
    log_warn("Null device pointer\n");
    return -1;
  }

  channel = gaps_ilip_get_channel_from_minor(gaps_dev, mn);
  
  if (direction == 0)  /* Send device */
  {
    gaps_ilip_message_write_count[channel] = 0;
    gaps_ilip_message_write_reject_count[channel] = 0;
    gaps_ilip_copy_write_count[channel] = 0;
    gaps_ilip_copy_write_reject_count[channel] = 0;
  }
  else  /* Receive device */
  {
    gaps_ilip_message_read_count[channel] = 0;
    gaps_ilip_message_read_reject_count[channel] = 0;
    gaps_ilip_copy_read_count[channel] = 0;
    gaps_ilip_copy_read_reject_count[channel] = 0;
  }

  return 0;
}


int
ilip_loopback_get_finalw_statistics
(
  struct gaps_ilip_dev * gaps_dev,
  unsigned int * good, 
  unsigned int * bad, 
  unsigned int * c_good,
  unsigned int * c_bad,
  unsigned int channel
)
{
  *good = gaps_ilip_message_write_count[channel];
  *bad = gaps_ilip_message_write_reject_count[channel];
  *c_good = gaps_ilip_copy_write_count[channel];
  *c_bad = gaps_ilip_copy_write_reject_count[channel];

  return 0;
}

int 
ilip_loopback_get_finalr_statistics
(
  struct gaps_ilip_dev * gaps_dev,
  unsigned int * good, 
  unsigned int * bad, 
  unsigned int * c_good,
  unsigned int * c_bad,
  unsigned int channel
)
{
  *good = gaps_ilip_message_read_count[channel];
  *bad = gaps_ilip_message_read_reject_count[channel];
  *c_good = gaps_ilip_copy_read_count[channel];
  *c_bad = gaps_ilip_copy_read_reject_count[channel];

  return 0;
}

int
ilip_loopback_write
(
  struct gaps_ilip_dev * gaps_dev,
  struct gaps_ilip_copy_workqueue * wq,
  unsigned int verbosity
) 
{
  bool do_read_first_time = false;
  ssize_t retval = wq->length;
  unsigned write_user_index;
  unsigned int channel;

  channel = gaps_ilip_get_channel_from_minor(gaps_dev, gaps_dev->mn);
  if (channel == 0xffffffffu)
  {
      log_warn("(Minor: %u) invalid\n", gaps_dev->mn);
      retval = -ENODEV;
      goto error_return;
  }

  /* Schedule the copy to be done */
  if (verbosity > 8)
  {
    log_info("(schedule_work(%p))\n", &wq->workqueue);
  }

  /* Initialize the workqueue */
  INIT_WORK(&wq->workqueue, gaps_ilip_copy);

  schedule_work(&wq->workqueue);

  /* Wait for the work to be done */
  flush_scheduled_work();
  if (verbosity > 8)
  {
    log_info("(flush_scheduled_work()) completed\n");
  }
  
  /* Use the completion FIFO for the proper ending condition */
  if (gaps_ilip_write_user_index[channel] == gaps_ilip_messages)
  {
    /* Initialize the read index into the FIFO on first time use */
    gaps_ilip_write_user_index[channel] = 0;
    write_user_index = 0;
    do_read_first_time = true;
  }
  else
  {
    write_user_index = gaps_ilip_write_user_index[channel];
  }
  if ((write_user_index != gaps_ilip_write_driver_index[channel]) || 
       ((do_read_first_time == true) && 
	(gaps_ilip_write_completions_fifo[channel][write_user_index] != (uintptr_t)-1)))
  {
    /* Increment to the FIFO location */
    if (do_read_first_time == false)
    {
      write_user_index = (gaps_ilip_write_user_index[channel] + 1) % gaps_ilip_messages;
    }
    if (verbosity > 8)
    {
      printk(KERN_INFO "gaps_ilip_write( FIFO (User: %2u - Driver: %2u) ): %lx\n", 
	     write_user_index, gaps_ilip_write_driver_index[channel],
	     gaps_ilip_write_completions_fifo[channel][gaps_ilip_write_user_index[channel]] );
    }
    /* Clear the entry in the FIFO */
    gaps_ilip_write_completions_fifo[channel][write_user_index] = (uintptr_t)-1;
    wmb();
    gaps_ilip_write_user_index[channel] = write_user_index;
    wmb();
  }
  else
  {
    if (verbosity >= 2)
    {
      printk(KERN_WARNING "gaps_ilip_write( channel: %u FIFO (User: %2u - Driver: %2u) ): %lx invalid FIFO state\n", channel,
	     gaps_ilip_write_user_index[channel], gaps_ilip_write_driver_index[channel],
	     gaps_ilip_write_completions_fifo[channel][gaps_ilip_write_user_index[channel]] );
    }
    retval = -EAGAIN;
  }

error_return:
  return retval;
}

int
ilip_loopback_read
(
  struct gaps_ilip_dev * gaps_dev,
  char __user * buf,
  size_t count,
  unsigned int verbosity
)
{
  int retval = 0;
  unsigned int channel;    

  channel = gaps_ilip_get_channel_from_minor(gaps_dev, gaps_dev->mn);                         
  if (channel == 0xffffffffu)
  {
    retval = -ENODEV;
    goto out;
  }

  if (gaps_ilip_read_do_read_first_time[channel] == false)
  {
    gaps_ilip_read_user_index[channel] = (gaps_ilip_read_user_index[channel] + 1) % gaps_ilip_messages;
  }
  else
  {
    /* clear the first read command, now we will increment the read offset */
    gaps_ilip_read_do_read_first_time[channel] = false;
  }
  
  if (verbosity > 8)
  {
    printk(KERN_INFO "gaps_ilip_read( FIFO (User: %2u - Driver: %2u) ): %lx\n",
	   gaps_ilip_read_user_index[channel], 
           gaps_ilip_read_driver_index[channel],
	   gaps_ilip_read_completions_fifo[channel][gaps_ilip_read_user_index[channel]]);
  }
  
  /* copy to user, I should not need the -1 check on the address */
  if (gaps_ilip_read_completions_fifo[channel][gaps_ilip_read_user_index[channel]] != (uintptr_t)-1)
  {
    struct ilip_message * msg;
    uint32_t len = gaps_ilip_block_size;

    /* Get a pointer to the source */
    msg = (struct ilip_message *)((smb_t *)gaps_ilip_read_completions_fifo
             [channel][gaps_ilip_read_user_index[channel]])->data;

    if (channel != 0)
    {
      /* Get the length of the payload */
      len += ntohl(msg->u.pay.payload.payload_len);
    }

    /* Adjust count to be not greater than the total message length */
    count = (count > len) ? len : count;

    if (copy_to_user(buf, (void *)((smb_t *)gaps_ilip_read_completions_fifo[channel][gaps_ilip_read_user_index[channel]])->data, count) != 0)
    {
      retval = -EFAULT;
      gaps_ilip_message_read_reject_count[channel]++;
      goto out;
    } 
    
    /* Clear the entry in the FIFO */
    iops.free_buffer(gaps_dev, (smb_t *)gaps_ilip_read_completions_fifo[channel][gaps_ilip_read_user_index[channel]]);
    gaps_ilip_read_completions_fifo[channel][gaps_ilip_read_user_index[channel]] = (uintptr_t)-1;
    gaps_ilip_message_read_count[channel]++;
  }
  else
  {
    if (verbosity > 1)
    {
      printk(KERN_WARNING "gaps_ilip_read( FIFO (User: %2u - Driver: %2u) ): UserBuffer@ %p, invalid address\n",
	     gaps_ilip_read_user_index[channel], gaps_ilip_read_driver_index[channel],
	     (void *)gaps_ilip_read_completions_fifo[channel][gaps_ilip_read_user_index[channel]] );
    }
  } 

  retval = count;

out:
  return retval;
}

void 
ilip_loopback_free_buffer
(
  struct gaps_ilip_dev * gaps_dev,
  smb_t * handle
) 
{
  kfree((void *)handle->data);
  kfree((void *)handle);
}

smb_t *
ilip_loopback_alloc_buffer
(
  struct gaps_ilip_dev * gaps_dev,
  unsigned int length
)
{
  smb_t * smb = (smb_t *)kzalloc(sizeof(smb_t), GFP_KERNEL);
  if (smb == NULL)
  {
    log_warn("open(): out of memory\n");
    return 0;
  }

  smb->data = kzalloc(length, GFP_KERNEL);
  if (smb->data == NULL)
  {
    log_warn("open(): out of memory\n");
    return 0;
  }

  return smb;
}

smb_t *
ilip_loopback_get_buffer
(
  struct gaps_ilip_dev * gaps_dev,
  unsigned int channel
) 
{
  smb_t * smb = (smb_t *)kzalloc(sizeof(smb_t), GFP_KERNEL);
  if (smb == NULL)
  {
    log_warn("open(): out of memory\n");
    return 0;
  }

  /* Do something with channel later */
  (void)channel;

  smb->data = kzalloc(gaps_ilip_block_size, GFP_KERNEL);
  if (smb->data == NULL)
  {
    log_warn("open(): out of memory\n");
    return 0;
  }

  return smb;
}

bool 
ilip_loopback_read_data_available
(
  struct gaps_ilip_dev * gaps_dev
)
{
  unsigned int channel;
  bool rc = false;

  channel = gaps_ilip_get_channel_from_minor(gaps_dev, gaps_dev->mn);
  if (channel == 0xffffffffu)
  {
    log_warn("Minor: %u, invalid channel\n", gaps_dev->mn);
    goto error_return;
  }

  rc = ((gaps_ilip_read_user_index[channel] != gaps_ilip_read_driver_index[channel]) ||
        ((gaps_ilip_read_do_read_first_time[channel] == true) && 
         (gaps_ilip_read_user_index[channel] < gaps_ilip_messages) && 
         (gaps_ilip_read_completions_fifo[channel][gaps_ilip_read_user_index[channel]] != (uintptr_t)-1)));

error_return:
  if (gaps_ilip_verbose_level >= 7)
  {
    log_info("(Minor: %u): Result: %d\n", gaps_dev->mn, rc);
  }
  return rc;
}

module_init(gaps_loopback_init_module);
module_exit(gaps_loopback_exit_module);

/* ================================================================ */

