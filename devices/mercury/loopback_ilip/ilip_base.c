
/* Derived from cfake.c - implementation of a simple module for a character device 
 * can be used for testing, demonstrations, etc.
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


/* ------------------ MERCURY SYSTEMS INC IP COPYRIGHT HEADER  ------------------------
 *
 * Copyright:
 *  Copyright (c) 1984-2020,  Mercury Systems, Inc.,
 *  Andover MA.,and all third party embedded software sources.
 *  All rights reserved under the Copyright laws of US. and international treaties.
 *
 * ------------------ MERCURY SYSTEMS INC IP COPYRIGHT HEADER  ------------------------*/

/*
 * This material is based upon work supported by the Defense Advanced Research Projects 
 * Agency (DARPA) under Contract No. HR011-19-C-0105. 
 * Any opinions, findings and conclusions or recommendations expressed in this 
 * material are those of the author(s) and do not necessarily reflect the views 
 * of the Defense Advanced Research Projects Agency (DARPA).
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
#include <linux/sched/signal.h>
#include <linux/seq_file.h>

#include <linux/uaccess.h>

#include "ilip_base.h"
#include "ilip_nl.h"

MODULE_AUTHOR("Michael Desrochers");
MODULE_LICENSE("GPL");

#define GAPS_ILIP_DEVICE_NAME "gaps_ilip_"

/* parameters */
static unsigned int gaps_ilip_levels = GAPS_ILIP_LEVELS;
static unsigned int gaps_ilip_messages = GAPS_ILIP_MESSAGE_COUNT;
static unsigned int gaps_ilip_ndevices = GAPS_ILIP_NDEVICES;
static unsigned int gaps_ilip_total_devices = GAPS_ILIP_TOTAL_DEVICES;
static unsigned long gaps_ilip_buffer_size = GAPS_ILIP_BUFFER_SIZE;
static unsigned long gaps_ilip_block_size = GAPS_ILIP_BLOCK_SIZE;
/**
 * @brief Verbose level 
 *  
 * @details  The larger the number the more driver print outs
 * 
 * @author mdesroch (2/7/20)
 */
static uint gaps_ilip_verbose_level = 2;
static uint gaps_ilip_nt_verbose_level = 2;

module_param(gaps_ilip_levels, int, S_IRUGO);
MODULE_PARM_DESC( gaps_ilip_levels, "The total number of levels to be created" );
module_param(gaps_ilip_buffer_size, ulong, S_IRUGO);
MODULE_PARM_DESC( gaps_ilip_buffer_size, "Buffer size to use for read and write, must be a multiple of block size" );
module_param(gaps_ilip_block_size, ulong, S_IRUGO);
MODULE_PARM_DESC(gaps_ilip_block_size, "Message block size, must be 256 for demonstration" );
module_param(gaps_ilip_verbose_level, uint, S_IRUGO);
MODULE_PARM_DESC(gaps_ilip_verbose_level, "ilip driver verbose mode, larger is more verbose, 0 is quiet");
module_param(gaps_ilip_nt_verbose_level, uint, S_IRUGO);
MODULE_PARM_DESC(gaps_ilip_nt_verbose_level, "ilip netlink driver verbose mode, larger is more verbose, 0 is quiet");

/* ================================================================ */

static unsigned int gaps_ilip_major = 0;
static struct gaps_ilip_dev *gaps_ilip_devices = NULL;
static struct gaps_ilip_copy_workqueue gaps_ilip_queues[GAPS_ILIP_LEVELS*GAPS_ILIP_MESSAGE_COUNT];
static struct class *gaps_ilip_class = NULL;
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
static struct workqueue_struct *gaps_ilip_wq[GAPS_ILIP_LEVELS*2] = {NULL};

/**
 * @note The use of levels is not really correct, a level is an instance of the ILIP hardware 
 * DMA engine. Each DMA engine presents a set of completion FIFOs and is generally 
 * associated to a specific level of security but it does not have to be the case. 
 */

/**
 * Address FIFO of the buffer that has been consumed, read, by the ILIP and can now be 
 * recycled and used again for the next message. 
 * 
 * @author mdesroch (3/16/20)
 */
static uintptr_t gaps_ilip_write_completions_fifo[GAPS_ILIP_LEVELS][GAPS_ILIP_MESSAGE_COUNT];
/**
 * Address FIFO of read buffers by the ILIP that are ready to be copied to the user buffer presented in the application 
 * read() call. We extract the address from this FIFO and use it as a source address t the copy_to_user() 
 * call in the read() driver function. 
 * 
 * @author mdesroch (3/16/20)
 */
static uintptr_t  gaps_ilip_read_completions_fifo[GAPS_ILIP_LEVELS][GAPS_ILIP_MESSAGE_COUNT];
static unsigned int gaps_ilip_write_driver_index[GAPS_ILIP_LEVELS] = {0};
/**
 * Message index of where the last message was placed into the drivers
 * 
 * @author mdesroch (3/16/20)
 */
static unsigned int gaps_ilip_write_user_index[GAPS_ILIP_LEVELS] = {0};
static unsigned int gaps_ilip_read_driver_index[GAPS_ILIP_LEVELS] = {0};
static unsigned int gaps_ilip_read_user_index[GAPS_ILIP_LEVELS] = {0};

static bool gaps_ilip_write_driver_initialized[GAPS_ILIP_LEVELS] = {false};
static bool gaps_ilip_read_driver_initialized[GAPS_ILIP_LEVELS] = {false};

static unsigned int gaps_ilip_message_write_count[GAPS_ILIP_LEVELS];
static unsigned int gaps_ilip_message_read_count[GAPS_ILIP_LEVELS];
static unsigned int gaps_ilip_message_write_reject_count[GAPS_ILIP_LEVELS];
static unsigned int gaps_ilip_message_read_reject_count[GAPS_ILIP_LEVELS];

static unsigned int gaps_ilip_copy_write_count[GAPS_ILIP_LEVELS];
static unsigned int gaps_ilip_copy_write_reject_count[GAPS_ILIP_LEVELS];
static unsigned int gaps_ilip_copy_read_count[GAPS_ILIP_LEVELS];
static unsigned int gaps_ilip_copy_read_reject_count[GAPS_ILIP_LEVELS];

/**
 * The individual session device have a mutex or a semaphore for mutual exclusion, but 
 * we should have and may need a mutual exclusion at the ILIP DMA context level, or 
 * in the case of the loopback driver as implemented at the 'level' context. 
 */
struct mutex gaps_ilip_context_mutex[GAPS_ILIP_LEVELS];

/**
 * @brief Session array, each session will have two minor devices created, one 
 *        for read and one for write.
 */
struct ilip_session_info {
    unsigned int session;
    unsigned int level_src;
    unsigned int level_dst;
    unsigned int minor_src;
    unsigned int minor_dst;
};

static struct ilip_session_info gaps_ilip_sessions[GAPS_ILIP_NSESSIONS];

/** 
 * @brief Number of session we have created, DCD requires 7, simple demo does 
 *        not use this
 */ 
static unsigned int gaps_ilip_session_count = 0;

uint gaps_ilip_get_nt_verbose_level( void )
{
    return gaps_ilip_nt_verbose_level;
}

static bool gaps_ilip_save_session_id( unsigned int session_id );
static bool gaps_ilip_save_session_id( unsigned int session_id )
{
    unsigned int i;

    for ( i=0; i<GAPS_ILIP_NSESSIONS; i++ ) {
        if ( gaps_ilip_sessions[i].session == session_id ) {
            /* session exists, return false as we do not have to create the device */
            if ( gaps_ilip_verbose_level >= 6 ) {
                printk( KERN_INFO "gaps_ilip_save_session_id( Session: %.8x ) found at [%2u]\n", session_id, i );
            }
            return false;
        }
    }
    for ( i=0; i<GAPS_ILIP_NSESSIONS; i++ ) {
        if ( gaps_ilip_sessions[i].session == 0xffffffff ) {
            /* session empty, save session id and return true */
            gaps_ilip_sessions[i].session = session_id;
            gaps_ilip_session_count++;
            if ( gaps_ilip_verbose_level >= 6 ) {
            printk( KERN_INFO "gaps_ilip_save_session_id( Session: %.8x ) New [%2u] Count: %u\n", session_id, i, gaps_ilip_session_count );
            }
            return true;
        }
    }
    printk( KERN_WARNING "gaps_ilip_save_session_id( Session: %.8x ) No Slot Found, Count: %u\n", session_id, gaps_ilip_session_count );

    return false;
}

unsigned int gaps_ilip_get_session_index( unsigned int session_id )
{
    unsigned int i;

    for ( i=0; i<GAPS_ILIP_NSESSIONS; i++ ) {
        if ( gaps_ilip_sessions[i].session == session_id ) {
            /* session exists, return index in session array */
            return i;
        }
    }
    return GAPS_ILIP_NSESSIONS;
}

int gaps_ilip_clear_statistics( uint32_t session_id )
{
    int rc = -1;
    uint32_t session_index;
    uint32_t level_src, level_dst;

    if ( session_id == 0xffffffff ) {
        printk( KERN_WARNING "gaps_ilip_get_statistics() Invalid session ID: %.8x\n", session_id );
        goto error_return;
    }

    session_index = gaps_ilip_get_session_index( session_id );
    if ( session_index == GAPS_ILIP_NSESSIONS ) {
        printk( KERN_WARNING "gaps_ilip_get_statistics() Session not found: %.8x\n", session_id );
        goto error_return;
        }

    /* These are levels and not level index values. */
    if ( gaps_ilip_sessions[session_index].level_src <= 0 || gaps_ilip_sessions[session_index].level_src >= 3) {
        printk( KERN_WARNING "gaps_ilip_clear_statistics() Session src not initialized correctly: %.8x (%u)\n", 
                session_id, gaps_ilip_sessions[session_index].level_src );
        goto error_return;
    }
    if ( gaps_ilip_sessions[session_index].level_dst <= 0 || gaps_ilip_sessions[session_index].level_dst >= 3) {
        printk( KERN_WARNING "gaps_ilip_clear_statistics() Session dst not initialized correctly: %.8x (%u)\n", 
                session_id, gaps_ilip_sessions[session_index].level_dst  );
        goto error_return;
    }

    level_src = gaps_ilip_sessions[session_index].level_src - 1;
    level_dst = gaps_ilip_sessions[session_index].level_dst - 1;

    /** @bug Fix this   */

    gaps_ilip_message_write_count[level_src] = 0;
    gaps_ilip_message_read_count[level_src] = 0;
    gaps_ilip_message_write_reject_count[level_src] = 0;
    gaps_ilip_message_read_reject_count[level_src] = 0;

    gaps_ilip_copy_write_count[level_src] = 0;
    gaps_ilip_copy_write_reject_count[level_src] = 0;
    gaps_ilip_copy_read_count[level_src] = 0;
    gaps_ilip_copy_read_reject_count[level_src] = 0;

    rc = 0;

error_return:
    return rc;
}

