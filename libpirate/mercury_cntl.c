
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

 /* */

#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <linux/genetlink.h>
#include <linux/netlink.h>
#include "gaps_ilip.h"
#include "mercury_cntl.h"

/* netlink message */
typedef struct {
	struct nlmsghdr n;
	struct genlmsghdr g;
} ilip_nl_hdr_t;

typedef struct {
	int fd;
	unsigned short family;
	unsigned int snd_seq;
	unsigned int rcv_seq;
} ilip_nl_ctx_t;

static int ilip_nl_send(ilip_nl_ctx_t *ctx, ilip_nl_hdr_t *hdr) {
	ssize_t rv;
	struct sockaddr_nl addr = {
		.nl_family = AF_NETLINK,
	};

	hdr->n.nlmsg_seq = ctx->snd_seq;
	ctx->snd_seq++;

	rv = sendto(ctx->fd, (char *)hdr, hdr->n.nlmsg_len, 0, 
				(struct sockaddr *)&addr, sizeof(addr));
	if ((unsigned)rv != hdr->n.nlmsg_len) {
		return -1;
	}

	return 0;
}

static int ilip_nl_recv(ilip_nl_ctx_t *ctx, ilip_nl_hdr_t *hdr, int dlen) {
	ssize_t rv;

	memset(hdr, 0, sizeof(ilip_nl_hdr_t) + dlen);

	rv = recv(ctx->fd, hdr, dlen, 0);
	if (rv < 0) {
		return -1;
	}

	/* as long as there is attribute, even if it is shorter than expected */
	if (!NLMSG_OK((&hdr->n), rv) && ((unsigned)rv <= sizeof(ilip_nl_hdr_t))) {
		return -1;
	}

	if (hdr->n.nlmsg_type == NLMSG_ERROR) {
		return -1;
	}

	return 0;
}

static inline ilip_nl_hdr_t *ilip_nl_msg_alloc(unsigned int dlen) {
	ilip_nl_hdr_t *msg;
	size_t alloc_size = sizeof(ilip_nl_hdr_t) + dlen;

	if (!dlen) {
		alloc_size += GAPS_ILIP_ATTR_MAX * (sizeof(struct nlattr) + sizeof(uint32_t));
	}

	msg = (ilip_nl_hdr_t *) malloc(alloc_size);
	if (msg) {
		memset(msg, 0, alloc_size);
	}

	return msg;
}

static int ilip_nl_connect(ilip_nl_ctx_t *ctx) {	
	struct sockaddr_nl addr;
	ilip_nl_hdr_t *hdr = NULL;
	struct nlattr *attr;
	int rv = -1;

	ctx->fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_GENERIC);
	if (ctx->fd < 0) {
		return ctx->fd;
    }

	/* Bind to Netlink socket, destination kernel, unicast */
	memset(&addr, 0, sizeof(addr));
	addr.nl_family = AF_NETLINK;
	rv = bind(ctx->fd, (struct sockaddr *)&addr, sizeof(addr));
	if (rv < 0) {
		goto out;
	}

	hdr = ilip_nl_msg_alloc(0);
	if (hdr == NULL) {
		goto out;
	}

	hdr->n.nlmsg_type = GENL_ID_CTRL;
	hdr->n.nlmsg_flags = NLM_F_REQUEST;
	hdr->n.nlmsg_pid = getpid();
	hdr->n.nlmsg_len = NLMSG_LENGTH(GENL_HDRLEN);

    hdr->g.cmd = CTRL_CMD_GETFAMILY;
    hdr->g.version = ILIP_VERSION;

	attr = (struct nlattr *)(hdr + 1);
	attr->nla_type = CTRL_ATTR_FAMILY_NAME;
	ctx->family = CTRL_ATTR_FAMILY_NAME;

    attr->nla_len = strlen(ILIP_NAME_PF) + 1 + NLA_HDRLEN;
    strcpy((char *)(attr + 1), ILIP_NAME_PF);

    hdr->n.nlmsg_len += NLMSG_ALIGN(attr->nla_len);

	rv = ilip_nl_send(ctx, hdr);
	if (rv < 0) {
		goto out;
	}

	rv = ilip_nl_recv(ctx, hdr, ILIP_NL_RESP_BUFLEN_MIN);
	if (rv < 0) {
		goto out;
	}

	/* family ID */
	attr = (struct nlattr *)((char *)attr + NLA_ALIGN(attr->nla_len));
    if (attr->nla_type == CTRL_ATTR_FAMILY_ID) {
		ctx->family = *(__u16 *)(attr + 1);
	}

	rv = 0;
