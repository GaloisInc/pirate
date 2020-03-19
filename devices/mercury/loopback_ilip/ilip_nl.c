
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

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/pci.h>
#include <net/genetlink.h>

#include "include/gaps_ilip.h"
#include "ilip_nl.h"
#include "ilip_base.h"

static int gaps_ilip_dev_list(struct sk_buff *skb_2, struct genl_info *info);

#if KERNEL_VERSION(5, 2, 0) >= LINUX_VERSION_CODE
static struct nla_policy gaps_ilip_policy[GAPS_ILIP_ATTR_MAX] = {
	[GAPS_ILIP_ATTR_GENMSG] =		    { .type = NLA_NUL_STRING },

	[GAPS_ILIP_ATTR_DRV_INFO] =		{ .type = NLA_NUL_STRING },

	[GAPS_ILIP_ATTR_DEV_IDX] =		{ .type = NLA_U32 },
	[GAPS_ILIP_ATTR_PCI_BUS] =		{ .type = NLA_U32 },
	[GAPS_ILIP_ATTR_PCI_DEV] =		{ .type = NLA_U32 },
	[GAPS_ILIP_ATTR_PCI_FUNC] =		{ .type = NLA_U32 },
	[GAPS_ILIP_ATTR_DEV_CFG_BAR] =	{ .type = NLA_U32 },
	[GAPS_ILIP_ATTR_DEV_USR_BAR] =	{ .type = NLA_U32 },
	[GAPS_ILIP_ATTR_DEV_QSET_MAX] =	{ .type = NLA_U32 },
	[GAPS_ILIP_ATTR_DEV_QSET_QBASE] =	{ .type = NLA_U32 },

	[GAPS_ILIP_ATTR_VERSION_INFO] =	{ .type = NLA_NUL_STRING },
	[GAPS_ILIP_ATTR_DEVICE_TYPE]	=	{ .type = NLA_NUL_STRING },
	[GAPS_ILIP_ATTR_IP_TYPE]	=	    { .type = NLA_NUL_STRING },
	[GAPS_ILIP_ATTR_DEV_NUMQS] =          { .type = NLA_U32 },
	[GAPS_ILIP_ATTR_DEV_NUM_PFS] =	{ .type = NLA_U32 },
	[GAPS_ILIP_ATTR_DEV_MM_CHANNEL_MAX] =	{ .type = NLA_U32 },
	[GAPS_ILIP_ATTR_DEV_MAILBOX_ENABLE] = { .type = NLA_U32 },
	[GAPS_ILIP_ATTR_DEV_FLR_PRESENT] =	{ .type = NLA_U32 },
	[GAPS_ILIP_ATTR_DEV_ST_ENABLE] =	{ .type = NLA_U32 },
	[GAPS_ILIP_ATTR_DEV_MM_ENABLE] =	{ .type = NLA_U32 },
	[GAPS_ILIP_ATTR_DEV_MM_CMPT_ENABLE] =	{ .type = NLA_U32 },

	[GAPS_ILIP_ATTR_REG_BAR_NUM] =	{ .type = NLA_U32 },
	[GAPS_ILIP_ATTR_REG_ADDR] =		{ .type = NLA_U32 },
	[GAPS_ILIP_ATTR_REG_VAL] =		{ .type = NLA_U32 },

	[GAPS_ILIP_ATTR_QIDX] =		    { .type = NLA_U32 },
	[GAPS_ILIP_ATTR_NUM_Q] =		    { .type = NLA_U32 },
	[GAPS_ILIP_ATTR_QFLAG] =		    { .type = NLA_U32 },
	[GAPS_ILIP_ATTR_CMPT_DESC_SIZE] =	{ .type = NLA_U32 },
	[GAPS_ILIP_ATTR_SW_DESC_SIZE] =	{ .type = NLA_U32 },
	[GAPS_ILIP_ATTR_QRNGSZ_IDX] =		{ .type = NLA_U32 },
	[GAPS_ILIP_ATTR_C2H_BUFSZ_IDX] =	{ .type = NLA_U32 },
	[GAPS_ILIP_ATTR_CMPT_TIMER_IDX] =	{ .type = NLA_U32 },
	[GAPS_ILIP_ATTR_CMPT_CNTR_IDX] =	{ .type = NLA_U32 },
	[GAPS_ILIP_ATTR_MM_CHANNEL] =		{ .type = NLA_U32 },
	[GAPS_ILIP_ATTR_CMPT_TRIG_MODE] =	{ .type = NLA_U32 },
	[GAPS_ILIP_ATTR_CMPT_ENTRIES_CNT] =	{ .type = NLA_U32 },
	[GAPS_ILIP_ATTR_RANGE_START] =	{ .type = NLA_U32 },
	[GAPS_ILIP_ATTR_RANGE_END] =		{ .type = NLA_U32 },

	[GAPS_ILIP_ATTR_INTR_VECTOR_IDX] =	{ .type = NLA_U32 },
	[GAPS_ILIP_ATTR_PIPE_GL_MAX] =	{ .type = NLA_U32 },
	[GAPS_ILIP_ATTR_PIPE_FLOW_ID] =	{ .type = NLA_U32 },
	[GAPS_ILIP_ATTR_PIPE_SLR_ID] =	{ .type = NLA_U32 },
	[GAPS_ILIP_ATTR_PIPE_TDEST] =		{ .type = NLA_U32 },