int gaps_ilip_get_statistics( uint32_t session_id, struct ilip_session_statistics *stat )
{
    int rc = -1;
    uint32_t session_index;
    uint32_t level_src, level_dst;

    if ( session_id == 0xffffffff ) {
        printk( KERN_WARNING "gaps_ilip_get_statistics() Invalid session ID: %.8x\n", session_id );
        goto error_return;
    }

    if ( stat == NULL ) {
        printk( KERN_WARNING "gaps_ilip_get_statistics() Invalid statistics buffer\n" );
        goto error_return;
    }

    session_index = gaps_ilip_get_session_index( session_id );
    if ( session_index == GAPS_ILIP_NSESSIONS ) {
        printk( KERN_WARNING "gaps_ilip_get_statistics() Session not found: %.8x\n", session_id );
        goto error_return;
    }

    level_src = gaps_ilip_sessions[session_index].level_src - 1;
    level_dst = gaps_ilip_sessions[session_index].level_dst - 1;

    /* These are levels and not level index values. */
    if ( gaps_ilip_sessions[session_index].level_src <= 0 || gaps_ilip_sessions[session_index].level_src >= 3) {
        printk( KERN_WARNING "gaps_ilip_get_statistics() Session src not initialized correctly: %.8x (%u)\n", 
                session_id, gaps_ilip_sessions[session_index].level_src );
        goto error_return;
    }
    if ( gaps_ilip_sessions[session_index].level_dst <= 0 || gaps_ilip_sessions[session_index].level_dst >= 3) {
        printk( KERN_WARNING "gaps_ilip_get_statistics() Session dst not initialized correctly: %.8x (%u)\n", 
                session_id, gaps_ilip_sessions[session_index].level_dst  );
        goto error_return;
    }

    /** @bug Fix this   */
    stat->send_count = gaps_ilip_message_write_count[level_src];
    stat->receive_count = gaps_ilip_message_read_count[level_src];
    stat->send_reject_count = gaps_ilip_message_write_reject_count[level_src];
    stat->receive_reject_count = gaps_ilip_message_read_reject_count[level_src];

    stat->send_ilip_count = gaps_ilip_copy_write_count[level_src];
    stat->send_ilip_reject_count = gaps_ilip_copy_write_reject_count[level_src];
    stat->receive_ilip_count = gaps_ilip_copy_read_count[level_src];
    stat->receive_ilip_reject_count = gaps_ilip_copy_read_reject_count[level_src];

    if ( gaps_ilip_get_nt_verbose_level()  >= 5 ) {
        printk(KERN_INFO "\ngaps_ilip_get_statistics( Session: %.8x ) Index: %u SrcLevel: %u DstLevel: %u\n",
                session_id, session_index, level_src, level_dst );
        printk( KERN_INFO "                   send: %u ( ilip )\n", stat->send_count );
        printk( KERN_INFO "                receive: %u ( ilip )\n", stat->receive_count );
        printk( KERN_INFO "            send reject: %u ( ilip )\n", stat->send_reject_count );
        printk( KERN_INFO "         receive reject: %u ( ilip )\n", stat->receive_reject_count );
        printk( KERN_INFO "              send ilip: %u ( ilip )\n", stat->send_ilip_count );
        printk( KERN_INFO "           receive ilip: %u ( ilip )\n", stat->receive_ilip_count );
        printk( KERN_INFO "       send ilip reject: %u ( ilip )\n", stat->send_ilip_reject_count );
        printk( KERN_INFO "    receive ilip reject: %u ( ilip )\n", stat->receive_ilip_reject_count );

        printk( KERN_INFO "\ngaps_ilip_get_statistics( Session: %.8x ) Index: %u SrcLevel: %u\n", 
                session_id, session_index, level_src );
        printk( KERN_INFO "                   send: %u ( ilip )\n", gaps_ilip_message_write_count[level_src] );
        printk( KERN_INFO "                receive: %u ( ilip )\n", gaps_ilip_message_read_count[level_src] );
        printk( KERN_INFO "            send reject: %u ( ilip )\n", gaps_ilip_message_write_reject_count[level_src] );
        printk( KERN_INFO "         receive reject: %u ( ilip )\n", gaps_ilip_message_read_reject_count[level_src] );
        printk( KERN_INFO "              send ilip: %u ( ilip )\n", gaps_ilip_copy_write_count[level_src] );
        printk( KERN_INFO "           receive ilip: %u ( ilip )\n", gaps_ilip_copy_read_count[level_src] );
        printk( KERN_INFO "       send ilip reject: %u ( ilip )\n", gaps_ilip_copy_write_reject_count[level_src] );
        printk( KERN_INFO "    receive ilip reject: %u ( ilip )\n", gaps_ilip_copy_read_reject_count[level_src] );

        printk( KERN_INFO "\ngaps_ilip_get_statistics( Session: %.8x ) Index: %u DstLevel: %u\n", 
                session_id, session_index, level_dst );
        printk( KERN_INFO "                   send: %u ( ilip )\n", gaps_ilip_message_write_count[level_dst] );
        printk( KERN_INFO "                receive: %u ( ilip )\n", gaps_ilip_message_read_count[level_dst] );
        printk( KERN_INFO "            send reject: %u ( ilip )\n", gaps_ilip_message_write_reject_count[level_dst] );
        printk( KERN_INFO "         receive reject: %u ( ilip )\n", gaps_ilip_message_read_reject_count[level_dst] );
        printk( KERN_INFO "              send ilip: %u ( ilip )\n", gaps_ilip_copy_write_count[level_dst] );
        printk( KERN_INFO "           receive ilip: %u ( ilip )\n", gaps_ilip_copy_read_count[level_dst] );
        printk( KERN_INFO "       send ilip reject: %u ( ilip )\n", gaps_ilip_copy_write_reject_count[level_dst] );
        printk( KERN_INFO "    receive ilip reject: %u ( ilip )\n", gaps_ilip_copy_read_reject_count[level_dst] );
    }

    rc = 0;

error_return:
    return rc;
}


static bool gaps_ilip_remove_session_index( unsigned int session_id );
static bool gaps_ilip_remove_session_index( unsigned int session_id )
{
    unsigned int i;

    for ( i=0; i<gaps_ilip_session_count; i++ ) {
        if ( gaps_ilip_sessions[i].session == session_id ) {
            /* session exists, return index in session array */
            gaps_ilip_sessions[i].session = 0xffffffff;
            gaps_ilip_sessions[i].level_src = 0xffffffff;
            gaps_ilip_sessions[i].level_dst = 0xffffffff;
            gaps_ilip_sessions[i].minor_src = 0xffffffff;
            gaps_ilip_sessions[i].minor_dst = 0xffffffff;
            gaps_ilip_session_count--;
            return true;
        }
    }
    return false;
}

/**
 * @brief The number of times the open() has been called, indexed by minor 
 *        number.
 * 
 */
static DEFINE_SPINLOCK(root_open_lock);
static unsigned int gaps_ilip_open_count[GAPS_ILIP_TOTAL_DEVICES] = {0};

/**
 * @brief Indication the read side of the driver has been open by an application
 *        and it is able to receive messages in host memory even if the user
 *        read application at this level has not tried to do a read yet.
 *  
 * @details The issue is when should the ILIP be allowed to DMA data into the 
 *          host memory. If there is no application that has even opened the
 *          receive ILIP driver would not post receive buffers, and the ILIP
 *          hardware would just drop stuff on the floor. Once there is an open
 *          called on the receive channel, there is an application so the driver
 *          will post a bunch of receive buffers and the ILIP hardware will start
 *          forwarding the messages to the host, even when the application is
 *          not ready to receive the data yet. There will be a case where all of
 *          the receive buffers will get consumed and the completion FIFO will
 *          not have any slots in it so the ILIP will start to drop messages
 *          again.
 * 
 */
static bool gaps_ilip_read_do_read_first_time[GAPS_ILIP_LEVELS] = {false};
/**
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
static loff_t gaps_ilip_copy_read_offset[GAPS_ILIP_LEVELS];

/* ================================================================ */
/**
 * @brief Compute the application level index from the minor driver number 
 *  
 * @details The level as a value is 1 based i.e. we talk about level 1 or level 2, but in the 
 * driver the level is used as an index so it is zero based.
 * 
 * @author mdesroch (2/7/20)
 * 
 * @param mn Driver minor number
 * 
 * @return unsigned int Level index for driver operations.
 */
static unsigned int gaps_ilip_get_level_from_minor( struct gaps_ilip_dev *dev, unsigned int mn );
/**
 * @brief Models the behavior of the ILIP hardware. 
 *  
 * @details It has the job of copying the data from one driver buffer to another 
 *          and notifying the write side the message has been sent and notifying
 *          the read side that a new message has to be delivered.
 * 
 * @author mdesroch (2/15/20)
 * 
 * @param work Work Queue structure from which we extract the work to be done 
 *             and the data to be copied.
 */
static void gaps_ilip_copy( struct work_struct *work );

/**
 * @brief static declaration used in testing only
 * 
 * @author mdesroch (2/15/20)
 */
static struct gaps_ilip_copy_workqueue gaps_ilip_local_workqueue;
static DECLARE_WORK( gaps_ilip_workqueue, gaps_ilip_copy );

static char *gaps_ilip_devnode( struct device *dev, umode_t *mode );

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
static int gaps_ilip_construct_device(struct gaps_ilip_dev *dev, int minor, struct class *class);

/**
 * @brief the access control matrix associated to the write of the data by the 
 *        hardware ILIP.
 *  
 * @details This routine determines if the sender is allowed to write the message 
 *          to the wire. It looks at the session ID, the message and data tags
 *          to see if a write is allowed by this ILIP hardware entity.
 * 
 * @author mdesroch (2/15/20)
 * 
 * @param cp Driver details of the message to be copied
 * @param msg Message details of the data to be copied.
 * 
 * @return bool True if we are allowed to write the data, False if we are not.
 */
static bool gaps_ilip_access_write(struct gaps_ilip_copy_workqueue *cp, struct ilip_message * msg);
/**
 * @brief the access control matrix associated to the read of the data by the 
 *        hardware ILIP
 *  
 * @details This routine determines if the receiving ILIP is allowed to process 
 *          the message coming in from the wire. It looks at the session ID, the
 *          message and data tags to see if a read is allowed by this ILIP
 *          hardware entity.
 * 
 * @author mdesroch (2/15/20)
 * 
 * @param cp Driver details of the message to be copied
 * @param msg Message details of the data to be copied.
 * 
 * @return bool True if we are allowed to read the data, False if we are not.
 */
static bool  gaps_ilip_access_read(struct gaps_ilip_copy_workqueue *cp, struct ilip_message * msg);


static unsigned int jenkins_one_at_a_time_hash_init( size_t len, unsigned int initval);
static unsigned int jenkins_one_at_a_time_hash_done( unsigned int hash );
static unsigned int jenkins_one_at_a_time_hash_ex(unsigned int hash, const unsigned char *key, size_t len);
static unsigned int jenkins_one_at_a_time_hash(const void *key, size_t len, unsigned int initval);

int 
gaps_ilip_open(struct inode *inode, struct file *filp)
{
	unsigned int mj = imajor(inode);
    /**
     * if minor node is zero - this is the root node of the driver instance 
     * if minor node is even it is a read device only 
     * if minor node is odd it is a write device only 
     *  
     * On device creation we create one root node and pairs of application 
     * nodes, one to write into and one to read out of. 
     */
	unsigned int mn = iminor(inode);
    unsigned int level;
    unsigned int i;
	
	struct gaps_ilip_dev *dev = NULL;
	
	if (mj != gaps_ilip_major || mn < 0 || mn >= gaps_ilip_total_devices)
	{
		printk(KERN_WARNING "gaps_ilip_open() "
			"No device found with minor=%d and major=%d\n", 
			mj, mn);
		return -ENODEV; /* No such device */
	}

    /* Root device can only be opened one application at a time so
       as to compute the correct session data */
    spin_lock(&root_open_lock);
    if( mn == 0 && gaps_ilip_open_count[mn] >= 1  ) {
        spin_unlock(&root_open_lock);
        printk(KERN_WARNING "gaps_ilip_open() open: root busy\n");
        return -EBUSY; /* Try later, root busy */
    }

    /* increment the open count on this device */
    gaps_ilip_open_count[mn]++;
    spin_unlock(&root_open_lock);
	
	/* store a pointer to struct gaps_ilip_dev here for other methods */
	dev = &gaps_ilip_devices[mn];
	filp->private_data = dev; 
    level = gaps_ilip_get_level_from_minor( dev, mn );

	if (inode->i_cdev != &dev->cdev)
	{
		printk(KERN_WARNING "gaps_ilip_open() open: internal error\n");
		return -ENODEV; /* No such device */
	}

	/* if opened the 1st time, allocate the buffer */
	if (dev->data == NULL)
	{
		dev->data = (unsigned char*)kzalloc(dev->buffer_size, GFP_KERNEL);
		if (dev->data == NULL)
		{
			printk(KERN_WARNING "gaps_ilip_open() open(): out of memory\n");
			return -ENOMEM;
		}
	}

    if ( gaps_ilip_verbose_level >= 1 ) {
    printk( KERN_INFO "gaps_ilip_open( Minor: %d ) Data@ %p Length: %lu Non-Blocking flag: %d\n", mn, dev->data, dev->buffer_size, ((filp->f_flags & O_NONBLOCK) != 0) );
    }

    /* On a root device open, zero out the buffer, we may have used it before. */
    if ( mn == 0 ) {
        memset( dev->data, 0x0, dev->buffer_size );
    }

    if ( gaps_ilip_verbose_level > 8 ) {
        printk(KERN_INFO "gaps_ilip_open( Minor: %u ) open count: %u\n", 
               mn, gaps_ilip_open_count[mn] );
    }

    /* Only initialize the minor device once */
    if ( gaps_ilip_open_count[mn] == 1) {
        /* Setup the completion FIFOs based on the level and access mode for all non root devices */
        if ( mn != 0 ) {
            if ( level == 0xffffffff || level >= 2 ) {
                printk( KERN_WARNING "gaps_ilip_open() Invalid level: %u in session: %.8x\n", level, dev->session_id );
                return -EBADF;
            }
            if ((filp->f_flags & O_ACCMODE) == O_WRONLY) {
                if ( gaps_ilip_verbose_level > 5 ) {
                    printk(KERN_INFO "\n\n----------------\ngaps_ilip_open( Minor: %u ) send channel O_WRONLY, level: %u, Messages: %u\n", 
                           mn, level, gaps_ilip_messages );
                }
                for (i = 0; i < gaps_ilip_messages; i++) {
                    gaps_ilip_write_completions_fifo[level][i] = (uintptr_t)-1;
                }
                gaps_ilip_write_driver_index[level] = gaps_ilip_messages;
                gaps_ilip_write_user_index[level] = gaps_ilip_messages;
                gaps_ilip_message_write_reject_count[level] = 0;
                gaps_ilip_message_write_count[level] = 0;
                gaps_ilip_copy_write_reject_count[level] = 0;
                gaps_ilip_copy_write_count[level] = 0;
            } else if( (filp->f_flags & O_ACCMODE) == O_RDONLY ) {
                if ( gaps_ilip_verbose_level > 5 ) {
                    printk(KERN_INFO "\n\n----------------\ngaps_ilip_open( Minor: %u ) receive channel O_RDONLY, level: %u, Messages: %u\n", 
                           mn, level, gaps_ilip_messages );
                }
                for (i = 0; i < gaps_ilip_messages; i++) {
                    gaps_ilip_read_completions_fifo[level][i] = (uintptr_t)-1;
                }
                gaps_ilip_read_driver_index[level] = gaps_ilip_messages;
                gaps_ilip_read_user_index[level] = gaps_ilip_messages;
                gaps_ilip_message_read_reject_count[level] = 0;
                gaps_ilip_message_read_count[level] = 0;
                gaps_ilip_copy_read_reject_count[level] = 0;
                gaps_ilip_copy_read_count[level] = 0;
                gaps_ilip_copy_read_offset[level] = 0;
            } else {
                printk(KERN_WARNING "gaps_ilip_open() open: Invalid access mode\n");
                return -EACCES;
            }

            /* Indicate the driver is initialized for read or write operations */
            if ( level != 0xffffffff ) {
                if ( (filp->f_flags & O_ACCMODE) == O_WRONLY ) {
                    /* To handle the case of the first write */
                    gaps_ilip_write_driver_initialized[level] = true;
                } else if( (filp->f_flags & O_ACCMODE) == O_RDONLY ) {
                    /* There is interest in this channel so all messages to be posted */
                    /* To handle the case of the first read */
                    gaps_ilip_read_do_read_first_time[level] = true;
                    /* Driver initialized */
                    gaps_ilip_read_driver_initialized[level] = true;
                }
            }
        }

        if ( gaps_ilip_verbose_level > 0 ) {
            printk(KERN_INFO "gaps_ilip_open( %u ): level: %x Major: %u - Minor: %u\n", gaps_ilip_open_count[mn], level + 1, mj, mn);
        }
    } else {
        if ( gaps_ilip_verbose_level > 5 ) {
        printk(KERN_INFO "gaps_ilip_open( %u ): level: %x Major: %u - Minor: %u - Already Opened!\n", gaps_ilip_open_count[mn], level + 1, mj, mn);
        }
    }

	return 0;
}

