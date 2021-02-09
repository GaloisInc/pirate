#ifndef _MERCURY_ILIP_COMMON_H
#define _MERCURY_ILIP_COMMON_H

/* ilip_common.h */

/* Derived from cfake.h - implementation of a simple module for a 
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
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
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

#include <linux/cdev.h>
#include <linux/mutex.h>
#include <linux/device.h>
#include <linux/types.h>
#include <linux/dmapool.h>

/* The number of channels the ILIP device supports */
#ifndef GAPS_ILIP_CHANNELS
#define GAPS_ILIP_CHANNELS (4)
#endif

/* The number of sessions we have to handle */
#ifndef GAPS_ILIP_NSESSIONS
#define GAPS_ILIP_NSESSIONS (32)
#endif

/* The number of sessions with PCIe we have to handle */
#ifndef GAPS_ILIP_PCIE_NSESSIONS
#define GAPS_ILIP_PCIE_NSESSIONS (4)
#endif

/* Number of devices to create, two per level one for root */
#ifndef GAPS_ILIP_NDEVICES
#define GAPS_ILIP_NDEVICES ((2*GAPS_ILIP_CHANNELS)+1)    
#endif

/* Total Number of devices to create,
   two per level one for root,
   and two per session in the DCD model */
#ifndef GAPS_ILIP_TOTAL_DEVICES
#define GAPS_ILIP_TOTAL_DEVICES ((2*GAPS_ILIP_NSESSIONS)+(2*GAPS_ILIP_CHANNELS)+1)    
#endif

/* Size of a buffer used for data storage */
#ifndef GAPS_ILIP_BUFFER_SIZE
#define GAPS_ILIP_BUFFER_SIZE (16*1024)
#endif

/* Maxumum length of a block that can be read or written in one operation */
#ifndef GAPS_ILIP_BLOCK_SIZE
#define GAPS_ILIP_BLOCK_SIZE (256)
#endif

/* The number of messages at al level we can handle at a time */
#define GAPS_ILIP_MESSAGE_COUNT (GAPS_ILIP_BUFFER_SIZE/GAPS_ILIP_BLOCK_SIZE)