	[GAPS_ILIP_ATTR_DEV_STM_BAR] =	{ .type = NLA_U32 },
	[GAPS_ILIP_ATTR_Q_STATE]   =		{ .type = NLA_U32 },
	[GAPS_ILIP_ATTR_ERROR]   =		{ .type = NLA_U32 },
	[GAPS_ILIP_ATTR_DEV]		=	    { .type = NLA_BINARY,
                                  .len = QDMA_DEV_ATTR_STRUCT_SIZE, },
    [GAPS_ILIP_ATTR_SESSION_ID] =		{ .type = NLA_U32 },

#ifdef ERR_DEBUG
	[GAPS_ILIP_ATTR_QPARAM_ERR_INFO] ={ .type = NLA_U32 },
#endif
};
#endif

/**
 * XNL command operation type
 */
static const char *gaps_ilip_op_str[GAPS_ILIP_CMD_MAX] = {
	"DEV_LIST",		/** GAPS_ILIP_CMD_DEV_LIST */
	"DEV_INFO",		/** GAPS_ILIP_CMD_DEV_INFO */
	"DEV_STAT",		/** GAPS_ILIP_CMD_DEV_STAT */
	"DEV_STAT_CLEAR",	/** GAPS_ILIP_CMD_DEV_STAT_CLEAR */

	"REG_DUMP",		/** GAPS_ILIP_CMD_REG_DUMP */
	"REG_READ",		/** GAPS_ILIP_CMD_REG_RD */
	"REG_WRITE",		/** GAPS_ILIP_CMD_REG_WRT */

    "GLOBAL_CSR",		/** GAPS_ILIP_CMD_GLOBAL_CSR */
    "DEV_CAP",		/** GAPS_ILIP_CMD_DEV_CAP */
};

static const char *gaps_ilip_attr_str[GAPS_ILIP_ATTR_MAX + 1] = {
	"GENMSG",		         /**< GAPS_ILIP_ATTR_GENMSG */

	"DRV_INFO",		         /**< GAPS_ILIP_ATTR_DRV_INFO */

	"DEV_IDX",		         /**< GAPS_ILIP_ATTR_DEV_IDX */
	"DEV_PCIBUS",			 /**< GAPS_ILIP_ATTR_PCI_BUS */
	"DEV_PCIDEV",			 /**< GAPS_ILIP_ATTR_PCI_DEV */
	"DEV_PCIFUNC",			 /**< GAPS_ILIP_ATTR_PCI_FUNC */

	"DEV_STAT_MMH2C_PKTS1",	 /**< number of MM H2C packkts */
	"DEV_STAT_MMH2C_PKTS2",	 /**< number of MM H2C packkts */
	"DEV_STAT_MMC2H_PKTS1",	 /**< number of MM C2H packkts */
	"DEV_STAT_MMC2H_PKTS2",	 /**< number of MM C2H packkts */
	"DEV_STAT_STH2C_PKTS1",	 /**< number of ST H2C packkts */
	"DEV_STAT_STH2C_PKTS2",	 /**< number of ST H2C packkts */
	"DEV_STAT_STC2H_PKTS1",	 /**< number of ST C2H packkts */
	"DEV_STAT_STC2H_PKTS2",	 /**< number of ST C2H packkts */

	"DEV_CFG_BAR",			/**< GAPS_ILIP_ATTR_DEV_CFG_BAR */
	"DEV_USR_BAR",			/**< GAPS_ILIP_ATTR_DEV_USER_BAR */
	"DEV_QSETMAX",			/**< GAPS_ILIP_ATTR_DEV_QSET_MAX */
	"DEV_QBASE",			/**< GAPS_ILIP_ATTR_DEV_QSET_QBASE */

	"VERSION_INFO",			/**< GAPS_ILIP_ATTR_VERSION_INFO */
	"DEVICE_TYPE",			/**< GAPS_ILIP_ATTR_DEVICE_TYPE */
	"IP_TYPE",			    /**< GAPS_ILIP_ATTR_IP_TYPE */
	"DEV_NUMQS",			/**<GAPS_ILIP_ATTR_DEV_NUMQS */
	"DEV_NUM_PFS",			/**<GAPS_ILIP_ATTR_DEV_NUM_PFS */
	"DEV_MM_CHANNEL_MAX",	/**<GAPS_ILIP_ATTR_DEV_MM_CHANNEL_MAX */
	"DEV_MAILBOX_ENABLE",	/**<GAPS_ILIP_ATTR_DEV_MAILBOX_ENABLE */
	"DEV_FLR_PRESENT",		/**<GAPS_ILIP_ATTR_DEV_FLR_PRESENT */
	"DEV_ST_ENABLE",		/**<GAPS_ILIP_ATTR_DEV_ST_ENABLE */
	"DEV_MM_ENABLE",		/**<GAPS_ILIP_ATTR_DEV_MM_ENABLE */
	"DEV_MM_CMPT_ENABLE",	/**<GAPS_ILIP_ATTR_DEV_MM_CMPT_ENABLE */

	"REG_BAR",		        /**< GAPS_ILIP_ATTR_REG_BAR_NUM */
	"REG_ADDR",		        /**< GAPS_ILIP_ATTR_REG_ADDR */
	"REG_VAL",		        /**< GAPS_ILIP_ATTR_REG_VAL */

	"CSR_INDEX",			/**< GAPS_ILIP_ATTR_CSR_INDEX*/
	"CSR_COUNT",			/**< GAPS_ILIP_ATTR_CSR_COUNT*/

	"QIDX",			        /**< GAPS_ILIP_ATTR_QIDX */
	"NUM_Q",		        /**< GAPS_ILIP_ATTR_NUM_Q */
	"QFLAG",		        /**< GAPS_ILIP_ATTR_QFLAG */

	"CMPT_DESC_SZ",			/**< GAPS_ILIP_ATTR_CMPT_DESC_SIZE */
	"SW_DESC_SIZE",			/**< GAPS_ILIP_ATTR_SW_DESC_SIZE */
	"QRINGSZ_IDX",			/**< GAPS_ILIP_ATTR_QRNGSZ */
	"C2H_BUFSZ_IDX",		/**< GAPS_ILIP_ATTR_QBUFSZ */
	"CMPT_TIMER_IDX",		/**< GAPS_ILIP_ATTR_CMPT_TIMER_IDX */
	"CMPT_CNTR_IDX",		/**< GAPS_ILIP_ATTR_CMPT_CNTR_IDX */
	"CMPT_TRIG_MODE",		/**< GAPS_ILIP_ATTR_CMPT_TRIG_MODE */
    "MM_CHANNEL",   		/**< GAPS_ILIP_ATTR_MM_CHANNEL */
    "ENTRIES_CNT",		    /**< GAPS_ILIP_ATTR_CMPT_ENTRIES_CNT */

	"RANGE_START",			/**< GAPS_ILIP_ATTR_RANGE_START */
	"RANGE_END",			/**< GAPS_ILIP_ATTR_RANGE_END */

	"INTR_VECTOR_IDX",		/**< GAPS_ILIP_ATTR_INTR_VECTOR_IDX */
	"INTR_VECTOR_START_IDX",/**< GAPS_ILIP_ATTR_INTR_VECTOR_START_IDX */
	"INTR_VECTOR_END_IDX",	/**< GAPS_ILIP_ATTR_INTR_VECTOR_END_IDX */
	"RSP_BUF_LEN",			/**< GAPS_ILIP_ATTR_RSP_BUF_LEN */
	"GLOBAL_CSR",			/**< global csr data */
	"PIPE_GL_MAX",			/**< max no. of gl for pipe */
	"PIPE_FLOW_ID",			/**< pipe flow id */
	"PIPE_SLR_ID",			/**< pipe slr id */
	"PIPE_TDEST",			/**< pipe tdest */
	"DEV_STM_BAR",			/**< device STM bar number */
	"Q_STATE",			    /**< GAPS_ILIP_ATTR_Q_STATE*/
	"ERROR",			    /**< GAPS_ILIP_ATTR_ERROR */
	"DEV_ATTR",			    /**< GAPS_ILIP_ATTR_DEV */
	"SESSION_ID",			/**< GAPS_ILIP_ATTR_SESSION_ID */
#ifdef ERR_DEBUG
	"QPARAM_ERR_INFO",		/**< queue param info */
#endif
	"ATTR_MAX",

};