int 
gaps_ilip_release(struct inode *inode, struct file *filp)
{
    unsigned int mj = imajor(inode);
    unsigned int mn = iminor(inode);
    unsigned int good = 0;
    unsigned int bad = 0;
    unsigned int c_good = 0;
    unsigned int c_bad = 0;
	struct gaps_ilip_dev *dev = (struct gaps_ilip_dev *)filp->private_data;
    unsigned int level = gaps_ilip_get_level_from_minor( dev, mn );

    /* decrement the open count on this device */
    if (mn == 0) {
        spin_lock(&root_open_lock);
    }
    
    if ( gaps_ilip_open_count[mn] > 0 ) {
        gaps_ilip_open_count[mn]--;
    } else {
        printk(KERN_WARNING "gaps_ilip_release( %u ): level: %x Major: %u - Minor: %u open counter error\n", 
               gaps_ilip_open_count[mn], level+1, mj, mn );
    }

    if (mn == 0) {
        spin_unlock(&root_open_lock);
    }

    if ( gaps_ilip_verbose_level >= 2 ) {
        printk( KERN_INFO "gaps_ilip_release(Minor: %u ) Level: %x OpenCount: %u\n", mn, level, gaps_ilip_open_count[mn] );
    }

    /* Close and reset the device if all users have done the close */
    if ( gaps_ilip_open_count[mn] == 0 ) {
        if (mn != 0) {
            /* Not root device */
            if ( (filp->f_flags & O_ACCMODE) == O_WRONLY ) {
                gaps_ilip_write_driver_initialized[level] = false;
                good = gaps_ilip_message_write_count[level];
                bad = gaps_ilip_message_write_reject_count[level];
                c_good = gaps_ilip_copy_write_count[level];
                c_bad = gaps_ilip_copy_write_reject_count[level];
            } else if( (filp->f_flags & O_ACCMODE) == O_RDONLY ) {
                gaps_ilip_read_driver_initialized[level] = false;
                good = gaps_ilip_message_read_count[level];
                bad = gaps_ilip_message_read_reject_count[level];
                c_good = gaps_ilip_copy_read_count[level];
                c_bad = gaps_ilip_copy_read_reject_count[level];
            }
        } else {
            /* Root device */
            dev->src_level = 0;
            dev->dst_level = 0;
            dev->source_id = 0;
            dev->destination_id = 0;
            dev->session_message_count = 0;
            dev->message_data_array = NULL;
            if ( gaps_ilip_verbose_level >= 2 ) {
            printk( KERN_INFO "gaps_ilip_release(Minor: %u ) Level: %x OpenCount: %u Clear\n", mn, level, gaps_ilip_open_count[mn] );
        }
    }
    }

    /* dump the stats */
    if ( gaps_ilip_open_count[mn] == 0 ) {
        if (level != 0xffffffff) {
            if ( gaps_ilip_verbose_level > 0 ) {
                printk(KERN_INFO "gaps_ilip_release( %2u ): level: %.2x Major: %3u - Minor: %3u %s "
                                 "[Messages: %7u Rejects: %7u] [Copy: %7u Rejects: %7u]\n", 
                       gaps_ilip_open_count[mn], level+1, mj, mn, ((mn%2)==0)?" read":"write", 
                       good, bad, c_good, c_bad );
            }
        }
    }

    return 0;
}

/**
 * @todo Need a way to send a message that will be rejected on the read side as
 *       well as the write side.
 * 
 */
static bool gaps_ilip_access_read(struct gaps_ilip_copy_workqueue *cp, struct ilip_message * msg)
{
    bool rc = false;

    if ( cp == NULL || msg == NULL || cp->dst == NULL || cp->src == NULL ) {
        printk(KERN_WARNING "gaps_ilip_access_read( cp: %p, msg: %p ) Invalid call parameters\n", cp, msg );
        goto error_return;
    }

    /* count has to be one for the demo */
    if ( ntohl(msg->header.count) != 1 ) {
        printk(KERN_WARNING "gaps_ilip_access_read( count: %u ) Invalid\n", ntohl(msg->header.count) );
        goto error_return;
    }
    /* Session 1, level 1, application 1 in minor number 1 */
    switch (ntohl(msg->header.session)) {
    case 1:
        /* A hack as we should not really have the session ID attached to the device minor number */
        if( cp->dst->mn != 2 ) {
            printk( KERN_WARNING "gaps_ilip_access_read(1) Invalid minor number: Src: %d Dst: %d for session: %.8x\n",
                    cp->src->mn, cp->dst->mn, ntohl(msg->header.session) );
            goto error_return;
        }
/* See https://stackoverflow.com/questions/45349079/how-to-use-attribute-fallthrough-correctly-in-gcc */
#if KERNEL_VERSION(5, 2, 0) >= LINUX_VERSION_CODE
        /* fall through */
#else
        __attribute__((__fallthrough__));
#endif
    case 0xeca51756:
        if ( gaps_ilip_verbose_level >= 8 ) {
        printk( KERN_INFO "gaps_ilip_access_read( Session: %.8x SrcMn: %u, DstMn: %u Dst: %p )\n", 
                ntohl(msg->header.session),
                cp->src->mn,
                cp->dst->mn,
                cp->dst );
        }
        switch ( ntohl(msg->header.message) ) {
        case 1:
            switch ( ntohl(msg->header.data_tag) ) {
            case 1:
            case 3:
                break;
            default:
                goto error_return;
            }
            break;
        case 2:
            switch ( ntohl(msg->header.data_tag) ) {
            case 3:
                break;
            default:
                goto error_return;
            }
            break;
        case 3:
            switch ( ntohl(msg->header.data_tag) ) {
            case 3:
                break;
            default:
                goto error_return;
            }
            break;
        default:
            goto error_return;
        }
        break;
    case 2:
        /* A hack as we should not really have the session ID attached to the device minor number */
        if( cp->dst->mn != 4 ) {
            printk( KERN_WARNING "gaps_ilip_access_read(2) Invalid minor number: Src: %d Dst: %d for session: %.8x\n",
                    cp->src->mn, cp->dst->mn, ntohl(msg->header.session) );
            goto error_return;
        }
/* See https://stackoverflow.com/questions/45349079/how-to-use-attribute-fallthrough-correctly-in-gcc */
#if KERNEL_VERSION(5, 2, 0) >= LINUX_VERSION_CODE
        /* fall through */
#else
        __attribute__((__fallthrough__));
#endif
    case 0x67ff90f4:
        if ( gaps_ilip_verbose_level >= 8 ) {
        printk( KERN_INFO "gaps_ilip_access_read( Session: %.8x SrcMn: %u, DstMn: %u, Dst: %p )\n", 
                ntohl(msg->header.session),
                cp->src->mn,
                cp->dst->mn,
                cp->dst );
        }
        switch ( ntohl(msg->header.message) ) {
        case 2:
            switch ( ntohl(msg->header.data_tag) ) {
            case 2:
                break;
            default:
                goto error_return;
            }
            break;
        case 3:
            switch ( ntohl(msg->header.data_tag) ) {
            case 3:
                break;
            default:
                goto error_return;
            }
            break;
        default:
            goto error_return;
        }
        break;
    case 0x6bb83e13:
        /* establish_session_id( Level: 1 Src: 1, Dst: 2 ): new session ID 0x6bb83e13 */
        switch ( ntohl(msg->header.message) ) {
        case 1:
            switch ( ntohl(msg->header.data_tag) ) {
            case 1:
                break;
            default:
                goto error_return;
            }
            break;
        case 5:
            switch ( ntohl(msg->header.data_tag) ) {
            case 3:
                break;
            default:
                goto error_return;
            }
            break;
        case 6:
            switch ( ntohl(msg->header.data_tag) ) {
            case 4:
                break;
            default:
                goto error_return;
            }
            break;
        default:
            goto error_return;
        }
        break;
    case 0x8127aa5b:
        /* establish_session_id( Level: 2 Src: 2, Dst: 1 ): new session ID 0x8127aa5b */
        switch ( ntohl(msg->header.message) ) {
        case 2:
            switch ( ntohl(msg->header.data_tag) ) {
            case 1:
                break;
            default:
                goto error_return;
            }
            break;
        case 3:
            switch ( ntohl(msg->header.data_tag) ) {
            case 1:
                break;
            default:
                goto error_return;
            }
            break;
        case 4:
            switch ( ntohl(msg->header.data_tag) ) {
            case 2:
                break;
            default:
                goto error_return;
            }
            break;
        default:
            goto error_return;
        }
        break;
    case 0x2c2b8e86:
        /* establish_session_id( Level: 1 Src: 1, Dst: 2, Msg: 1 ): new session ID 0x2c2b8e86 */
        switch ( ntohl(msg->header.message) ) {
        case 1:
            switch ( ntohl(msg->header.data_tag) ) {
            case 1:
            case 3:
            case 4:
                break;
            default:
                goto error_return;
            }
            break;
        default:
            break;
        }
        break;
    case 0x442d2490:
        /* establish_session_id( Level: 2 Src: 2, Dst: 1, Msg: 2 ): new session ID 0x442d2490 */
        switch ( ntohl(msg->header.message) ) {
        case 2:
            switch ( ntohl(msg->header.data_tag) ) {
            case 1:
            case 2:
                break;
            default:
                goto error_return;
            }
            break;
        default:
            break;
        }
        break;
    case 0xbc5a32fb:
        /* establish_session_id( Level: 1 Src: 1, Dst: 2, Msg: 1 ): new session ID 0xbc5a32fb */
        switch ( ntohl(msg->header.message) ) {
        case 1:
            switch ( ntohl(msg->header.data_tag) ) {
            case 2:
            case 5:
                break;
            default:
                goto error_return;
            }
            break;
        default:
            break;
        }
        break;
    case 0x574c9a21:
        /* establish_session_id( Level: 2 Src: 2, Dst: 1, Msg: 2 ): new session ID 0x574c9a21 */
        switch ( ntohl(msg->header.message) ) {
        case 2:
            switch ( ntohl(msg->header.data_tag) ) {
            case 1:
            case 3:
            case 4:
                break;
            default:
                goto error_return;
            }
            break;
        default:
            break;
        }
        break;
    default:
        goto error_return;
    }

    if ( gaps_ilip_verbose_level > 5 ) {
    printk(KERN_INFO "gaps_ilip_access_read() Msg: %x Minor: %u, Session: %.8x, Message: %u, Data: %u read allowed\n", 
           cp->end_marker, cp->dst->mn, 
           ntohl(msg->header.session), ntohl(msg->header.message), ntohl(msg->header.data_tag) );
    }
    rc = true;
    return rc;

error_return:
    printk(KERN_WARNING "gaps_ilip_access_read() Msg: %x Level: %u, Session: %.8x, Message: %u, Data: %u read rejected\n", 
           cp->end_marker, cp->dst->mn, 
           ntohl(msg->header.session), ntohl(msg->header.message), ntohl(msg->header.data_tag) );
    return rc;
}

