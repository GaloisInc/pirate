#ifndef __PIRATE_CHANNEL_MERCURY_CNTL_H
#define __PIRATE_CHANNEL_MERCURY_CNTL_H

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

/* 
 * Portions of this file have been copied from:
 * 
 * This file is part of the Xilinx DMA IP Core driver for Linux
 *
 * Copyright (c) 2017-2019,  Xilinx, Inc.
 * All rights reserved.
 *
 * This source code is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * The full GNU General Public License is included in this distribution in
 * the file called "COPYING".
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** ILIP physical function name (no more than 15 characters) */
#define ILIP_NAME_PF		"ilip_pf"

/** ILIP netlink interface version number */
#define ILIP_VERSION		0x1 

/** ILIP nl interface minimum response buffer length   */
#define ILIP_NL_RESP_BUFLEN_MIN	 256
/** ILIP nl interface maximum response buffer length   */
#define ILIP_NL_RESP_BUFLEN_MAX	 (2048 * 6)
/** ILIP nl interface error buffer length   */
#define ILIP_NL_ERR_BUFLEN		 64
/** ILIP nl command parameter length   */
#define ILIP_NL_STR_LEN_MAX		 20

#define MAX_KMALLOC_SIZE	(4*1024*1024)

/* Session statistics command - request */
typedef struct {
    unsigned int session_id;
} ilip_cmd_stat_t;

/* Session statistics command - response */
typedef struct {
    uint32_t send_count;
    uint32_t receive_count;
    uint32_t send_reject_count;
    uint32_t receive_reject_count;
    uint32_t send_ilip_count;
    uint32_t receive_ilip_count;
    uint32_t send_ilip_reject_count;
    uint32_t receive_ilip_reject_count;
} mercury_dev_stat_t;


/* Command */
typedef struct {
	unsigned char vf:1;
	unsigned char op:7;

	/* Command request */
	union {	
        ilip_cmd_stat_t stat;           /* Get or clear statistics */
	} req;

	/* Command response */
	union {
		mercury_dev_stat_t stat;	    /* Statistics */
	} resp;
	
	unsigned int if_bdf;			    /* Interface BDF */
} mercury_cmd_t;

/* Commands */
int mercury_cmd_stat(uint32_t session_id, mercury_dev_stat_t *stats);
int mercury_cmd_stat_clear(uint32_t session_id);

#ifdef __cplusplus
}
#endif

#endif /* __PIRATE_CHANNEL_MERCURY_CNTL_H */