/* Logging macros */
#define log_info(fmt, args...)  \
  printk(KERN_INFO "%s : " fmt, __FUNCTION__, ##args)

#define log_warn(fmt, args...)  \
  printk(KERN_WARNING "%s : " fmt, __FUNCTION__, ##args)

#define log_err(fmt, args...)  \
  printk(KERN_ERR "%s : " fmt, __FUNCTION__, ##args)

/**
 * @brief structure that hold the kernel virtula address and the DMA PCIe address 
 * of a buffer. 
 * 
 */
typedef struct 
{
  /** Kernel virtual address of buffer */
  char * data;
  /** PCIe address of buffer */
  dma_addr_t handle;
  /** Size of the buffer */
  size_t count;
  /** Level index that will access this buffer */
  unsigned int channel;
  /** Flag indicating completion has been received for buffer */
  volatile int completion_rcvd;
} smb_t;

typedef struct 
{
  char name[128];
  struct dma_pool * handle;
} smb_pool_t;

struct ilip_node 
{
  void * data;

  struct ilip_node * next;
  struct ilip_node * prev;
};


/* The structure to represent 'ilip' devices. 
 *  data - data buffer;
 *  buffer_size - size of the data buffer;
 *  block_size - maximum number of bytes that can be read or written 
 *    in one call;
 *  gaps_ilip_mutex - a mutex to protect the fields of this structure;
 *  cdev - Character device structure.
 */
struct gaps_ilip_dev 
{
  void * device;
  smb_t * smb;
  unsigned char * data;
  unsigned long buffer_size; 
  unsigned long block_size;  
  struct mutex gaps_ilip_mutex; 
  struct cdev cdev;
  loff_t increment;
  unsigned int mj;
  unsigned int mn;
  /* used for session establishment */
  unsigned int session_message_count;
  uint32_t * message_data_array;
  unsigned int src_level;
  unsigned int dst_level;
  unsigned int session_id;
  unsigned int source_id;
  unsigned int destination_id;
  wait_queue_head_t write_wq;
  wait_queue_head_t read_desc_wq;
  wait_queue_head_t read_done_wq;
};

struct gaps_ilip_copy_workqueue 
{
  unsigned int start_marker;
  int read_minor;
  struct gaps_ilip_dev * src;
  struct gaps_ilip_dev * dst;
  smb_t * src_handle;
  smb_t * dst_handle;
  unsigned long block_size;  
  size_t length;
  struct work_struct workqueue;
  struct workqueue_struct * wq;
  unsigned int end_marker;
};

struct ilip_message
{
  union
  {
    struct ilip_message_imm
    {
      struct ilip_header_imm
      {
        uint32_t session_tag;
        uint32_t message_tag;
        uint32_t desc_type;
        uint32_t data_tag;
      } header;

      struct ilip_time_imm
      {
        uint64_t ilip_time;
        uint64_t linux_time;
      } time;

      struct ilip_payload_imm
      {
        uint32_t dest_tag;
        uint32_t imm_data_len;
        uint32_t imm_data[52];
        uint64_t desc_siphash;
      } payload;
    } imm;

    struct ilip_message_pay
    {
      struct ilip_header_pay
      {
        uint32_t session_tag;
        uint32_t message_tag;
        uint32_t desc_type;
        uint32_t desc_tag;
      } header;

      struct ilip_time_pay
      {
        uint64_t ilip_time;
        uint64_t linux_time;
      } time;

      struct ilip_payload_pay
      {
        uint32_t dest_tag;
        uint32_t rsvd_1;
        uint64_t msg_siphash[2];
        uint32_t data_tag;
        uint32_t payload_len;
        uint64_t payload_adr;
        uint32_t rsvd_2[44];
        uint64_t desc_siphash;
      } payload;
    } pay;
  } u;
}__attribute__((packed));

/**
 * @brief NetLink statistics buffer to be returned
 * 
 * @author mdesroch (17 Apr 2020)
 */
struct ilip_session_statistics 
{
  /** Number of messages sent by the host to the ILIP */
  uint32_t send_count;
  /** Number of message received by the host from the ILIP  */
  uint32_t receive_count;
  /** Number of messages rejected by the driver during a write() call */
  uint32_t send_reject_count;
  /** Number of messages rejected by the driver during a read() call   */
  uint32_t receive_reject_count;
  /** Number of messages received by the ILIP for processing from the host */
  uint32_t send_ilip_count;
  /** Number of messages received by the ILIP for processing from the wire */
  uint32_t receive_ilip_count;
  /** Number of messages rejected by the host send ILIP processing */
  uint32_t send_ilip_reject_count;
  /** Number of messages rejected by the wire receive ILIP processing */
  uint32_t receive_ilip_reject_count;
  /** Number of messages received on the cable */
  uint32_t receive_cable_count;
  /** Number of messages dropped due to the cable interface processing */
  uint32_t receive_cable_reject_count;
};

struct ilip_operations 
{
  struct module * owner;
  int (*check_status)(struct gaps_ilip_dev * dev);
  int (*open)(struct gaps_ilip_dev * dev);
  smb_t * (*alloc_buffer)(struct gaps_ilip_dev * dev, unsigned int length);
  smb_t * (*get_buffer)(struct gaps_ilip_dev * dev, unsigned int channel);
  void  (*free_buffer)(struct gaps_ilip_dev * dev, smb_t * handle);
  int (*init)(void);
  int (*get_statistics)(unsigned int mn,
			unsigned int session_id,
			unsigned int session_index,			    
                        unsigned int direction,
			struct ilip_session_statistics * stat);
  int (*get_finalr_statistics)(struct gaps_ilip_dev * dev,
			       unsigned int * good, 
			       unsigned int * bad, 
			       unsigned int * c_good,
			       unsigned int * c_bad,
			       unsigned int level);
  int (*get_finalw_statistics)(struct gaps_ilip_dev * dev,
			       unsigned int * good, 
			       unsigned int * bad, 
			       unsigned int * c_good,
			       unsigned int * c_bad,
			       unsigned int level);
  int (*init_write_counters)(struct gaps_ilip_dev * dev, unsigned int channel);
  int (*init_read_counters)(struct gaps_ilip_dev * dev, unsigned int channel);
  int (*clear_counters)(unsigned int mn, unsigned int direction);
  int (*write)(struct gaps_ilip_dev * dev,
               struct gaps_ilip_copy_workqueue * wq,
	       unsigned int verbosity); 
  int (*read)(struct gaps_ilip_dev * dev,
              char __user * buf,
	      size_t count,
	      unsigned int verbosity); 
  int (*read_init)(struct gaps_ilip_dev * dev, unsigned int channel); 
  bool (*read_data_available)(struct gaps_ilip_dev * dev);
  int (*read_release)(struct gaps_ilip_dev * dev, unsigned int channel); 
  int (*release)(struct gaps_ilip_dev * dev, unsigned int channel); 
};

extern struct ilip_operations iops;

/**
 * @brief Verbose level 
 *  
 * @details  The larger the number the more driver print outs
 * 
 * @author mdesroch (2/7/20)
 */

extern uint gaps_ilip_verbose_level;
extern uint gaps_ilip_nt_verbose_level;

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
extern bool gaps_ilip_read_do_read_first_time[GAPS_ILIP_CHANNELS];

/**
 * @brief Session array, each session will have two minor devices created, one
 *        for read and one for write.
 */
struct ilip_session_info 
{
  unsigned int session;
  unsigned int level_src;
  unsigned int level_dst;
  unsigned int minor_src;
  unsigned int minor_dst;
};

extern unsigned int gaps_ilip_message_write_count[GAPS_ILIP_CHANNELS];
extern unsigned int gaps_ilip_message_read_count[GAPS_ILIP_CHANNELS];
extern unsigned int gaps_ilip_message_write_reject_count[GAPS_ILIP_CHANNELS];
extern unsigned int gaps_ilip_message_read_reject_count[GAPS_ILIP_CHANNELS];

extern unsigned int 
jenkins_one_at_a_time_hash_init
(
  size_t len, 
  unsigned int initval
);

extern unsigned int 
jenkins_one_at_a_time_hash_done
(
  unsigned int hash
);

extern unsigned int 
jenkins_one_at_a_time_hash_ex
(
  unsigned int hash, 
  const unsigned char * key, 
  size_t len
);

extern unsigned int 
jenkins_one_at_a_time_hash
(
  const void * key, 
  size_t len, 
  unsigned int initval
);

extern struct gaps_ilip_copy_workqueue gaps_ilip_queues[GAPS_ILIP_CHANNELS*GAPS_ILIP_MESSAGE_COUNT];

extern struct gaps_ilip_dev *gaps_ilip_devices;

extern void 
_gaps_ilip_destroy_device
(
  void
);

extern struct ilip_session_info gaps_ilip_sessions[GAPS_ILIP_NSESSIONS];

/** 
 * @brief Number of session we have created, DCD requires 7, simple demo does 
 *        not use this
 */ 
extern unsigned int gaps_ilip_session_count;

extern unsigned int 
gaps_ilip_get_session_index
(
  unsigned int session_id
);

extern unsigned int 
gaps_ilip_get_level_from_minor
(
  struct gaps_ilip_dev * dev, 
  unsigned int mn
);

extern unsigned int
gaps_ilip_get_channel_from_minor
(
  struct gaps_ilip_dev * dev,
  unsigned int mn
);

extern bool 
gaps_ilip_save_session_id
(
  unsigned int session_id, 
  unsigned int verbosity
);

extern bool 
gaps_ilip_remove_session_index
(
  unsigned int session_id
);

/**
 * @brief get the verbose level for the netlink section of the ILIP driver
 * 
 * @author mdesroch (3/5/20)
 * 
 * @param void 
 * 
 * @return uint 0 to 10, 0 being quiet.
 */
extern uint 
gaps_ilip_get_nt_verbose_level
(
  void
);

extern int 
_gaps_ilip_check_status
(
  int mn
);
extern int 
_gaps_ilip_open
(
  int mn
);
extern smb_t * 
_gaps_get_buffer
(
  void
);
extern void 
_gaps_free_buffer
(
  smb_t * handle
);
extern int 
_gaps_ilip_init
(
  void
);

/**
 * @brief Is there data to be read
 * 
 * @author mdesroch (3/17/20)
 * 
 * @param mn The minor number of the device. 
 * 
 * @return bool true if there is data to be read and false if no data
 */
extern bool 
_gaps_ilip_read_data_available
(
  unsigned int mn
);

#if 0
extern int gaps_ilip_get_minor(struct ilip_node ** head, spinlock_t * lock);
#endif
extern int 
gaps_ilip_list_add
(
  struct ilip_node ** head, 
  spinlock_t * lock, 
  void * data
);
extern void 
gaps_ilip_list_del
(
  struct ilip_node ** head, 
  spinlock_t * lock, 
  void * data
);
extern void * 
gaps_ilip_list_match_fn
(
  struct ilip_node ** head, 
  spinlock_t * lock, 
  bool (*fn)(void *, void *), 
  void * arg
);
#if 0
extern bool gaps_ilip_minor_match_fn(void * node, void * arg);
extern bool gaps_ilip_session_match_fn(void * node, void * arg);
#endif
extern bool 
gaps_ilip_smb_match_fn
(
  void * node, 
  void * arg
);

#endif /* !defined _MERCURY_ILIP_COMMON_H */