static bool gaps_ilip_access_write(struct gaps_ilip_copy_workqueue *cp, struct ilip_message * msg)
{
    bool rc = false;

    /**
     * @note At this point in time the dst pointer, who will receive 
     *       the message has not neen setup yet.
     */
    if ( cp == NULL || msg == NULL || cp->src == NULL ) {
        printk(KERN_WARNING "gaps_ilip_access_write( cp: %p, msg: %p ) Invalid call parameters\n", cp, msg );
        goto error_return;
    }

    /* count has to be one for the demo */
    if ( ntohl(msg->header.count) != 1 ) {
        printk(KERN_WARNING "gaps_ilip_access_write( count: %u ) Invalid\n", ntohl(msg->header.count) );
        goto error_return;
    }
    /* Session 1, level 1, application 1 in minor number 1 */
    switch ( ntohl(msg->header.session) ) {
    case 1:
        /* A hack as we should not really have the session ID attached to the device minor number */
        if( cp->src->mn != 1 ) {
            printk( KERN_WARNING "gaps_ilip_access_write(1) Invalid minor number: Src: %d Dst: %p for session: %.8x\n",
                    cp->src->mn, cp->dst, ntohl(msg->header.session) );
            goto error_return;
        }
/* See https://stackoverflow.com/questions/45349079/how-to-use-attribute-fallthrough-correctly-in-gcc */
#if KERNEL_VERSION(5, 2, 0) >= LINUX_VERSION_CODE
        /* fall through */
#else
        __attribute__((__fallthrough__));
#endif
    case 0xeca51756:
        if ( gaps_ilip_verbose_level >= 8 ) {
        printk( KERN_INFO "gaps_ilip_access_write( Session: %.8x SrcMn: %u, Dst: %p )\n", 
                ntohl(msg->header.session),
                cp->src->mn,
                cp->dst );
        }
        switch ( ntohl(msg->header.message) ) {
        case 1:
            switch ( ntohl(msg->header.data_tag) ) {
            case 1:
            case 3:
                break;
            default:
                goto error_return;
            }
            break;
        case 2:
            switch ( ntohl(msg->header.data_tag) ) {
            case 3:
                break;
            default:
                goto error_return;
            }
            break;
        case 3:
            switch ( ntohl(msg->header.data_tag) ) {
            case 3:
                break;
            default:
                goto error_return;
            }
            break;
        default:
            goto error_return;
        }
        break;
    case 2:
        /* A hack as we should not really have the session ID attached to the device minor number */
        if( cp->src->mn != 3  ) {
            printk( KERN_WARNING "gaps_ilip_access_write(2) Invalid minor number: Src: %d Dst: %p for session: %.8x\n",
                    cp->src->mn, cp->dst, ntohl(msg->header.session) );
            goto error_return;
        }
/* See https://stackoverflow.com/questions/45349079/how-to-use-attribute-fallthrough-correctly-in-gcc */
#if KERNEL_VERSION(5, 2, 0) >= LINUX_VERSION_CODE
        /* fall through */
#else
        __attribute__((__fallthrough__));
#endif
    case 0x67ff90f4:
        if ( gaps_ilip_verbose_level >= 8 ) {
        printk( KERN_INFO "gaps_ilip_access_write( Session: %.8x SrcMn: %u, Dst: %p )\n", 
                ntohl(msg->header.session),
                cp->src->mn,
                cp->dst );
        }
        switch ( ntohl(msg->header.message) ) {
        case 1:
            switch ( ntohl(msg->header.data_tag) ) {
            case 3:
                break;
            default:
                goto error_return;
            }
            break;
        case 2:
            switch ( ntohl(msg->header.data_tag) ) {
            case 2:
            case 3:
                break;
            default:
                goto error_return;
            }
            break;
        case 3:
            switch ( ntohl(msg->header.data_tag) ) {
            case 3:
                break;
            default:
                goto error_return;
            }
            break;
        default:
            goto error_return;
        }
        break;
    case 0x6bb83e13:
        /* establish_session_id( Level: 1 Src: 1, Dst: 2 ): new session ID 0x6bb83e13 */
        switch ( ntohl(msg->header.message) ) {
        case 1:
            switch ( ntohl(msg->header.data_tag) ) {
            case 1:
                break;
            default:
                goto error_return;
            }
            break;
        case 5:
            switch ( ntohl(msg->header.data_tag) ) {
            case 3:
                break;
            default:
                goto error_return;
            }
            break;
        case 6:
            switch ( ntohl(msg->header.data_tag) ) {
            case 4:
                break;
            default:
                goto error_return;
            }
            break;
        default:
            goto error_return;
        }
        break;
    case 0x8127aa5b:
        /* establish_session_id( Level: 2 Src: 2, Dst: 1 ): new session ID 0x8127aa5b */
        switch ( ntohl(msg->header.message) ) {
        case 2:
            switch ( ntohl(msg->header.data_tag) ) {
            case 1:
                break;
            default:
                goto error_return;
            }
            break;
        case 3:
            switch ( ntohl(msg->header.data_tag) ) {
            case 1:
                break;
            default:
                goto error_return;
            }
            break;
        case 4:
            switch ( ntohl(msg->header.data_tag) ) {
            case 2:
                break;
            default:
                goto error_return;
            }
            break;
        default:
            goto error_return;
        }
        break;
    case 0x2c2b8e86:
        /* establish_session_id( Level: 1 Src: 1, Dst: 2, Msg: 1 ): new session ID 0x2c2b8e86 */
        switch ( ntohl(msg->header.message) ) {
        case 1:
            switch ( ntohl(msg->header.data_tag) ) {
            case 1: /* Position Data */
            case 3: /* Track Data */
            case 4: /* Time data */
                break;
            default:
                goto error_return;
            }
            break;
        default:
            break;
        }
        break;
    case 0x442d2490:
        /* establish_session_id( Level: 2 Src: 2, Dst: 1, Msg: 2 ): new session ID 0x442d2490 */
        switch ( ntohl(msg->header.message) ) {
        case 2:
            switch ( ntohl(msg->header.data_tag) ) {
            case 1: /* Position data */
            case 2: /* Polar data */
                break;
            default:
                goto error_return;
            }
            break;
        default:
            break;
        }
        break;
    case 0xbc5a32fb:
        /* establish_session_id( Level: 1 Src: 1, Dst: 2, Msg: 1 ): new session ID 0xbc5a32fb */
        switch ( ntohl(msg->header.message) ) {
        case 1:
            switch ( ntohl(msg->header.data_tag) ) {
            case 2:
            case 5:
                break;
            default:
                goto error_return;
            }
            break;
        default:
            break;
        }
        break;
    case 0x574c9a21:
        /* establish_session_id( Level: 2 Src: 2, Dst: 1, Msg: 2 ): new session ID 0x574c9a21 */
        switch ( ntohl(msg->header.message) ) {
        case 2:
            switch ( ntohl(msg->header.data_tag) ) {
            case 1:
            case 3:
            case 4:
                break;
            default:
                goto error_return;
            }
            break;
        default:
            break;
        }
        break;
    default:
            goto error_return;
            break;
    }

    if ( gaps_ilip_verbose_level > 5 ) {
        printk(KERN_INFO "gaps_ilip_access_write() Msg: %x Minor: %u, Session: %.8x, Message: %u, Data: %u write allowed\n", 
               cp->end_marker, cp->src->mn, 
               ntohl(msg->header.session), ntohl(msg->header.message), ntohl(msg->header.data_tag) );
    }

    rc = true;
    return rc;

error_return:
    printk(KERN_WARNING "gaps_ilip_access_write() Msg: %x Level: %u, Session: %.8x, Message: %u, Data: %u write rejected\n", 
           cp->end_marker, cp->src->mn, 
           ntohl(msg->header.session), ntohl(msg->header.message), ntohl(msg->header.data_tag) );
    return rc;
}

static unsigned int gaps_ilip_get_level_from_minor( struct gaps_ilip_dev *dev, unsigned int mn )
{
    unsigned int level = 0xffffffffu;

    switch ( mn ) {
    case 1:
        level = 0;
        break;
    case 2:
        level = 0;
        break;
    case 3:
        level = 1;
        break;
    case 4:
        level = 1;
        break;
    default:
        if ( dev != NULL ) {
            /* We save the level, but return the level index */
            if ( dev->src_level == 1 ) {
                level = 0;
            } else if( dev->src_level == 2 ) {
                level = 1;
            }
        }
        break;
    }
    return level;
}

static void gaps_ilip_copy( struct work_struct *work )
{
    struct gaps_ilip_copy_workqueue *cp = NULL;
    struct ilip_message *msg = NULL;
    unsigned int end_marker;
    unsigned int level = 0xffffffffu;
    bool do_copy_first_time = false;
    unsigned int write_driver_index;
    unsigned int write_driver_index_saved;
    unsigned int read_driver_index;
    unsigned int read_driver_index_save;
    u64 t;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,19,0) && LINUX_VERSION_CODE < KERNEL_VERSION(5,3,0)
    struct timespec64 ts64;