static int gaps_ilip_dev_info(struct sk_buff *, struct genl_info *);
static int gaps_ilip_dev_version_capabilities(struct sk_buff *skb2, struct genl_info *info);
static int gaps_ilip_dev_stat(struct sk_buff *, struct genl_info *);
static int gaps_ilip_dev_stat_clear(struct sk_buff *, struct genl_info *);
static int gaps_ilip_config_reg_dump(struct sk_buff *, struct genl_info *);
static int gaps_ilip_register_read(struct sk_buff *, struct genl_info *);
static int gaps_ilip_register_write(struct sk_buff *, struct genl_info *);
static int gaps_ilip_get_global_csr(struct sk_buff *skb2, struct genl_info *info);

static struct genl_ops gaps_ilip_ops[] = {
	{
		.cmd = GAPS_ILIP_CMD_DEV_LIST,
#if KERNEL_VERSION(5, 2, 0) > LINUX_VERSION_CODE
		.policy = gaps_ilip_policy,
#endif
		.doit = gaps_ilip_dev_list,
	},
	{
		.cmd = GAPS_ILIP_CMD_DEV_CAP,
#if KERNEL_VERSION(5, 2, 0) > LINUX_VERSION_CODE
		.policy = gaps_ilip_policy,
#endif
		.doit = gaps_ilip_dev_version_capabilities,
	},
	{
		.cmd = GAPS_ILIP_CMD_DEV_INFO,
#if KERNEL_VERSION(5, 2, 0) > LINUX_VERSION_CODE
		.policy = gaps_ilip_policy,
#endif
		.doit = gaps_ilip_dev_info,
	},
	{
		.cmd = GAPS_ILIP_CMD_DEV_STAT,
#if KERNEL_VERSION(5, 2, 0) > LINUX_VERSION_CODE
		.policy = gaps_ilip_policy,
#endif
		.doit = gaps_ilip_dev_stat,
	},
	{
		.cmd = GAPS_ILIP_CMD_DEV_STAT_CLEAR,
#if KERNEL_VERSION(5, 2, 0) > LINUX_VERSION_CODE
		.policy = gaps_ilip_policy,
#endif
		.doit = gaps_ilip_dev_stat_clear,
	},
	{
		.cmd = GAPS_ILIP_CMD_REG_DUMP,
#if KERNEL_VERSION(5, 2, 0) > LINUX_VERSION_CODE
		.policy = gaps_ilip_policy,
#endif
		.doit = gaps_ilip_config_reg_dump,
	},
	{
		.cmd = GAPS_ILIP_CMD_REG_RD,
#if KERNEL_VERSION(5, 2, 0) > LINUX_VERSION_CODE
		.policy = gaps_ilip_policy,
#endif
		.doit = gaps_ilip_register_read,
	},
	{
		.cmd = GAPS_ILIP_CMD_REG_WRT,
#if KERNEL_VERSION(5, 2, 0) > LINUX_VERSION_CODE
		.policy = gaps_ilip_policy,
#endif
		.doit = gaps_ilip_register_write,
	},
	{
		.cmd = GAPS_ILIP_CMD_GLOBAL_CSR,
#if KERNEL_VERSION(5, 2, 0) > LINUX_VERSION_CODE
		.policy = gaps_ilip_policy,
#endif
		.doit = gaps_ilip_get_global_csr,
	},
#ifdef ERR_DEBUG
	{
		.cmd = GAPS_ILIP_CMD_Q_ERR_INDUCE,
#if KERNEL_VERSION(5, 2, 0) > LINUX_VERSION_CODE
		.policy = gaps_ilip_policy,
#endif
		.doit = gaps_ilip_err_induce,
	}
#endif
};

