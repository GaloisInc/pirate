#pragma once

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
 * Portions of this filoe have been copied from:
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

/** ILIP physical function name (no more than 15 characters) */
#define ILIP_NAME_PF		"ilip_pf"
/** ILIP netlink interface version number */
#define ILIP_VERSION		0x1 

/** qdma_dev_attributes structure size */
#define QDMA_DEV_ATTR_STRUCT_SIZE	(16u)

/**
 * @brief Create and initialize the NetLink interface
 * 
 * @author mdesroch (3/2/20)
 * 
 * @param void 
 * 
 * @return int 0 on success otherwise an error occured
 */
int ilip_nl_init(void);
void ilip_nl_exit(void);