#endif
    /* Cannot be NULL */
    if ( work == NULL ) {
        printk( KERN_WARNING "gaps_ilip_copy( ) work pointer invalid\n" );
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

    if ( gaps_ilip_verbose_level > 8 ) {
    printk(KERN_INFO "gaps_ilip_copy(  %p )\n", work );
    }
    cp = container_of(work, struct gaps_ilip_copy_workqueue, workqueue );
    if ( gaps_ilip_verbose_level > 8 ) {
    printk(KERN_INFO "gaps_ilip_copy(  %p : EndMarker: %x )\n", cp, cp->end_marker );
    }

    if ( cp->src == NULL ) {
        printk(KERN_WARNING "gaps_ilip_copy( Copy: %p ) Source %p is invalid\n", cp, cp->src );
        return;
    }

    level = gaps_ilip_get_level_from_minor( cp->src, cp->src->mn );

    if ( level == 0xffffffffu ) {
        printk( KERN_WARNING "gaps_ilip_copy(  %p : %x ) level invalid\n", cp, cp->src->mn );
        return;
    } else {
        if ( level >= (GAPS_ILIP_LEVELS) ) {
            printk( KERN_WARNING "gaps_ilip_copy( level_index: %u ) level invalid\n", level );
            return;
        }
        /* Increment the copy write count */
        gaps_ilip_copy_write_count[level] ++;

        /* Post the address to the write completion FIFO */
        if ( gaps_ilip_write_driver_index[level] == gaps_ilip_messages ) {
            //gaps_ilip_write_driver_index[level] = 0;
            write_driver_index = 0;
            /* Allow the write as this is the first time through */
            do_copy_first_time = true;
        } else {
            write_driver_index_saved = gaps_ilip_write_driver_index[level];
            wmb(); /* Not sure about this */
            write_driver_index = (gaps_ilip_write_driver_index[level] + 1)%gaps_ilip_messages;
            //gaps_ilip_write_driver_index[level] = (gaps_ilip_write_driver_index[level] + 1)%gaps_ilip_messages;
            wmb();
        }
        if ( write_driver_index != gaps_ilip_write_user_index[level] || do_copy_first_time == true ) {
            /* There is room in the FIFO either first time or the FIFO is not full */
            do_copy_first_time = false;
            gaps_ilip_write_completions_fifo[level][write_driver_index] = (uintptr_t)cp->src_data;
            if ( gaps_ilip_verbose_level >= 5 ) {
            printk(KERN_INFO "gaps_ilip_copy( FIFO (User: %2u - Driver: %2u) ): %lx write completion posted\n", 
                   gaps_ilip_write_user_index[level], gaps_ilip_write_driver_index[level],
                   gaps_ilip_write_completions_fifo[level][write_driver_index] );
            }
            wmb();
            gaps_ilip_write_driver_index[level] = write_driver_index;
            wmb();
        } else {
            /* reset the index on the failure to place completion in FIFO */
            wmb();
            gaps_ilip_write_driver_index[level] = write_driver_index_saved;
            wmb();
            printk( KERN_WARNING "gaps_ilip_copy(  %p : %x ) write completion FIFO overflow\n", cp, cp->src->mn );
            gaps_ilip_copy_write_reject_count[level] ++;
            return;
        }
    }

#if 1
    /* Start marker has to be correct */
    if ( cp->start_marker != 0x01234567 ) {
        printk( KERN_WARNING "gaps_ilip_copy(  %p : %x ) start marker invalid\n", cp, cp->start_marker );
        gaps_ilip_copy_write_reject_count[level] ++;
        return;
    }

    /* End marker has to be correct */
    if( (end_marker=jenkins_one_at_a_time_hash(cp, offsetof(struct gaps_ilip_copy_workqueue, length )+sizeof(size_t), 0x76543210u )) != cp->end_marker ) {
        printk( KERN_WARNING "gaps_ilip_copy(  %p : %x ): %x  end marker invalid\n", cp, cp->end_marker, end_marker );
        if ( gaps_ilip_verbose_level > 4 ) {
        printk( KERN_INFO "Copy: %p (level: %u)\n",      cp, level );\
        printk( KERN_INFO "      start_marker: %.8x ilip\n",  cp->start_marker );
        printk( KERN_INFO "             minor: %d ilip\n",  cp->read_minor );
        printk( KERN_INFO "               src: %p ilip\n",  cp->src );
        printk( KERN_INFO "               dst: %p ilip\n",  cp->dst );
        printk( KERN_INFO "          src_data: %p ilip\n",  cp->src_data );
        printk( KERN_INFO "          dst_data: %p ilip\n",  cp->dst_data );
        printk( KERN_INFO "        block_size: %lu ilip\n", cp->block_size );
        printk( KERN_INFO "            length: %lu ilip\n", cp->length );
        }
        gaps_ilip_copy_write_reject_count[level] ++;
        return;
    }
#endif

    msg = (struct ilip_message *)cp->src_data;
    if ( gaps_ilip_verbose_level >= 4 ) {
        printk( KERN_INFO "gaps_ilip_copy() Message@ %p ilip\n",      msg );
        printk( KERN_INFO "      session: %.8x ilip\n",  ntohl(msg->header.session) );
        printk( KERN_INFO "      message: %u ilip\n",  ntohl(msg->header.message) );
        printk( KERN_INFO "        count: %u ilip\n",  ntohl(msg->header.count) );
        printk( KERN_INFO "     data_tag: %x ilip\n",  ntohl(msg->header.data_tag) );
        printk( KERN_INFO "    ilip_time: %lx ilip\n", (long unsigned int)msg->time.ilip_time);
        printk( KERN_INFO "   linux_time: %lx ilip\n", (long unsigned int)msg->time.linux_time );
        printk( KERN_INFO "       sizeof: %lx (long unsigned int) ilip\n", sizeof(long unsigned int) );
        printk( KERN_INFO "  data_length: %u ilip\n",  msg->payload.data_length );
    }

    /* Verify the send side can process the message */
    if ( gaps_ilip_access_write(cp, msg) == false ) {
        /* Not allowed write access, so drop the packet */
        printk( KERN_WARNING "ilip Write access denied: %x\n", cp->end_marker );
        gaps_ilip_copy_write_reject_count[level] ++;
        return;
    }

    /* Yes it can so set the ILIP time in the outgoing message */
    msg->time.ilip_time = (uint64_t)t;
    if ( gaps_ilip_verbose_level > 8 ) {
    printk( KERN_INFO "    ilip_time: %lx\n", (long unsigned int)msg->time.ilip_time);
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

    /* cp->read_minor has the read device, associated to the write device */
    if ( gaps_ilip_verbose_level >= 8 ) {
        printk(KERN_INFO "gaps_ilip_copy( level: %u ): Dev index: %u, ReadMinor: %d, Dev@ %p \n", 
               level, 2*(level+1), cp->read_minor, &gaps_ilip_devices[cp->read_minor] );
    }
    /* Get the read device minor number, private device control structure */
    if ( cp->read_minor < 0 || cp->read_minor >= gaps_ilip_total_devices ) {
        printk(KERN_WARNING "gaps_ilip_copy( read minor: %d ) Invalid\n", cp->read_minor );
    } else {
        if ( gaps_ilip_verbose_level >= 5) {
        printk(KERN_WARNING "gaps_ilip_copy( read minor: %d )\n", cp->read_minor );
        }
    }
    cp->dst = &gaps_ilip_devices[cp->read_minor];
    if ( gaps_ilip_verbose_level >= 5) {
        printk(KERN_INFO "gaps_ilip_copy( read minor: %d ) device minor: %u target: %p\n", 
               cp->read_minor, cp->dst->mn, cp->dst->data );
    }
    if ( cp->read_minor != cp->dst->mn ) {
        printk(KERN_WARNING "gaps_ilip_copy( read minor: %d/%u ) Invalid\n", 
               cp->read_minor, cp->dst->mn  );
        return;
    }

    /* Are we allowed to receive this message */
    if ( gaps_ilip_access_read(cp, msg) == false ) {
        /* Not allowed read access, so drop the packet */
        printk( KERN_WARNING "ilip Read access denied: %x\n", cp->end_marker );
        gaps_ilip_copy_read_reject_count[level] ++;
        return;
    }

    /* Read is going to be processed in some manner */
    gaps_ilip_copy_read_count[level]++;

    /* Reset the copy first time as it now applies to the read side of the copy */
    do_copy_first_time = false;

    /**
     *  Is there a read side driver that has initialized the receive
     *  buffers.
     *  
     *  @note This is using the [level_index], i.e. did an
     *       application on the read side open up the driver. This
     *       does not mean that the driver is open on the correct
     *       session, just means some sesion is open.
     */
    if ( gaps_ilip_read_driver_initialized[level] == false ) {
        /* So we just exit, increment the failure count */
        gaps_ilip_copy_read_reject_count[level]++;
        if ( gaps_ilip_verbose_level >= 4 ) {
            printk(KERN_INFO "gaps_ilip_copy( Read level: %u ): Read driver buffers not initialized\n", level );
        }
        return;
    }

    /* has the read section of the driver been opened and buffers allocated , done on first open
       of the specific driver and ILIP session. */
    if( gaps_ilip_open_count[cp->read_minor] == 0 ) {
        printk(KERN_WARNING "gaps_ilip_copy( Read minor: %d ): Read driver session not open\n", 
               cp->read_minor );
        printk(KERN_WARNING "gaps_ilip_copy( Read minor: %d ): Read buffers@ %p\n", 
               cp->read_minor, cp->dst->data );
        if ( cp->dst->data == NULL ) {
            printk(KERN_WARNING "gaps_ilip_copy( Read minor: %d ): Read buffers@ %p Offset: %llu Not initialized\n", 
                   cp->read_minor, cp->dst->data, gaps_ilip_copy_read_offset[level] );
            /* Increment the reject count as there is no place to deliver the data */
        }
        gaps_ilip_copy_read_reject_count[level] ++;
        return;
    }

    /* Get the completion FIFO read side, this is where we determine if the copy to the read side is allowed */
    /* Post the address to the write completion FIFO after the copy */
    /* It is the driver open that sets the gaps_ilip_read_driver_index to gaps_ilip_messages */
    if ( gaps_ilip_read_driver_index[level] == gaps_ilip_messages ) {
        /* First time through, we set the read driver index to zero */
        //gaps_ilip_read_driver_index[level] = 0;
        read_driver_index = 0;
        /* Allow the read copy as this is the first time through */
        do_copy_first_time = true;
    } else {
        /* Save the current state in case there is no room in the FIFO */
        read_driver_index = (gaps_ilip_read_driver_index[level] + 1)%gaps_ilip_messages;
        read_driver_index_save = gaps_ilip_read_driver_index[level];
        /* Location to receive the completion */
        rmb(); /* Not sure about this, will cause read to look at buffer to early */
        //gaps_ilip_read_driver_index[level] = (gaps_ilip_read_driver_index[level] + 1)%gaps_ilip_messages;
        rmb();
    }
    /* Is there room in the read FIFO to accept the message from the writer. First time the read driver
       and the read user will be set to zero but there is a message to be handled. */
    if ( read_driver_index != gaps_ilip_read_user_index[level] || do_copy_first_time == true ) {
        /* There is room in the FIFO either first time of FIFO not full */
        do_copy_first_time = false;
        /* Get the address on the read side of where the message is to be placed */
        cp->dst_data = &cp->dst->data[gaps_ilip_copy_read_offset[level]];
        msg = (struct ilip_message*)cp->dst_data;
        if ( gaps_ilip_verbose_level >= 4 ) {
            printk(KERN_INFO "gaps_ilip_copy( Read buffer offset: %lld ): Read Buffer@ %p DstB@ %p SrcB@ %p Driver: %u, User: %u\n", 
                   gaps_ilip_copy_read_offset[level],
                   &cp->dst->data[gaps_ilip_copy_read_offset[level]], cp->dst_data, cp->src_data,
                   read_driver_index, gaps_ilip_read_user_index[level] );
        }
        if ( gaps_ilip_verbose_level >= 6 ) {
        printk(KERN_INFO "gaps_ilip_copy() memcpy( Dst: %p, Src: %p, Len: %lu ) ...\n", 
               cp->dst_data, cp->src_data, cp->dst->block_size );
        }
        memcpy(cp->dst_data, cp->src_data, cp->dst->block_size );
        if ( gaps_ilip_verbose_level >= 6 ) {
        printk( KERN_INFO "... memcpy() completed\n" );
        }
        /**
         * @todo At this point the data has been copied out of the write side of the driver, so it is really 
         * only at this time we can post to the write completion queue as this is when the buffer is 
         * free and can be returned to the write part of the driver. However this is on the read side of the 
         * copy routine. What we should have is additiona set of bufferes in the copy part of the driver to 
         * represent the storage of the packets in the ILIP hardware first on the send side and then on the 
         * receive side. 
         */
        /* Record delta ILIP time in the receive buffer */
        #if LINUX_VERSION_CODE >= KERNEL_VERSION(5,3,0)
        /* Ubuntu 19.10 */
        msg->time.ilip_time = ktime_get_boottime_ns() - msg->time.ilip_time ;
        #elif LINUX_VERSION_CODE >= KERNEL_VERSION(4,19,0)
        ktime_get_boottime_ts64(&ts64);
        msg->time.ilip_time = ts64.tv_sec * 1000000000 + ts64.tv_nsec - msg->time.ilip_time ;
        #else 
        /* RedHat 7.x */
        msg->time.ilip_time = ktime_get_boot_ns() - msg->time.ilip_time ;
        #endif
        if ( gaps_ilip_verbose_level >= 4 ) {
            printk( KERN_WARNING "gaps_ilip_copy() memcpy completed, detal time assigned: %llu\n", msg->time.ilip_time );
        }
        /* Has an open been called on the user receiver side,  */
        if( gaps_ilip_read_do_read_first_time[level] == true ) {
            /* do this in the read() routine */
            //gaps_ilip_read_do_read_first_time[level] = false;
            /* Why is this here ???, this plus gaps_ilip_read_do_read_first_time allows
               a read when the driver and user indices are the same in the
               read queue, this only happens the first time through on the read side. */
            gaps_ilip_read_user_index[level] = 0;
        }
        /* Address of where we copied the data in the completion FIFO */
        gaps_ilip_read_completions_fifo[level][read_driver_index] = (uintptr_t)cp->dst_data;
        if ( gaps_ilip_verbose_level > 8 ) {
        printk(KERN_INFO "gaps_ilip_copy( FIFO (User: %2u - Driver: %2u) ): %lx read completion posted\n", 
               gaps_ilip_read_user_index[level], gaps_ilip_read_driver_index[level],
               gaps_ilip_read_completions_fifo[level][gaps_ilip_read_driver_index[level]] );
        }
        rmb();
        /* Pointer is valid, so we can bump the driver pointer index to say there is something there */
        gaps_ilip_read_driver_index[level] = read_driver_index;
        rmb();
        if (  cp->dst != NULL ) {
            if ( gaps_ilip_verbose_level >= 6 ) {
                printk(KERN_INFO "gaps_ilip_copy(): Wake any destination readers\n" );
            }
            wake_up_interruptible(&cp->dst->read_wq);
        } else {
            printk( KERN_WARNING "gaps_ilip_copy() destination device structure missing\n" );
        }
        /* Update the offset, wrapping the offset based on the size. */
        if ( gaps_ilip_verbose_level >= 4 ) {
        printk(KERN_INFO "gaps_ilip_copy( Increment read buffer: %lld, Dev@ %p ): Current offset %lld\n", 
               cp->dst->increment, cp->dst, gaps_ilip_copy_read_offset[level] );
        }
        gaps_ilip_copy_read_offset[level] = (gaps_ilip_copy_read_offset[level]+cp->dst->increment)%(loff_t)cp->dst->buffer_size;
        if ( gaps_ilip_verbose_level >= 4 ) {
        printk(KERN_INFO "gaps_ilip_copy( Increment read buffer: %lld, Dev@ %p): Updated offset %lld\n", 
               cp->dst->increment, cp->dst, gaps_ilip_copy_read_offset[level] );
        }
    } else {
        /* reset the index if there is no place to copy the data in the read completion FIFO */
        rmb();
        gaps_ilip_read_driver_index[level] = read_driver_index_save;
        rmb();
        if ( gaps_ilip_verbose_level > 3 ) {
        printk( KERN_WARNING "gaps_ilip_copy(  %p : Minor %u ) read completion/buffer FIFO overflow D: %u U: %u\n", 
                cp, cp->src->mn, gaps_ilip_read_driver_index[level], gaps_ilip_read_user_index[level] );
        }
        gaps_ilip_copy_read_reject_count[level] ++;
        return;
    }

    return;
}

/**
 * @brief Is there data to be read
 * 
 * @author mdesroch (3/17/20)
 * 
 * @param level What level (index) of ILIP context we are running at
 * 
 * @return bool true if there is data to be read and false if no data
 */
static bool gaps_ilip_read_data_available( unsigned int level )
{
    bool rc;

    rc = ((gaps_ilip_read_user_index[level] != gaps_ilip_read_driver_index[level]) || /* OR */
        /* This is the first time and slot zero has stuff in it to read and the read index has been set by the copy routine */
        ((gaps_ilip_read_do_read_first_time[level] == true) && 
         (gaps_ilip_read_user_index[level] < gaps_ilip_messages) && 
         (gaps_ilip_read_completions_fifo[level][gaps_ilip_read_user_index[level]] != (uintptr_t)-1)) );

    return rc;
}

ssize_t 
gaps_ilip_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	struct gaps_ilip_dev *dev = (struct gaps_ilip_dev *)filp->private_data;
	ssize_t retval = 0;
    unsigned int level;
    /* Do we need to create a new session */
    bool create_session_device = false;
    /* New minor device number on session create for root device */
    unsigned int new_minor;
    /* New device structure address for root device session create */
    struct gaps_ilip_dev *new_dev = NULL;
	
    level = gaps_ilip_get_level_from_minor( dev, dev->mn );
    if ( level == 0xffffffffu && dev->mn != 0 ) {
        printk( KERN_WARNING "gaps_ilip_read() Session: %.8x Level invalid for deviceMinor: %u deviceLevel: %u\n", 
                dev->session_id, dev->mn, dev->src_level );
        return -EFAULT;
    }

    if ( gaps_ilip_verbose_level >=8 && dev->mn == 0 ) {
        printk( KERN_INFO "gaps_ilip_read() dev: %p Major: %u Minor: %u\n", dev, dev->mj, dev->mn );
    }

    #if LINUX_VERSION_CODE >= KERNEL_VERSION(5,0,0)
    /* Ubuntu 19.10 */
    if( access_ok(buf,count) == 0 ) {
        printk( KERN_WARNING "gaps_ilip_read() Address: %p invalid\n", buf );
        return -EFAULT;
    }
    if ( gaps_ilip_verbose_level >= 9 && dev->mn != 0 ) {
        printk( KERN_INFO "gaps_ilip_read( Session: %.8x ) Address: %p, Count: %lu, Access: %ld\n", 
                dev->session_id, buf, count, access_ok(buf,count) );
    }
    #else 
    /* RedHat 7.x */
    if( access_ok(VERIFY_WRITE,buf,count) == 0 ) {
        printk( KERN_WARNING "gaps_ilip_read() Address: %p invalid\n", buf );
        return -EFAULT;
    }
    if ( gaps_ilip_verbose_level >= 9 && dev->mn != 0 ) {
        printk( KERN_INFO "gaps_ilip_read( Session: %.8x ) Address: %p, Count: %lu, Access: %ld\n", 
                dev->session_id, buf, count, access_ok(VERIFY_WRITE,buf,count) );
    }
    #endif

	if (mutex_lock_killable(&dev->gaps_ilip_mutex)) {
        printk( KERN_WARNING "gaps_ilip_read() device mutex lock failed\n" );
        if ( level != 0xffffffff ) {
            gaps_ilip_message_read_reject_count[level]++;
        }
		return -EINTR;
    }

    /* limited to the block size, you can read less */
    if (count > dev->block_size) {
        count = dev->block_size;
    }

    if ( gaps_ilip_verbose_level >= 8 && dev->mn == 0 ) {
        printk(KERN_INFO "gaps_ilip_read( copy_to_user() ) Offset: %llu, Buffer length: %lu, Message Length: %lu, Address: %p\n",
            *f_pos, dev->buffer_size, count, &(dev->data[*f_pos]) );
    }

    /* not the root device, blocking allowed */
    if ( dev->mn > 0 ) {
        /* Something in the FIFO queue, copy() increments driver index for new content, we increment
           read_user_index to get that content. The first time we do a read the driver index and the read user index are the
           same and the result, even though there is a message to transfer is the same the user and
           driver indexes are the same and after we read out the message the two indeces are the same indicating
           there is nothing in the FIFO to read. */
        #if 0
        if ((gaps_ilip_read_user_index[level] != gaps_ilip_read_driver_index[level]) || /* OR */
            /* This is the first time and slot zero has stuff in it to read and the read index has been set by the copy routine */
            ((gaps_ilip_read_do_read_first_time[level] == true) && 
             (gaps_ilip_read_user_index[level] < gaps_ilip_messages) && 
             (gaps_ilip_read_completions_fifo[level][gaps_ilip_read_user_index[level]] != (uintptr_t)-1)) ) 
        #else
        #if 0
        if (  gaps_ilip_read_data_available(level) == true )
        #else 
        while (  gaps_ilip_read_data_available(level) == false ) {
            /* Release the mutex */
            mutex_unlock(&dev->gaps_ilip_mutex);
            if (  (filp->f_flags & O_NONBLOCK) != 0 ) {
                return -EAGAIN;
            }
            if ( gaps_ilip_verbose_level >= 6 ) {
                printk(KERN_INFO "gaps_ilip_read( \"%s\" ) [Minor: %2d] Reader going to sleep\n", current->comm, dev->mn );
            }
            if ( wait_event_interruptible( dev->read_wq, (gaps_ilip_read_data_available(level) == true) ) ) {
                return -ERESTARTSYS;
            }
            if (mutex_lock_killable(&dev->gaps_ilip_mutex)) {
                printk( KERN_WARNING "gaps_ilip_read() device mutex lock failed\n" );
                if ( level != 0xffffffff ) {
                    gaps_ilip_message_read_reject_count[level]++;
                }
                return -ERESTARTSYS;
            }
            if ( gaps_ilip_verbose_level >= 6 ) {
                printk(KERN_INFO "gaps_ilip_read( \"%s\" ) [Minor: %2d] Reader was woken up\n", current->comm, dev->mn );
            }
        }
        #endif
        #endif
        {
            /* Increment to the FIFO location when it is not the very first read we have ever done */
            if (gaps_ilip_read_do_read_first_time[level] == false) {
                /* Next entry we are going to read from, the copy() function increments the gaps_ilip_read_driver_index[] */
                gaps_ilip_read_user_index[level] = (gaps_ilip_read_user_index[level] + 1) % gaps_ilip_messages;
            } else {
                /* clear the first read command, from now on we will increment the read offset */
                gaps_ilip_read_do_read_first_time[level] = false;
            }
            if ( gaps_ilip_verbose_level > 8 ) {
                printk(KERN_INFO "gaps_ilip_read( FIFO (User: %2u - Driver: %2u) ): %lx\n",
                       gaps_ilip_read_user_index[level], gaps_ilip_read_driver_index[level],
                       gaps_ilip_read_completions_fifo[level][gaps_ilip_read_user_index[level]]);
            }
            /* copy to user, I should not need the -1 check on the address */
            if ( gaps_ilip_read_completions_fifo[level][gaps_ilip_read_user_index[level]] != (uintptr_t)-1 ) {
                if (copy_to_user(buf, (void *)gaps_ilip_read_completions_fifo[level][gaps_ilip_read_user_index[level]], count) != 0) {
                    retval = -EFAULT;
                    gaps_ilip_message_read_reject_count[level]++;
                    goto out;
                }
                /* Clear the entry in the FIFO */
                gaps_ilip_read_completions_fifo[level][gaps_ilip_read_user_index[level]] = (uintptr_t)-1;
                gaps_ilip_message_read_count[level]++;
            } else{
                if ( gaps_ilip_verbose_level > 1 ) {
                    printk(KERN_WARNING "gaps_ilip_read( FIFO (User: %2u - Driver: %2u) ): UserBuffer@ %p, invalid address\n",
                           gaps_ilip_read_user_index[level], gaps_ilip_read_driver_index[level],
                           (void *)gaps_ilip_read_completions_fifo[level][gaps_ilip_read_user_index[level]] );
                }
            }
        }
        #if 0
        else if ( gaps_ilip_read_do_read_first_time[level] == false ) {
            /* Not first time and nothing to be read, here is where we would sleep  */
            if ( gaps_ilip_verbose_level > 9 ) {
                printk(KERN_WARNING "gaps_ilip_read( FIFO (User: %2u - Driver: %2u) ): %lx, return EAGAIN\n",
                       gaps_ilip_read_user_index[level], gaps_ilip_read_driver_index[level],
                       gaps_ilip_read_completions_fifo[level][gaps_ilip_read_user_index[level]]);
            }
            retval =  -EAGAIN;
            goto out;
        } else {
            /* First time, but other read conditions not met, shoud we also sleep in this case. */
            retval =  -EAGAIN;
            goto out;
        }
        #endif
    } else {
        /**
         * Process the root device read of the session ID from the material 
         * provided in the write part of the session ID calculation. 
         */
        /* Location of the session and application ID in driver buffer */
        uint32_t *session_id_adr = NULL;
        /* Get the application ID, resides */
        uint32_t source_id = 0xffffffff;
        /* Get the destination ID */
        uint32_t destination_id = 0xffffffff;

        /* Root device read write to get the session ID */
        if (*f_pos >= dev->buffer_size) { /* EOF */
            goto out;
        }
        if (*f_pos + count > dev->buffer_size) {
            count = dev->buffer_size - *f_pos;
        }

        /* Only allowed to read 4 bytes from offset 0 in root device */
        if ( *f_pos == 0 && count == sizeof(uint32_t) ) {
            session_id_adr = (uint32_t*)&(dev->data[*f_pos]);
            source_id = session_id_adr[0];
            destination_id = session_id_adr[2];
            if ( gaps_ilip_verbose_level >= 6 ) {
            printk( KERN_INFO "gaps_ilip_read() Session read: Src: %u Dst: %u MsgCount: %u\n", source_id, destination_id, dev->session_message_count );
            }
            /* Record the session ID associated to the source ID - why ??? */
            if ( 0 <= source_id && source_id < GAPS_ILIP_NSESSIONS ) {
                if ( 0 <= destination_id && destination_id < GAPS_ILIP_NSESSIONS ) {
                } else {
                    /* Only allowed to read offset 0 and 32 bits for a limited number of ID values */
                    retval =  -EPERM;
                    goto out;
                }
            } else {
                /* Only allowed to read offset 0 and 32 bits for a limited number of ID values */
                retval =  -EPERM;
                goto out;
            }
            /* Are there messages we have to hash to get the session ID*/
            if (dev->session_message_count != 0) {
                unsigned int hash; 
                size_t len;
                /* compute the session ID, otherwise we just return the application ID written */
                len = (2*sizeof(uint32_t)) + dev->session_message_count*sizeof(uint32_t);
                hash = jenkins_one_at_a_time_hash_init( len, 0xCB2A5AE3 );
                /* Source ID, array of message IDs in the to be hashed buffer */
                hash = jenkins_one_at_a_time_hash_ex(hash, (const unsigned char*)session_id_adr, sizeof(uint32_t));
                /* We skip the level in the determination of the session, then Destination ID and message array */
                len -= (1*sizeof(uint32_t));
                hash = jenkins_one_at_a_time_hash_ex(hash, (const unsigned char*)&session_id_adr[2], len);
                hash = jenkins_one_at_a_time_hash_done( hash );
                /* Set the session ID */
                *session_id_adr = hash;
                /* Need to create the session device */
                create_session_device = gaps_ilip_save_session_id( *session_id_adr );
                if ( gaps_ilip_verbose_level >= 6 ) {
                printk( KERN_INFO "gaps_ilip_save_session_id( %.8x ): %d {Src: %u, Dst: %u, Level: %u}\n", 
                        *session_id_adr, create_session_device, source_id, destination_id, session_id_adr[1] );
                }
            } else {
                uint32_t session_index;
                (void)gaps_ilip_save_session_id( *session_id_adr );
                session_index = gaps_ilip_get_session_index(*session_id_adr);
                if ( session_index == GAPS_ILIP_NSESSIONS) {
                    printk( KERN_WARNING "gaps_ilip_init_session_id( %.8x ): Invalid index\n", *session_id_adr );
                }
                if ( gaps_ilip_verbose_level >= 2 ) {
                printk(KERN_INFO "gaps_ilip_save_session_id( %.8x ): %d {Src: %u, Dst: %u, Level: %u} [index: %u]\n",
                        *session_id_adr, false, source_id, destination_id, session_id_adr[1], session_index );
                }
                if ( *session_id_adr  == 1 ) {
                    /**
                     * @todo need a init session with set commands 
                     */
                    gaps_ilip_sessions[session_index].level_src = 1;
                    gaps_ilip_sessions[session_index].level_dst = 2;
                    gaps_ilip_sessions[session_index].minor_src = 1;
                    gaps_ilip_sessions[session_index].minor_dst = 2;
                } else if ( *session_id_adr  == 2 ) {
                    /**
                     * @todo need a init session with set commands 
                     */
                    gaps_ilip_sessions[session_index].level_dst = 1;
                    gaps_ilip_sessions[session_index].level_src = 2;
                    gaps_ilip_sessions[session_index].minor_dst = 4;
                    gaps_ilip_sessions[session_index].minor_src = 3;
                }else {
                    printk( KERN_WARNING "gaps_ilip_init_session_id( %.8x ): Invalid\n", *session_id_adr );
                }
            }
        } else {
            /* Only allowed to read offset 0 and 32 bits */
            retval =  -EPERM;
            goto out;
        }

        /* Need to create the device */
        if ( create_session_device == true ) {
            /* Loop index for write and then read session device */
            unsigned int i;
            unsigned int session_index;
            session_index = gaps_ilip_get_session_index( *session_id_adr );
            if ( session_index == GAPS_ILIP_NSESSIONS ) {
                /* internal error, should not happen */
                printk(KERN_WARNING "Session ID: %.8x not found, but creation indicated\n", *session_id_adr );
                retval =  -EPERM;
                goto out;
            }
            for (i = 0; i < 2; i++) {
                /* We start using the minor number beyond the standard devices */
                new_minor = i+(session_index*2)+gaps_ilip_ndevices;
                new_dev = &gaps_ilip_devices[new_minor];
                /* Needed here so the device name has the session ID in it */
                new_dev->session_id = *session_id_adr;
                if( gaps_ilip_construct_device(new_dev, new_minor, gaps_ilip_class ) != 0 ) {
                    printk( KERN_WARNING "Create session device [%2u]: %.8x - Failed\n", new_minor, *session_id_adr );
                    *session_id_adr = 0xffffffff;
                    break;
                } else {
                    new_dev->src_level = session_id_adr[1];
                    new_dev->dst_level = (session_id_adr[1]==1)?2:1;
                    gaps_ilip_sessions[session_index].level_src = new_dev->src_level;
                    gaps_ilip_sessions[session_index].level_dst = new_dev->dst_level;
                    /** @bug fix this */
                    if ( ((new_minor+1)%2) == 0 ) {
                        /* Write device, source in this case */
                        gaps_ilip_sessions[session_index].minor_src = new_minor;
                    } else {
                        gaps_ilip_sessions[session_index].minor_dst = new_minor;
                        /* Read device, destination in this case */
                    }
                    if ( gaps_ilip_verbose_level >= 6 ) {
                        printk( KERN_INFO "Create session device [%2u]: Src: %u Dst: %u Session: %.8x srcLevel: %u dstLevel: %u\n", 
                                new_minor, source_id, destination_id, *session_id_adr, new_dev->src_level, new_dev->dst_level );
                    }
                }
            }
            /* Increment the session count if the devices were created ok */
            if ( *session_id_adr == 0xffffffff) {
                /* Reset the session ID here */
                if( gaps_ilip_remove_session_index( new_dev->session_id ) == true ) {
                    new_dev->session_id = 0;
                }
            }
        }

        /* Will just be the application ID if there are no messages */
        if ( session_id_adr != NULL && source_id != 0xffffffff ) {
            if (copy_to_user(buf, (const void *)session_id_adr, count) != 0) {
                retval = -EFAULT;
                goto out;
            }

            if ( gaps_ilip_verbose_level >= 8 ) {
                printk(KERN_INFO "gaps_ilip_read( source ID: %u ): session ID 0x%x\n", source_id, *session_id_adr );
            }
        }

        /* Root device does not move forward */
        if ( dev->increment != 0) {
            /* Always move one block for the reads or the writes */
            *f_pos += dev->increment;
        }
    }
	
	retval = count;
	
out:
    if ( dev->mn == 0 ) {
        /* erase everything in the root device buffer after a read attempt */
        memset( dev->data, 0x0, dev->buffer_size );
    }

	mutex_unlock(&dev->gaps_ilip_mutex);
	return retval;
}
				