static struct genl_family gaps_ilip_family = {
#ifdef GENL_ID_GENERATE
	.id = GENL_ID_GENERATE,
#endif
	.hdrsize = 0,
	.name = ILIP_NAME_PF,
#ifndef __GENL_REG_FAMILY_OPS_FUNC__
	.ops = gaps_ilip_ops,
	.n_ops = ARRAY_SIZE(gaps_ilip_ops),
#endif
	.maxattr = GAPS_ILIP_ATTR_MAX - 1,
};

static struct sk_buff *gaps_ilip_msg_alloc(enum gaps_ilip_op_t op, int min_sz, 
				void **hdr, struct genl_info *info)
{
	struct sk_buff *skb;
	void *p;
	unsigned long sz = min_sz < NLMSG_GOODSIZE ? NLMSG_GOODSIZE : min_sz;

	skb = genlmsg_new(sz, GFP_KERNEL);
	if (!skb) {
		pr_info("gaps_ilip_msg_alloc() failed to allocate skb %lu.\n", sz);
		return NULL;
	}

	p = genlmsg_put(skb, 0, info->snd_seq + 1, &gaps_ilip_family, 0, op);
	if (!p) {
		pr_info("gaps_ilip_msg_alloc() skb too small.\n");
		nlmsg_free(skb);
		return NULL;
	}

	*hdr = p;
	return skb;
}

static inline int gaps_ilip_msg_add_attr_str(struct sk_buff *skb,
					enum gaps_ilip_attr_t type, char *s)
{
	int rv;

	rv = nla_put_string(skb, type, s);
	if (rv != 0) {
		pr_info("gaps_ilip_msg_add_attr_str() nla_put_string return %d.\n", rv);
		return -EINVAL;
	}
	return 0;
}

static inline int gaps_ilip_msg_add_attr_data(struct sk_buff *skb,
		enum gaps_ilip_attr_t type, void *s, unsigned int len)
{
	int rv;

	rv = nla_put(skb, type, len, s);
	if (rv != 0) {
		pr_info("gaps_ilip_msg_add_attr_data() nla_put return %d.\n", rv);
		return -EINVAL;
	}
	return 0;
}


static inline int gaps_ilip_msg_add_attr_uint(struct sk_buff *skb,
					enum gaps_ilip_attr_t type, u32 v)
{
	int rv;

	rv = nla_put_u32(skb, type, v);
	if (rv != 0) {
		pr_info("gaps_ilip_msg_add_attr_uint() nla add dev_idx failed %d.\n", rv);
		return -EINVAL;
	}
	return 0;
}

static inline int gaps_ilip_msg_send(struct sk_buff *skb_tx, void *hdr,
				struct genl_info *info)
{
	int rv;

	genlmsg_end(skb_tx, hdr);

	rv = genlmsg_unicast(genl_info_net(info), skb_tx, info->snd_portid);
	if (rv)
		pr_info("gaps_ilip_msg_send() send portid %d failed %d.\n", info->snd_portid, rv);

	return 0;
}