out:
	if (rv) {
		int err = errno;
		if (ctx->fd > 0) {
			close(ctx->fd);
			ctx->fd = -1;
		}
		errno = err;
	}

	if (hdr) {
		int err = errno;
		free(hdr);
		hdr = NULL;
		errno = err;
	}

	return rv;
}

static void ilip_nl_msg_set_hdr(ilip_nl_hdr_t *hdr, int family, int op) {
	hdr->n.nlmsg_len = NLMSG_LENGTH(GENL_HDRLEN);
	hdr->n.nlmsg_type = family;
	hdr->n.nlmsg_flags = NLM_F_REQUEST;
	hdr->n.nlmsg_pid = getpid();

	hdr->g.cmd = op;
}

static void ilip_nl_msg_add_int_attr(ilip_nl_hdr_t *hdr, 
										enum gaps_ilip_attr_t type,
										unsigned int v) {
	struct nlattr *attr = (struct nlattr *)((char *)hdr + hdr->n.nlmsg_len);

    attr->nla_type = (__u16)type;
    attr->nla_len = sizeof(__u32) + NLA_HDRLEN;
	*(__u32 *)(attr + 1) = v;

    hdr->n.nlmsg_len += NLMSG_ALIGN(attr->nla_len);
}

static int ilip_nl_send_cmd(ilip_nl_ctx_t *ctx, ilip_nl_hdr_t *hdr,
			mercury_cmd_t *cmd) {
	int rv;

	ilip_nl_msg_add_int_attr(hdr, GAPS_ILIP_ATTR_DEV_IDX, cmd->if_bdf);

	switch(cmd->op) {

		case GAPS_ILIP_CMD_DEV_STAT:
        case GAPS_ILIP_CMD_DEV_STAT_CLEAR:
        	/* need the session ID to target the correct device statistics */
			ilip_nl_msg_add_int_attr(hdr, GAPS_ILIP_ATTR_SESSION_ID, cmd->req.stat.session_id);
			break;

	default:
		break;
	}

	rv = ilip_nl_send(ctx, hdr);
	if (rv < 0) {
		goto out;
	}
out:
	return rv;
}

static int ilip_get_cmd_resp_buf_len(enum gaps_ilip_op_t op) {
	int buf_len = GAPS_ILIP_RESP_BUFLEN_MAX;

	switch (op) {
        case GAPS_ILIP_CMD_DEV_LIST:
        case GAPS_ILIP_CMD_DEV_INFO:
        case GAPS_ILIP_CMD_DEV_CAP:
        	return buf_len;
        case GAPS_ILIP_CMD_REG_DUMP:
        	buf_len = GAPS_ILIP_RESP_BUFLEN_MAX * 6;
        break;
        default:
        	buf_len = ILIP_NL_RESP_BUFLEN_MIN;
        	return buf_len;
	}

	if(buf_len > MAX_KMALLOC_SIZE) {
		buf_len = MAX_KMALLOC_SIZE;
    }

	return buf_len;
}

static int ilip_recv_attrs(ilip_nl_hdr_t *hdr, uint32_t *attrs) {
	unsigned char *p = (unsigned char *)(hdr + 1);
	int maxlen = hdr->n.nlmsg_len - NLMSG_LENGTH(GENL_HDRLEN);

	while (maxlen > 0) {
		struct nlattr *na = (struct nlattr *)p;
		int len = NLA_ALIGN(na->nla_len);

		if (na->nla_type >= GAPS_ILIP_ATTR_MAX) {
			return -EINVAL;
		}

		if (na->nla_type != GAPS_ILIP_ATTR_GENMSG) {
			attrs[na->nla_type] = *(uint32_t *)(na + 1);
		}

		p += len;
		maxlen -= len;
	}

	return 0;
}

static int ilip_recv_nl_msg(ilip_nl_hdr_t *hdr, uint32_t *attrs) {
	ilip_recv_attrs(hdr, attrs);

	if (attrs && attrs[GAPS_ILIP_ATTR_ERROR] != 0) {
		return (int)attrs[GAPS_ILIP_ATTR_ERROR];
	}
		
	return 0;
}