ssize_t 
gaps_ilip_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	struct gaps_ilip_dev *dev = (struct gaps_ilip_dev *)filp->private_data;
	ssize_t retval = 0;
    unsigned int wq_index;
    unsigned int level;
    struct gaps_ilip_copy_workqueue *wq = NULL;
    bool do_read_first_time = false;
    unsigned write_user_index;
	
    /* Get the level */
    level = gaps_ilip_get_level_from_minor( dev, dev->mn );
    if ( level == 0xffffffffu && dev->mn != 0 ) {
        printk( KERN_WARNING "gaps_ilip_write() Session: %.8x Level invalid for deviceMinor: %u srcLevel: %u dstLevel: %u\n", 
                dev->session_id, dev->mn, dev->src_level, dev->dst_level );
        return -EFAULT;
    }

    /* increment the message write count */
    if ( level != 0xffffffff && dev->mn != 0 ) {
        gaps_ilip_message_write_count[level]++;
    }

    #if LINUX_VERSION_CODE >= KERNEL_VERSION(5,0,0)
    /* Ubuntu 19.10 */
    if( access_ok(buf,count) == 0 ) {
        printk( KERN_WARNING "gaps_ilip_write() Address: %p invalid\n", buf );
        return -EFAULT;
    }
    #else
    /* RedHat 7.x */
    if( access_ok(VERIFY_READ,buf,count) == 0 ) {
        printk( KERN_WARNING "gaps_ilip_write() Address: %p invalid\n", buf );
        return -EFAULT;
    }
    #endif

	if (mutex_lock_killable(&dev->gaps_ilip_mutex)) {
        printk( KERN_WARNING "gaps_ilip_write() mutex lock failed\n" );
        if ( level != 0xffffffff ) {
            gaps_ilip_message_write_reject_count[level]++;
        }
		return -EINTR;
    }
	
	if (*f_pos >= dev->buffer_size) {
        /* Writing beyond the end of the buffer is not allowed. */
		retval = -ENOSPC;
        printk( KERN_WARNING "gaps_ilip_write() Buffer full\n" );
        if ( level != 0xffffffff ) {
            gaps_ilip_message_write_reject_count[level]++;
        }
		goto out;
	}
	
	if ( gaps_ilip_verbose_level >= 8 ) {
    printk( KERN_WARNING "gaps_ilip_write() File blocking flag: %.8x\n", (filp->f_flags & O_NONBLOCK) );
    }

	if (*f_pos + count > dev->buffer_size) {
		count = dev->buffer_size - *f_pos;
    }
	
	if (count > dev->block_size) {
		count = dev->block_size;
    }
	
    if ( gaps_ilip_verbose_level >= 8 ) {
        printk(KERN_INFO "gaps_ilip_write( copy_from_user() ) "
                         "Offset: %llu, Buffer length: %lu, Message Length: %lu, Address: %p\n",
            *f_pos, dev->buffer_size, count, &(dev->data[*f_pos]) );
    }

    /* Is this a root device write, application ID and messages IDs to be written */
    if ( dev->mn == 0 ) {
        /* Is this the write of the message array associated to this session */
        if ( (*f_pos == (loff_t)12u) && (count != 0) && (count%(sizeof(uint32_t)) == 0) ) {
            dev->session_message_count = count/sizeof(uint32_t);
            if ( gaps_ilip_verbose_level > 6 ) {
                printk(KERN_INFO "gaps_ilip_write( session_messages ) "
                                 "Offset: %llu, Buffer length: %lu, Message Length: %lu, Address: %p\n",
                       *f_pos, dev->buffer_size, count, &(dev->data[*f_pos]) );
            }
        } else if ( (*f_pos == (loff_t)0u) && (count == sizeof(uint32_t) ) ) {
            /* Source ID write */
            dev->source_id = 0;
        } else if ( (*f_pos == (loff_t)4u) && (count == sizeof(uint32_t) ) ) {
            /* Source Level write, We only get the write sides level */
            dev->src_level = 0;
            dev->dst_level = 0;
        } else if ( (*f_pos == (loff_t)8u) && (count == sizeof(uint32_t) ) ) {
            /* Destination ID write */
            dev->destination_id = 0;
        }
    }

    /**
     * How do we know if there is room for this copy, i.e. is this buffer busy with data and if so we need a sleep in 
     * blocking mode so data is not lost. 
     */ 

    /* Copy data from the user buffer */
    if (copy_from_user(&(dev->data[*f_pos]), buf, count) != 0) {
		retval = -EFAULT;
        if ( dev->mn != 0 && level != 0xffffffff ) {
            gaps_ilip_message_write_reject_count[level]++;
        }
		goto out;
	}

    /* Is this a root device write, application and message ID information is
       extracted based on the currect offset the application is writting to.  */
    if ( dev->mn == 0 ) {
        /* Application ID write ? */
        if ( (*f_pos == (loff_t)0u) && (count == sizeof(uint32_t) ) ) {
            uint32_t *app_id = (uint32_t*)&(dev->data[*f_pos]);
            /* Record the application ID */
            dev->source_id = *app_id;
            if ( gaps_ilip_verbose_level > 6 ) {
                printk(KERN_INFO "gaps_ilip_write( source_id ): %x\n", dev->source_id );
            }
        } else if ( (*f_pos == (loff_t)4u) && (count == sizeof(uint32_t) ) ) {
            uint32_t *level_id = (uint32_t*)&(dev->data[*f_pos]);
            /* Record the source level */
            if ( *level_id != 1 && *level_id != 2 ) {
                printk( KERN_WARNING "gaps_ilip_write( session: %.8x ) srcLevel: %u invalid\n",
                        dev->session_id, *level_id );
            }
            switch ( *level_id ) {
            case 1:
                dev->src_level = *level_id;
                dev->dst_level = 2;
                break;
            case 2:
                dev->src_level = *level_id;
                dev->dst_level = 1;
                break;
            default:
                dev->src_level = 0;
                dev->dst_level = 0;
                break;
            }
            if ( gaps_ilip_verbose_level > 6 ) {
                printk(KERN_INFO "gaps_ilip_write( Minor: %u session: %.8x ): srcLevel: %u dstLevel: %u\n", 
                       dev->mn, dev->session_id, dev->src_level, dev->dst_level );
            }
        } else if ( (*f_pos == (loff_t)8u) && (count == sizeof(uint32_t) ) ) {
            uint32_t *dst_id = (uint32_t*)&(dev->data[*f_pos]);
            /* Record the destination ID */
            dev->destination_id = *dst_id;
            if ( gaps_ilip_verbose_level > 6 ) {
                printk(KERN_INFO "gaps_ilip_write( destination_id ): %x\n", dev->destination_id );
            }
        } else if ( (*f_pos == (loff_t)12u) && (count != 0) && (count%(sizeof(uint32_t)) == 0) ) {
            /* messages associated with the application ID */
            uint32_t *msg = (uint32_t*)&(dev->data[*f_pos]);
            unsigned int i;
            if (gaps_ilip_verbose_level > 6) {
                for ( i=0; i < dev->session_message_count; i++, msg++ ) {
                    if ( *msg != 0 ) {
                        printk(KERN_INFO "gaps_ilip_write( session_messages ) Index %u: MessageID: %u\n", i, *msg);
                    }
                }
            }
            /* Record the message ID array address in the buffer */
            dev->message_data_array = msg;
        } else {
            retval = -EPERM;
            goto out;
        }
    }

    /* root device is read write, others are level based */
    if ( dev->mn != 0 /*level != 0xffffffff*/ ) {
        /* Message index we are processing at */
        wq_index = (*f_pos)/gaps_ilip_block_size;
        if ( gaps_ilip_verbose_level >= 8 ) {
        printk( KERN_INFO "gaps_ilip_write( level: %u ) wq_index: %u WorkQueue: %u\n", 
                level+1, wq_index, (level*gaps_ilip_messages)+wq_index );
        }
        /* Get the address of the copy workqueue element we have to fill out */
        /* Need a queue for each level and send and receive */
        wq = &gaps_ilip_queues[(level*gaps_ilip_messages)+wq_index];
        /* Fill out the work request */
        wq->start_marker = 0x01234567;
        /* Read device minor number */
        wq->read_minor = dev->mn+1;
        wq->src = dev;
        wq->dst = NULL;
        wq->src_data = &(dev->data[*f_pos]);
        wq->dst_data = NULL;
        wq->block_size = dev->block_size;
        wq->length = count;
        wq->wq = NULL;
        wq->end_marker = jenkins_one_at_a_time_hash(wq, offsetof(struct gaps_ilip_copy_workqueue, length )+sizeof(size_t), 0x76543210u );
        //wq->end_marker = jenkins_one_at_a_time_hash(wq, sizeof(struct gaps_ilip_copy_workqueue)-sizeof(unsigned int)-sizeof(struct workqueue_struct *)-sizeof(struct work_struct), 0x76543210u );
        if ( gaps_ilip_verbose_level > 8 ) {
        printk(KERN_INFO "gaps_ilip_write( %p : %x ) Message: %p\n", wq, wq->end_marker, wq->src_data );
        }
        #if 1
        /* Schedule the copy to be done */
        if ( gaps_ilip_verbose_level > 8 ) {
        printk(KERN_INFO "gaps_ilip_write( schedule_work( %p ) )\n", &wq->workqueue );
        }
        schedule_work( &wq->workqueue );
        /* Wait for the work to be done, this make the writes synchronous, so there is never a need for the
           writes to sleep as the application cannot exceed the completion FIFO or the incoming buffer as
           we do not return to the caller until the buffer is copied to the read side. */
        flush_scheduled_work();
        if ( gaps_ilip_verbose_level > 8 ) {
        printk(KERN_INFO "gaps_ilip_write( flush_scheduled_work( ) ) completed)\n" );
        }
        #else
        if ( gaps_ilip_verbose_level > 8 ) {
            printk(KERN_INFO "gaps_ilip_write( %p  )\n", &wq->workqueue );
        }
        /* Write side of the work queue, read is add one */
        queue_work( gaps_ilip_wq[level], &wq->workqueue );
        /* Wait for the write work to be done */
        flush_workqueue( gaps_ilip_wq[level] );
        if ( gaps_ilip_verbose_level > 8 ) {
        printk(KERN_INFO "gaps_ilip_write( flush_workqueue( ) ) completed)\n" );
        }
        #endif

        /* Use the completion FIFO for the proper ending condition */
        if ( gaps_ilip_write_user_index[level] == gaps_ilip_messages ) {
            /* Initialize the write index into the FIFO on first time use */
            gaps_ilip_write_user_index[level] = 0;
            write_user_index = 0;
            do_read_first_time = true;
        } else {
            write_user_index = gaps_ilip_write_user_index[level];
        }
        /* On the very first time write is called the user and driver index value will be the same, this only happens once */
        if ( (write_user_index != gaps_ilip_write_driver_index[level]) || 
             ((do_read_first_time == true) && (gaps_ilip_write_completions_fifo[level][write_user_index] != (uintptr_t)-1)) ) {
            /* Increment to the FIFO location */
            if ( do_read_first_time == false ) {
                //gaps_ilip_write_user_index[level] = (gaps_ilip_write_user_index[level] + 1) % gaps_ilip_messages;
                write_user_index = (gaps_ilip_write_user_index[level] + 1) % gaps_ilip_messages;
            }
            if ( gaps_ilip_verbose_level > 8 ) {
            printk(KERN_INFO "gaps_ilip_write( FIFO (User: %2u - Driver: %2u) ): %lx\n", 
                   write_user_index, gaps_ilip_write_driver_index[level],
                   gaps_ilip_write_completions_fifo[level][gaps_ilip_write_user_index[level]] );
            }
            /* Clear the entry in the FIFO */
            gaps_ilip_write_completions_fifo[level][write_user_index] = (uintptr_t)-1;
            wmb();
            gaps_ilip_write_user_index[level] = write_user_index;
            wmb();
        } else {
            if ( gaps_ilip_verbose_level >= 2 ) {
                printk(KERN_WARNING "gaps_ilip_write( level: %u FIFO (User: %2u - Driver: %2u) ): %lx invalid FIFO state\n", level,
                       gaps_ilip_write_user_index[level], gaps_ilip_write_driver_index[level],
                       gaps_ilip_write_completions_fifo[level][gaps_ilip_write_user_index[level]] );
            }
            retval = -EAGAIN;
        }
        //msleep_interruptible( 1u );
    }

    /* Root device does not move forward */
    if ( dev->increment != 0 ) {
        /* Always move one block for the reads or the writes, and wrap on the buffer */
        *f_pos = (*f_pos + dev->increment)%dev->buffer_size;
    }
	retval = count;
	
