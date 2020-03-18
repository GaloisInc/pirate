/* gaps_ilip.h */

/* Derived from cfake.h - implementation of a simple module for a character device 
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

#include <linux/cdev.h>
#include <linux/mutex.h>

#ifndef GAPS_ILIP_H_1727_INCLUDED
#define GAPS_ILIP_H_1727_INCLUDED

/* The number of levels we have to handle */
#ifndef GAPS_ILIP_LEVELS
#define GAPS_ILIP_LEVELS (2)
#endif

/* The number of sessionss we have to handle */
#ifndef GAPS_ILIP_NSESSIONS
#define GAPS_ILIP_NSESSIONS (32)
#endif

/* Number of devices to create, two per level one for root */
#ifndef GAPS_ILIP_NDEVICES
#define GAPS_ILIP_NDEVICES ((2*GAPS_ILIP_LEVELS)+1)    
#endif

/* Total Number of devices to create,
   two per level one for root,
   and two per session in the DCD model */
#ifndef GAPS_ILIP_TOTAL_DEVICES
#define GAPS_ILIP_TOTAL_DEVICES ((2*GAPS_ILIP_NSESSIONS)+(2*GAPS_ILIP_LEVELS)+1)    
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

/* The structure to represent 'ilip' devices. 
 *  data - data buffer;
 *  buffer_size - size of the data buffer;
 *  block_size - maximum number of bytes that can be read or written 
 *    in one call;
 *  gaps_ilip_mutex - a mutex to protect the fields of this structure;
 *  cdev - Character device structure.
 */
struct gaps_ilip_dev {
	unsigned char *data;
	unsigned long buffer_size; 
	unsigned long block_size;  
	struct mutex gaps_ilip_mutex; 
	struct cdev cdev;
    loff_t increment;
    unsigned int mj;
    unsigned int mn;
    /* used for session establishment */
    unsigned int session_message_count;
    uint32_t *message_data_array;
    unsigned int src_level;
    unsigned int dst_level;
    unsigned int session_id;
    unsigned int source_id;
    unsigned int destination_id;
};

struct gaps_ilip_copy_workqueue {
    unsigned int start_marker;
    int read_minor;
    struct gaps_ilip_dev *src;
    struct gaps_ilip_dev *dst;
	unsigned char *src_data;
	unsigned char *dst_data;
	unsigned long block_size;  
	size_t length;
    struct work_struct workqueue;
    struct workqueue_struct *wq;
    unsigned int end_marker;
};

struct ilip_header {
    uint32_t session;
    uint32_t message;
    uint32_t count;
    uint32_t data_tag;
};
struct ilip_time {
    uint64_t ilip_time;
    uint64_t linux_time;
};
struct ilip_payload {
    uint32_t data_length;
    uint8_t data[];
};

struct ilip_message {
    struct ilip_header header;
    struct ilip_time time;
    struct ilip_payload payload;
}__attribute__((packed));

struct ilip_session_statistics {
    uint32_t send_count;
    uint32_t receive_count;
    uint32_t send_reject_count;
    uint32_t receive_reject_count;
    uint32_t send_ilip_count;
    uint32_t receive_ilip_count;
    uint32_t send_ilip_reject_count;
    uint32_t receive_ilip_reject_count;
};

/**
 * @brief Find the session ID and return the index
 * 
 * @author mdesroch (3/4/20)
 * 
 * @param session_id  The session ID provided by the root ILIP device
 * 
 * @return unsigned int Session index or GAPS_ILIP_NSESSIONS indicates session 
 *         ID not found.
 */
unsigned int gaps_ilip_get_session_index( unsigned int session_id );
/**
 * @brief Clear or reset the statistics associated to a session
 * 
 * @author mdesroch (3/4/20)
 * 
 * @param session_id The session ID provided by the root ILIP device
 * 
 * @return int  0 on success, -1 on an error
 */
int gaps_ilip_clear_statistics( uint32_t session_id );
/**
 * @brief Get the statistics associated to a session
 * 
 * @author mdesroch (3/4/20)
 * 
 * @param session_id The session ID provided by the root ILIP device
 * @param stat Buffer address to place statistics into
 * 
 * @return int 0 on success, -1 on an error
 */
int gaps_ilip_get_statistics( uint32_t session_id, struct ilip_session_statistics *stat );
/**
 * @brief get the verbose level for the netlink section of the ILIP driver
 * 
 * @author mdesroch (3/5/20)
 * 
 * @param void 
 * 
 * @return uint 0 to 10, 0 being quiet.
 */
uint gaps_ilip_get_nt_verbose_level( void );

#endif /* GAPS_ILIP_H_1727_INCLUDED */