static int ilip_nl_parse_response(ilip_nl_ctx_t *ctx, ilip_nl_hdr_t *hdr,
			      unsigned int dlen, uint32_t *attrs) {
	int rv;

	rv = ilip_nl_recv(ctx, hdr, dlen);
	if (rv < 0) {
		return rv;
	}

	return ilip_recv_nl_msg(hdr, attrs);
}

static void ilip_nl_parse_dev_stat_attrs(uint32_t *attrs, mercury_cmd_t *cmd) {
	mercury_dev_stat_t *stats = &cmd->resp.stat;

	stats->send_count = attrs[GAPS_ILIP_ATTR_DEV_STAT_MMH2C_PKTS1];
	stats->receive_count = attrs[GAPS_ILIP_ATTR_DEV_STAT_MMH2C_PKTS2];

	stats->send_reject_count = attrs[GAPS_ILIP_ATTR_DEV_STAT_MMC2H_PKTS1];
	stats->receive_reject_count = attrs[GAPS_ILIP_ATTR_DEV_STAT_MMC2H_PKTS2];

	stats->send_ilip_count = attrs[GAPS_ILIP_ATTR_DEV_STAT_STH2C_PKTS1];
	stats->receive_ilip_count = attrs[GAPS_ILIP_ATTR_DEV_STAT_STH2C_PKTS2];

	stats->send_ilip_reject_count = attrs[GAPS_ILIP_ATTR_DEV_STAT_STC2H_PKTS1];
	stats->receive_ilip_reject_count  = attrs[GAPS_ILIP_ATTR_DEV_STAT_STC2H_PKTS2];
}

static int ilip_nl_parse_cmd_attrs(mercury_cmd_t *cmd, uint32_t *attrs) {
	switch(cmd->op) {

        case GAPS_ILIP_CMD_DEV_STAT:
        	ilip_nl_parse_dev_stat_attrs(attrs, cmd);
			break;

		case GAPS_ILIP_CMD_DEV_STAT_CLEAR:
			break;

	default:
		return -1;
	}

	return 0;
}

static int mercury_cmd(mercury_cmd_t *cmd, uint32_t *attrs) {
	(void) attrs;
	int err;
	int rv = -1;
	ilip_nl_hdr_t *hdr = NULL;
	ilip_nl_ctx_t ctx = { 0 };
	unsigned int dlen;


	rv = ilip_nl_connect(&ctx);
	if (rv < 0) {
		return rv;
	}

	dlen = ilip_get_cmd_resp_buf_len(cmd->op);
	hdr = ilip_nl_msg_alloc(dlen);
	if (!hdr) {
		goto done;
	}
		
	ilip_nl_msg_set_hdr(hdr, ctx.family, cmd->op);
	rv = ilip_nl_send_cmd(&ctx, hdr, cmd);
	if (rv < 0) {
		goto done;
	}
		

	rv = ilip_nl_parse_response(&ctx, hdr, dlen, attrs);
	if (rv < 0) {
		goto done;
    }

	rv = ilip_nl_parse_cmd_attrs(cmd, attrs);
done:
	err = errno;
	if (hdr != NULL) {
		free(hdr);
	}
	
	if (ctx.fd > 0) {
		close(ctx.fd);
		ctx.fd = -1;
	}
	errno = err;
	return rv;
}


int mercury_cmd_stat(uint32_t session_id, mercury_dev_stat_t *stats) {
	int rv = -1;
	uint32_t attrs[GAPS_ILIP_ATTR_MAX] = {0};
	mercury_cmd_t cmd = { 0 };
	cmd.op = GAPS_ILIP_CMD_DEV_STAT;
	cmd.req.stat.session_id = session_id;

	rv = mercury_cmd(&cmd, attrs);
	if (rv == 0) {
		*stats = cmd.resp.stat;
	}

	return rv;
}

int mercury_cmd_stat_clear(uint32_t session_id) {
	uint32_t attrs[GAPS_ILIP_ATTR_MAX] = {0};
	mercury_cmd_t cmd = { 0 };
	cmd.op = GAPS_ILIP_CMD_DEV_STAT_CLEAR;
	cmd.req.stat.session_id = session_id;

	return mercury_cmd(&cmd, attrs);
}