out:
	mutex_unlock(&dev->gaps_ilip_mutex);
	return retval;
}

loff_t 
gaps_ilip_llseek(struct file *filp, loff_t off, int whence)
{
	struct gaps_ilip_dev *dev = (struct gaps_ilip_dev *)filp->private_data;
	loff_t newpos = 0;

    if ( dev->increment == 0 ) {
        return newpos;
    }

	switch(whence) {
	  case 0: /* SEEK_SET */
		newpos = off;
		break;

	  case 1: /* SEEK_CUR */
		newpos = filp->f_pos + off;
		break;

	  case 2: /* SEEK_END */
		newpos = dev->buffer_size + off;
		break;

	  default: /* can't happen */
		return -EINVAL;
	}
	if (newpos < 0 || newpos > dev->buffer_size) 
		return -EINVAL;
	
	filp->f_pos = newpos;
	return newpos;
}

unsigned int
gaps_ilip_poll( struct file *filp, struct poll_table_struct *wait )
{
    unsigned mask = 0;
	struct gaps_ilip_dev *dev = (struct gaps_ilip_dev *)filp->private_data;
    unsigned int level;

    /* Get the level */
    level = gaps_ilip_get_level_from_minor( dev, dev->mn );

    if (mutex_lock_killable(&dev->gaps_ilip_mutex)) {
        printk( KERN_WARNING "gaps_ilip_poll() device mutex lock failed\n" );
        return -EINTR;
    }

    /* Root device can also be read and written at any time */
    if ( dev->mn == 0 ) {
        mask  = (POLLIN | POLLRDNORM);
        mask |= (POLLOUT | POLLWRNORM);
        goto quick_return;
    }

    poll_wait( filp, &dev->read_wq,  wait );
    poll_wait( filp, &dev->write_wq, wait );

    /* need to look at the read/write flags in file and only return the correct mask */
    if ((filp->f_flags & O_ACCMODE) == O_RDONLY ) {
        /* Read will return if data is available  */
        if (gaps_ilip_read_data_available(gaps_ilip_get_level_from_minor( dev, dev->mn )) == true) {
            if ( gaps_ilip_verbose_level >= 8 ) {
                printk( KERN_INFO "gaps_ilip_poll( Minor: %u, Session: %.8x ) read set\n", dev->mn, dev->session_id );
            }
            mask = (POLLIN | POLLRDNORM);
}
    } else if( (filp->f_flags & O_ACCMODE) == O_WRONLY ) {
        /* Write will return if we can write, this is always true */
        mask = (POLLOUT | POLLWRNORM);
    }

quick_return:
    mutex_unlock(&dev->gaps_ilip_mutex);
    return mask;
}