static int gaps_ilip_dump_attrs(struct genl_info *info)
{
	int i;

    if ( gaps_ilip_get_nt_verbose_level() >= 7 ) {
        pr_info("gaps_ilip_dump_attrs() snd_seq 0x%x, portid 0x%x.\n",
            info->snd_seq, info->snd_portid);
    }
#if 0
	print_hex_dump_bytes("nlhdr", DUMP_PREFIX_OFFSET, info->nlhdr,
			sizeof(struct nlmsghdr));
	pr_info("\n"); {
	print_hex_dump_bytes("genlhdr", DUMP_PREFIX_OFFSET, info->genlhdr,
			sizeof(struct genlmsghdr));
	pr_info("\n");
#endif

    if ( gaps_ilip_get_nt_verbose_level() >= 7 ) {
    	pr_info("gaps_ilip_dump_attrs() nlhdr: len %u, type %u, flags 0x%x, seq 0x%x, pid %u.\n",
    		info->nlhdr->nlmsg_len,
    		info->nlhdr->nlmsg_type,
    		info->nlhdr->nlmsg_flags,
    		info->nlhdr->nlmsg_seq,
    		info->nlhdr->nlmsg_pid);
    	pr_info("gaps_ilip_dump_attrs() genlhdr: cmd 0x%x %s, version %u, reserved 0x%x.\n",
    		info->genlhdr->cmd, gaps_ilip_op_str[info->genlhdr->cmd],
    		info->genlhdr->version,
    		info->genlhdr->reserved);
    }

	for (i = 0; i < GAPS_ILIP_ATTR_MAX; i++) {
		struct nlattr *na = info->attrs[i];

		if (na) {
#if KERNEL_VERSION(5, 2, 0) > LINUX_VERSION_CODE
			if (gaps_ilip_policy[i].type == NLA_NUL_STRING)
#else
#if 1
			if (1)
#endif
#endif

            /* From one of the two if statements above, prevents SlickEdit from matching braces */
            {
				char *s = (char *)nla_data(na);

				if (s) {

                    if ( gaps_ilip_get_nt_verbose_level() >= 7 ) {
                    #if KERNEL_VERSION(5, 2, 0) > LINUX_VERSION_CODE
					pr_info("gaps_ilip_dump_attrs( type: %d ) attr %d, %s, str %s\n",
                        gaps_ilip_policy[i].type, i, gaps_ilip_attr_str[i], s );
                    #else
                    pr_info("gaps_ilip_dump_attrs(          ) attr %d, %s, str %s\n",
                                                  i, gaps_ilip_attr_str[i], s );
                    #endif
                    }
                } else {
                    if ( gaps_ilip_get_nt_verbose_level() >= 7 ) {
                    #if KERNEL_VERSION(5, 2, 0) > LINUX_VERSION_CODE
					pr_info("gaps_ilip_dump_attrs( type: %d ) attr %d, %s, str NULL\n",
                        gaps_ilip_policy[i].type, i, gaps_ilip_attr_str[i]);
                    #else
                    pr_info("gaps_ilip_dump_attrs(          ) attr %d, %s, str NULL", i, gaps_ilip_attr_str[i]);
                    #endif
                    }
                }

			} else {
				u32 v = nla_get_u32(na);

                if ( gaps_ilip_get_nt_verbose_level() >= 7 ) {
                    #if KERNEL_VERSION(5, 2, 0) > LINUX_VERSION_CODE
                    printk(KERN_INFO "gaps_ilip_dump_attrs( type: %d ) attr %d %s, u32 0x%x. (U32)\n",
                        gaps_ilip_policy[i].type, i, gaps_ilip_attr_str[i], v);
                    #else
                    printk(KERN_INFO "gaps_ilip_dump_attrs(          ) attr %d %s, u32 0x%x. (U32)\n", i, gaps_ilip_attr_str[i], v);
                    #endif
                }
			}
		}
	}

	return 0;
}

static int gaps_ilip_respond_buffer_cmpt(struct genl_info *info, char *buf,
        int buflen, int error, long int cmpt_entries)
{
    struct sk_buff *skb;
    void *hdr;
    int rv;

    skb = gaps_ilip_msg_alloc(info->genlhdr->cmd, buflen, &hdr, info);
    if (!skb) {
        pr_err("gaps_ilip_respond_buffer_cmpt: ENOMEM" );
        return -ENOMEM;
    }

    rv = gaps_ilip_msg_add_attr_str(skb, GAPS_ILIP_ATTR_GENMSG, buf);
    if (rv != 0) {
        pr_err("gaps_ilip_respond_buffer_cmpt: gaps_ilip_msg_add_attr_str() failed: %d", rv);
        nlmsg_free(skb);
        return rv;
    }

    rv = gaps_ilip_msg_add_attr_uint(skb, GAPS_ILIP_ATTR_ERROR, error);
    if (rv != 0) {
        pr_err("gaps_ilip_respond_buffer_cmpt: gaps_ilip_msg_add_attr_str() failed: %d", rv);
        nlmsg_free(skb);
        return rv;
    }

    rv = gaps_ilip_msg_add_attr_uint(skb, GAPS_ILIP_ATTR_CMPT_ENTRIES_CNT,
            cmpt_entries);
    if (rv != 0) {
        pr_err("gaps_ilip_respond_buffer_cmpt: gaps_ilip_msg_add_attr_str() failed: %d", rv);
        nlmsg_free(skb);
        return rv;
    }

    rv = gaps_ilip_msg_send(skb, hdr, info);
    if ( rv != 0 ) {
        pr_err("gaps_ilip_respond_buffer_cmpt: gaps_ilip_msg_send() failed: %d", rv);
    }

    return rv;
}

int gaps_ilip_respond_buffer(struct genl_info *info, char *buf, int buflen, int error)
{
    struct sk_buff *skb;
    void *hdr;
    int rv;

    skb = gaps_ilip_msg_alloc(info->genlhdr->cmd, buflen, &hdr, info);
    if (!skb) {
        pr_err("gaps_ilip_respond_buffer: ENOMEM" );
        return -ENOMEM;
    }

    rv = gaps_ilip_msg_add_attr_str(skb, GAPS_ILIP_ATTR_GENMSG, buf);
    if (rv != 0) {
        pr_err("gaps_ilip_respond_buffer: gaps_ilip_msg_add_attr_str() failed: %d", rv);
        nlmsg_free(skb);
        return rv;
    }

    rv = gaps_ilip_msg_add_attr_uint(skb, GAPS_ILIP_ATTR_ERROR, error);
    if (rv != 0) {
        pr_err("gaps_ilip_respond_buffer: gaps_ilip_msg_add_attr_str() failed: %d", rv);
        nlmsg_free(skb);
        return rv;
    }

    rv = gaps_ilip_msg_send(skb, hdr, info);
    if ( rv != 0 ) {
        pr_err("gaps_ilip_respond_buffer: gaps_ilip_msg_send() failed: %d", rv);
    }

    return rv;
}

static int gaps_ilip_respond_data(struct genl_info *info, void *buf, int buflen)
{
    struct sk_buff *skb;
    void *hdr;
    int rv;

    skb = gaps_ilip_msg_alloc(info->genlhdr->cmd, buflen, &hdr, info);
    if (!skb) {
        pr_err("gaps_ilip_respond_data: ENOMEM" );
        return -ENOMEM;
    }

    rv = gaps_ilip_msg_add_attr_data(skb, GAPS_ILIP_ATTR_GLOBAL_CSR, buf, buflen);
    if (rv != 0) {
        pr_err("gaps_ilip_respond_data: gaps_ilip_msg_add_attr_data() failed: %d", rv);
        return rv;
    }

    rv = gaps_ilip_msg_send(skb, hdr, info);
    if ( rv != 0 ) {
        pr_err("gaps_ilip_respond_data: gaps_ilip_msg_send() failed: %d", rv);
    }

    return rv;
}

static char *gaps_ilip_mem_alloc(int l, struct genl_info *info)
{
    char ebuf[GAPS_ILIP_ERR_BUFLEN];
    char *buf = kmalloc(l, GFP_KERNEL);
    int rv;

    if ( gaps_ilip_get_nt_verbose_level() >= 5 ) {
    printk(KERN_INFO "gaps_ilip_mem_alloc( Len: %d, Info: %p )\n", l, info );
    }
    if (buf) {
        memset(buf, 0, l);
        return buf;
    }

    printk(KERN_WARNING "gaps_ilip_mem_alloc() xnl OOM %d. memory allocation failed\n", l);

    rv = snprintf(ebuf, GAPS_ILIP_ERR_BUFLEN, "gaps_ilip_mem_alloc() ERR! xnl OOM %d.\n", l);

    gaps_ilip_respond_buffer(info, ebuf, GAPS_ILIP_ERR_BUFLEN, rv);

    return NULL;
}

static int xpdev_list_dump(char *buf, int buflen)
{
    return 0;
}

static int gaps_ilip_dev_list(struct sk_buff *skb2, struct genl_info *info)
{
	char *buf;
	int rv;

	if (info == NULL)
		return -EINVAL;

	gaps_ilip_dump_attrs(info);

	buf = gaps_ilip_mem_alloc(GAPS_ILIP_RESP_BUFLEN_MAX, info);
	if (!buf) {
		pr_err("gaps_ilip_dev_list: ENOMEM" );
		return -ENOMEM;
    }

	rv = xpdev_list_dump(buf, GAPS_ILIP_RESP_BUFLEN_MAX);
	if (rv < 0) {
		pr_err("gaps_ilip_dev_list: xpdev_list_dump() failed: %d", rv);
		goto free_msg_buff;
	}

    rv = gaps_ilip_respond_buffer(info, buf, strlen(buf), rv);
    if ( rv < 0 ) {
		pr_err("gaps_ilip_dev_list: gaps_ilip_respond_buffer() failed: %d", rv);
    }

free_msg_buff:
	kfree(buf);
	return rv;
}

static int gaps_ilip_dev_version_capabilities(struct sk_buff *skb2,
		struct genl_info *info)
{
    int rv = -EINVAL;

    if (info == NULL) {
        return -EINVAL;
    }

	gaps_ilip_dump_attrs(info);

    pr_err("gaps_ilip_dev_version_capabilities: failed: %d", rv);

    return rv;
}

static uint32_t gaps_ilip_rcv_check_session(struct genl_info *info)
{
    uint32_t session_id = 0xffffffff;
    char err[GAPS_ILIP_ERR_BUFLEN];
    int rv;

    if (info == NULL) {
        return 0xffffffff;
    }

    if (!info->attrs[GAPS_ILIP_ATTR_SESSION_ID]) {
        snprintf(err, sizeof(err),
            "gaps_ilip_rcv_check_session() command %s missing attribute GAPS_ILIP_ATTR_SESSION_ID",
            gaps_ilip_op_str[info->genlhdr->cmd]);
        rv = -EINVAL;
        goto respond_error;
    }

	session_id = nla_get_u32(info->attrs[GAPS_ILIP_ATTR_SESSION_ID]);

    return session_id;

respond_error:
	gaps_ilip_respond_buffer(info, err, strlen(err), rv);
	return 0xffffffff;

}

static int gaps_ilip_dev_stat(struct sk_buff *skb2, struct genl_info *info)
{
    int rv = -EINVAL;
    uint32_t session_id;
    struct ilip_session_statistics stats;
    struct sk_buff *skb;
    void *hdr;

    if (info == NULL) {
        return -EINVAL;
    }

    gaps_ilip_dump_attrs(info);

    printk( KERN_INFO "");

    skb = gaps_ilip_msg_alloc(GAPS_ILIP_CMD_DEV_STAT, 0, &hdr, info);
    if (!skb) {
        printk( KERN_ERR "gaps_ilip_dev_stat: skb memory allocation failed\n" );
        return -ENOMEM;
    }

    session_id = gaps_ilip_rcv_check_session( info );
    if ( session_id == 0xffffffff ) {
        printk( KERN_ERR "gaps_ilip_dev_stat: failed, illegal session ID\n" );
        return rv;
    } else {
        if ( gaps_ilip_get_nt_verbose_level() >= 4 ) {
            printk( KERN_INFO "gaps_ilip_dev_stat: session ID: %.8x\n", session_id );
        }
        memset( &stats, 0x0, sizeof(stats) );
        if( gaps_ilip_get_statistics( session_id, &stats ) != 0 ) {
            printk( KERN_WARNING "gaps_ilip_dev_stat() Failed for session ID: %.8x\n", session_id );
            return rv;
        } else {
            if ( gaps_ilip_get_nt_verbose_level() >= 5 ) {
                printk(KERN_INFO "gaps_ilip_dev_stat( Session: %.8x ):\n", session_id);
                printk( KERN_INFO "                   send: %u ( ilip )\n", stats.send_count );
                printk( KERN_INFO "                receive: %u ( ilip )\n", stats.receive_count );
                printk( KERN_INFO "            send reject: %u ( ilip )\n", stats.send_reject_count );
                printk( KERN_INFO "         receive reject: %u ( ilip )\n", stats.receive_reject_count );
                printk( KERN_INFO "              send ilip: %u ( ilip )\n", stats.send_ilip_count );
                printk( KERN_INFO "           receive ilip: %u ( ilip )\n", stats.receive_ilip_count );
                printk( KERN_INFO "       send ilip reject: %u ( ilip )\n", stats.send_ilip_reject_count );
                printk( KERN_INFO "    receive ilip reject: %u ( ilip )\n", stats.receive_ilip_reject_count );
            }
        }
    }

    rv = gaps_ilip_msg_add_attr_uint(skb, GAPS_ILIP_ATTR_DEV_STAT_MMH2C_PKTS1, stats.send_count );
    if (rv < 0) {
        pr_err("gaps_ilip_dev_stat() gaps_ilip_msg_add_attr_uint() failed: %d", rv);
        return rv;
    }
    rv = gaps_ilip_msg_add_attr_uint(skb, GAPS_ILIP_ATTR_DEV_STAT_MMH2C_PKTS2, stats.receive_count );
    if (rv < 0) {
        pr_err("gaps_ilip_dev_stat() gaps_ilip_msg_add_attr_uint() failed: %d", rv);
        return rv;
    }
    rv = gaps_ilip_msg_add_attr_uint(skb, GAPS_ILIP_ATTR_DEV_STAT_MMC2H_PKTS1, stats.send_reject_count );
    if (rv < 0) {
        pr_err("gaps_ilip_dev_stat() gaps_ilip_msg_add_attr_uint() failed: %d", rv);
        return rv;
    }
    rv = gaps_ilip_msg_add_attr_uint(skb, GAPS_ILIP_ATTR_DEV_STAT_MMC2H_PKTS2, stats.receive_reject_count );
    if (rv < 0) {
        pr_err("gaps_ilip_dev_stat() gaps_ilip_msg_add_attr_uint() failed: %d", rv);
        return rv;
    }
    rv = gaps_ilip_msg_add_attr_uint(skb, GAPS_ILIP_ATTR_DEV_STAT_STH2C_PKTS1, stats.send_ilip_count );
    if (rv < 0) {
        pr_err("gaps_ilip_dev_stat() gaps_ilip_msg_add_attr_uint() failed: %d", rv);
        return rv;
    }
    rv = gaps_ilip_msg_add_attr_uint(skb, GAPS_ILIP_ATTR_DEV_STAT_STH2C_PKTS2, stats.receive_ilip_count );
    if (rv < 0) {
        pr_err("gaps_ilip_dev_stat() gaps_ilip_msg_add_attr_uint() failed: %d", rv);
        return rv;
    }
    rv = gaps_ilip_msg_add_attr_uint(skb, GAPS_ILIP_ATTR_DEV_STAT_STC2H_PKTS1, stats.send_ilip_reject_count );
    if (rv < 0) {
        pr_err("gaps_ilip_dev_stat() gaps_ilip_msg_add_attr_uint() failed: %d", rv);
        return rv;
    }
    rv = gaps_ilip_msg_add_attr_uint(skb, GAPS_ILIP_ATTR_DEV_STAT_STC2H_PKTS2, stats.receive_ilip_reject_count );
    if (rv < 0) {
        pr_err("gaps_ilip_dev_stat() gaps_ilip_msg_add_attr_uint() failed: %d", rv);
        return rv;
    }

	rv = gaps_ilip_msg_send(skb, hdr, info);
    if ( rv < 0 ) {
        pr_err("gaps_ilip_dev_stat() gaps_ilip_msg_send() failed: %d", rv);
    }

    return rv;

}

static int gaps_ilip_dev_stat_clear(struct sk_buff *skb2, struct genl_info *info)
{
    int rv = -EINVAL;
    uint32_t session_id;
	char *buf;

    if (info == NULL) {
        return -EINVAL;
    }

    if ( gaps_ilip_get_nt_verbose_level() >= 7 ) {
        printk(KERN_INFO "gaps_ilip_dev_stat_clear: Called\n");
    }

    gaps_ilip_dump_attrs(info);

    session_id = gaps_ilip_rcv_check_session( info );
    if ( session_id == 0xffffffff ) {
        printk( KERN_ERR "gaps_ilip_dev_stat_clear: failed, illegal session ID (%d)\n", rv );
        return rv;
    } else {
        if ( gaps_ilip_get_nt_verbose_level() >= 4 ) {
            printk( KERN_INFO "gaps_ilip_dev_stat_clear: session ID: %.8x\n", session_id );
        }
        if( gaps_ilip_clear_statistics( session_id ) != 0 ) {
            printk( KERN_WARNING "gaps_ilip_dev_stat_clear() Failed for session ID: %.8x\n", session_id );
            return rv;
        }
    }

    if ( gaps_ilip_get_nt_verbose_level() >= 5 ) {
    printk( KERN_INFO "gaps_ilip_dev_stat_clear( Min: %u Info: n%p\n", GAPS_ILIP_RESP_BUFLEN_MIN, info );
    }

    buf = gaps_ilip_mem_alloc(GAPS_ILIP_RESP_BUFLEN_MIN, info);
    if (!buf) {
        printk( KERN_ERR "gaps_ilip_dev_stat_clear: memory allocation failed\n" );
        return -ENOMEM;
    }

	buf[0] = '\0';

    rv = 0;

	rv = gaps_ilip_respond_buffer(info, buf, GAPS_ILIP_RESP_BUFLEN_MAX, rv );

    return rv;
}


static int gaps_ilip_config_reg_dump(struct sk_buff *skb2, struct genl_info *info)
{
    int rv = -EINVAL;

    if (info == NULL) {
        return -EINVAL;
    }

    gaps_ilip_dump_attrs(info);

    pr_err("gaps_ilip_config_reg_dump: failed: %d", rv);

    return rv;
}


static int gaps_ilip_register_read(struct sk_buff *skb2, struct genl_info *info)
{
    int rv = -EINVAL;

    if (info == NULL) {
        return -EINVAL;
    }

    gaps_ilip_dump_attrs(info);

    pr_err("gaps_ilip_register_read: failed: %d", rv);

    return rv;

}


static int gaps_ilip_register_write(struct sk_buff *skb2, struct genl_info *info)
{
    int rv = -EINVAL;

    if (info == NULL) {
        return -EINVAL;
    }

    gaps_ilip_dump_attrs(info);

    pr_err("gaps_ilip_register_write: failed: %d", rv);

    return rv;

}


static int gaps_ilip_get_global_csr(struct sk_buff *skb2, struct genl_info *info)
{
    int rv = -EINVAL;

    if (info == NULL) {
        return -EINVAL;
    }

    gaps_ilip_dump_attrs(info);

    pr_err("gaps_ilip_get_global_csr: failed: %d", rv);

    return rv;

}

static int gaps_ilip_dev_info(struct sk_buff *skb2, struct genl_info *info)
{
    int rv = -EINVAL;

    if (info == NULL) {
        return -EINVAL;
    }

    gaps_ilip_dump_attrs(info);

    pr_err("gaps_ilip_dev_info: failed: %d", rv );

    return rv;
}


int ilip_nl_init(void)
{
	int rv;

    (void)gaps_ilip_respond_buffer_cmpt;
    (void)gaps_ilip_respond_data;
    (void)gaps_ilip_op_str;
    (void)gaps_ilip_attr_str;

    (void)gaps_ilip_dev_info;
    (void)gaps_ilip_dev_version_capabilities;
    (void)gaps_ilip_dev_stat;
    (void)gaps_ilip_dev_stat_clear;
    (void)gaps_ilip_config_reg_dump;
    (void)gaps_ilip_register_read;
    (void)gaps_ilip_register_write;
    (void)gaps_ilip_get_global_csr;


#ifdef __GENL_REG_FAMILY_OPS_FUNC__
	rv = genl_register_family_with_ops(&gaps_ilip_family,
			gaps_ilip_ops, ARRAY_SIZE(gaps_ilip_ops));
#else
	rv = genl_register_family(&gaps_ilip_family);
#endif
	if (rv) {
		printk(KERN_WARNING "ilip_nl_init() genl_register_family failed: %d.\n", rv);
    }

	return rv;
}

void  ilip_nl_exit(void)
{
	int rv;

	rv = genl_unregister_family(&gaps_ilip_family);
	if (rv)
		pr_info("genl_unregister_family failed %d.\n", rv);
}