struct file_operations gaps_ilip_fops = {
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
gaps_ilip_construct_device(struct gaps_ilip_dev *dev, int minor, struct class *class)
{
	int err = 0;
	dev_t devno = MKDEV(gaps_ilip_major, minor);
	struct device *device = NULL;

    int gaps_ilip_mode;
    int gaps_ilip_application;
    char *gaps_ilip_stream = NULL;
	
	BUG_ON(dev == NULL || class == NULL);

	/* Memory is to be allocated when the device is opened the first time */
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

    init_waitqueue_head( &dev->write_wq );
    init_waitqueue_head( &dev->read_wq );

    dev->mj = gaps_ilip_major;
    dev->mn = (unsigned int)minor;
    if ( minor == 0 ) {
        /* Root device does not change the offset on reads and writes */
        dev->increment = 0;
    } else {
        /* We always move reads and writes by the block size */
        dev->increment = GAPS_ILIP_BLOCK_SIZE;
        if ( gaps_ilip_verbose_level > 8 ) {
        printk(KERN_INFO "gaps_ilip_construct_device( minor: %d, Increment : %lld): Dev@ %p\n", 
               minor, dev->increment, dev );
        }
    }

	cdev_init(&dev->cdev, &gaps_ilip_fops);
	dev->cdev.owner = THIS_MODULE;
	
	err = cdev_add(&dev->cdev, devno, 1);
	if (err)
	{
		printk(KERN_WARNING "gaps_ilip_construct_device() Error %d while trying to add %s%d",
			err, GAPS_ILIP_DEVICE_NAME, minor);
		return err;
	}

    if( minor == 0 ) {
        device = device_create(class, NULL, /* no parent device */
            devno, NULL, /* no additional data */
            GAPS_ILIP_DEVICE_NAME "%d_root", minor);
    } else {
        gaps_ilip_application = (minor + 1)/2;
        gaps_ilip_mode = ((minor+1)%2);
        if ( gaps_ilip_mode == 0 ) {
            gaps_ilip_stream = "write";
        } else {
            gaps_ilip_stream = "read";
        }
        /* Simple set of devices for two levels */
        if ( dev->session_id == 0 ) {
            /* Standard devices */
            device = device_create(class, NULL, /* no parent device */
                devno, NULL, /* no additional data */
                GAPS_ILIP_DEVICE_NAME "%d_%s", gaps_ilip_application, gaps_ilip_stream );
        } else {
            /* Session based devices */
            device = device_create(class, NULL, /* no parent device */
                devno, NULL, /* no additional data */
                GAPS_ILIP_DEVICE_NAME "s_%.8x_%s", dev->session_id, gaps_ilip_stream );
            printk( KERN_INFO "gaps_ilip_construct_device() Session device: %s_s_%.8x_%s\n", GAPS_ILIP_DEVICE_NAME, dev->session_id, gaps_ilip_stream );
        }
    }

	if (IS_ERR(device)) {
		err = PTR_ERR(device);
		printk(KERN_WARNING "gaps_ilip_construct_device() Error %d while trying to create %s%d",
			err, GAPS_ILIP_DEVICE_NAME, minor);
		cdev_del(&dev->cdev);
		return err;
	}
	return 0;
}

/* Destroy the device and free its buffer */
static void
gaps_ilip_destroy_device(struct gaps_ilip_dev *dev, int minor, struct class *class)
{
    int i;

    BUG_ON(dev == NULL || class == NULL);
    flush_scheduled_work();
    for (i = 0; i < (sizeof(gaps_ilip_wq)/sizeof(gaps_ilip_wq[0])); ++i) {
        if ( gaps_ilip_wq[i] != NULL ) {
            flush_workqueue(gaps_ilip_wq[i]);
            destroy_workqueue( gaps_ilip_wq[i] );
            gaps_ilip_wq[i] = NULL;
        }
    }
	device_destroy(class, MKDEV(gaps_ilip_major, minor));
	cdev_del(&dev->cdev);
	kfree(dev->data);
	mutex_destroy(&dev->gaps_ilip_mutex);
	return;
}

/* ================================================================ */
static void
gaps_ilip_cleanup_module(int devices_to_destroy)
{
	int i;
	
	/* Get rid of character devices (if any exist) */
	if (gaps_ilip_devices) {
        for (i = 0; i < devices_to_destroy; ++i) {
            if ( i < gaps_ilip_ndevices ) {
                gaps_ilip_destroy_device(&gaps_ilip_devices[i], i, gaps_ilip_class);
            } else if (gaps_ilip_devices[i].session_id != 0 ) {
                gaps_ilip_destroy_device(&gaps_ilip_devices[i], i, gaps_ilip_class);
            }
        }
		kfree(gaps_ilip_devices);
	}
	
	if (gaps_ilip_class)
		class_destroy(gaps_ilip_class);

	/* [NB] gaps_ilip_cleanup_module is never called if alloc_chrdev_region()
	 * has failed. */
	unregister_chrdev_region(MKDEV(gaps_ilip_major, 0), gaps_ilip_total_devices);
	return;
}

static char *gaps_ilip_devnode( struct device *dev, umode_t *mode )
{
    unsigned int mj;
    unsigned int mn;

    if ( !mode || !dev ) {
        return NULL;
    }

    mj = MAJOR(dev->devt);
    mn = MINOR(dev->devt);

    if ( mj != gaps_ilip_major ) {
        return NULL;
    }

    /* Root device need read and write to convert application to session */
    if (dev->devt == MKDEV(gaps_ilip_major, 0)) {
        /* root device to get session ID */
        *mode = 0666;
    } else if( (mn%2) == 1 ) {
        /* Write device */
        *mode = 0222;
    } else if( (mn%2) == 0 ) {
        /* Read device */
        *mode = 0444;
    }
    return NULL;
}

static const char *wq_name_template = "gaps_ilip_wq_%d";

static int __init
gaps_ilip_init_module(void)
{
	int err = 0;
	int i = 0;
	int j = 0;
	int k = 0;
	int devices_to_destroy = 0;
	dev_t dev = 0;
    char wq_name[32];
	
	if (gaps_ilip_total_devices <= 0)
	{
		printk(KERN_WARNING "gaps_ilip_init_module() Invalid value of gaps_ilip_total_devices: %d\n", 
			gaps_ilip_total_devices );
		err = -EINVAL;
		return err;
	}

    err = ilip_nl_init();
    if (err < 0) {
		printk(KERN_WARNING "ilip_nl_init() failed\n");
		return err;
	} else if( gaps_ilip_verbose_level >= 2 ) {
        printk(KERN_INFO "ilip_nl_init() succeeded\n");
    }

	/* Get a range of minor numbers (starting with 0) to work with, include session count */
	err = alloc_chrdev_region(&dev, 0, gaps_ilip_total_devices, GAPS_ILIP_DEVICE_NAME);
	if (err < 0) {
		printk(KERN_WARNING "gaps_ilip_init_module() alloc_chrdev_region() failed\n");
		return err;
	}
	gaps_ilip_major = MAJOR(dev);

	/* Create device class (before allocation of the array of devices) */
	gaps_ilip_class = class_create(THIS_MODULE, GAPS_ILIP_DEVICE_NAME);
	if (IS_ERR(gaps_ilip_class)) {
		err = PTR_ERR(gaps_ilip_class);
		goto fail;
	}

    /* Initialize the ILIP 'level' mutex */
    for ( i = 0; i < gaps_ilip_levels; i++ ) {
        mutex_init(&gaps_ilip_context_mutex[i]);
	}
    
    /* Routine to setup the correct permissions */
    gaps_ilip_class->devnode = gaps_ilip_devnode;
	
	/* Allocate the array of devices, each session has a read and a write minor device */
	gaps_ilip_devices = (struct gaps_ilip_dev *)kzalloc(
		(gaps_ilip_total_devices) * sizeof(struct gaps_ilip_dev), GFP_KERNEL);
	if (gaps_ilip_devices == NULL) {
		err = -ENOMEM;
		goto fail;
	}
    /* Clear out the device array, as we only create the first set of standard devices */
    memset( gaps_ilip_devices, 0x0, (gaps_ilip_total_devices) * sizeof(struct gaps_ilip_dev) );
	
    /* Init session ID array */
    for ( i = 0; i<GAPS_ILIP_NSESSIONS; i++ ) {
        gaps_ilip_sessions[i].session = 0xffffffff;
        gaps_ilip_sessions[i].level_src = 0xffffffff;
        gaps_ilip_sessions[i].level_dst = 0xffffffff;
        gaps_ilip_sessions[i].minor_src = 0xffffffff;
        gaps_ilip_sessions[i].minor_dst = 0xffffffff;
    }
    gaps_ilip_session_count = 0;

/* Construct basic devices, session devices are created when requested in root device access
   and session ID computation. */
	for (i = 0; i < gaps_ilip_ndevices; ++i) {
		err = gaps_ilip_construct_device(&gaps_ilip_devices[i], i, gaps_ilip_class);
		if (err) {
			devices_to_destroy = i;
			goto fail;
		}
	}

    /* Static allocation of message work queues */
	for (i = 0; i < gaps_ilip_levels; ++i) {
        for (j = 0; j < gaps_ilip_messages; ++j) {
            k = (i*gaps_ilip_messages) + j;
            /* create the work queue to do the copy between the write and read instances */
            INIT_WORK( &gaps_ilip_queues[k].workqueue, gaps_ilip_copy );
            gaps_ilip_queues[i].start_marker = 0xffffffff;
            gaps_ilip_queues[i].end_marker   = 0xffffffff;
        }
    }

    /* Create our own work queue for the processing of the messages, has to be single threaded,
       we create a write and read for each level. */
    for (i = 0; i < (sizeof(gaps_ilip_wq)/sizeof(gaps_ilip_wq[0])); ++i) {
        snprintf( wq_name, sizeof(wq_name), wq_name_template, i );
        gaps_ilip_wq[i] = create_singlethread_workqueue(wq_name);
        if (gaps_ilip_wq[i] == NULL) {
            printk( KERN_WARNING "ilip create_singlethread_workqueue(level %d) failed\n", i+1 );
            err = -ENOMEM;
            goto fail;
        }
        if ( gaps_ilip_verbose_level > 2 ) {
            printk( KERN_INFO "ilip create_singlethread_workqueue(level %d)@ %p Name: %s\n", 
                    i+1, gaps_ilip_wq[i], wq_name );
        }
    }

    if ( gaps_ilip_verbose_level > 0 ) {
        printk( KERN_INFO "gaps_ilip_init_module() gaps_ilip_verbose_level = %u\n", gaps_ilip_verbose_level );
        printk( KERN_INFO "gaps_ilip_init_module() gaps_ilip_nt_verbose_level = %u\n", gaps_ilip_nt_verbose_level );
    }

    /* create the work queue to do the copy between the write and read instances */
    INIT_WORK( &gaps_ilip_local_workqueue.workqueue, gaps_ilip_copy );

	return 0; /* success */

fail:
	gaps_ilip_cleanup_module(devices_to_destroy);
	return err;
}

static void __exit
gaps_ilip_exit_module(void)
{
    ilip_nl_exit();
	gaps_ilip_cleanup_module(gaps_ilip_total_devices);
	return;
}

static unsigned int jenkins_one_at_a_time_hash_done( unsigned int hash )
{
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);
    return hash;
}

static unsigned int jenkins_one_at_a_time_hash_ex(unsigned int hash, const unsigned char *key, size_t len)
{
    uint32_t i;
    for(i = 0; i < len; ++i) {
        hash += key[i];
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }
    //hash += (hash << 3);
    //hash ^= (hash >> 11);
    //hash += (hash << 15);
    return hash;
}

static unsigned int jenkins_one_at_a_time_hash_init( size_t len, unsigned int initval)
{
    uint32_t hash;
    hash = 0xdeadbeef + ((unsigned int)len) + initval;
    return hash;
}

static unsigned int jenkins_one_at_a_time_hash(const void *key, size_t len, unsigned int initval)
{
    uint32_t hash;
    hash = 0xdeadbeef + ((unsigned int)len) + initval;
    hash = jenkins_one_at_a_time_hash_ex(hash, key, len );
    hash = jenkins_one_at_a_time_hash_done( hash );
    return hash;
}

module_init(gaps_ilip_init_module);
module_exit(gaps_ilip_exit_module);
/* ================================================================ */
