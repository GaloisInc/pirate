#ifndef _MERCURY_GAPS_ILIP_H
#define _MERCURY_GAPS_ILIP_H

#define GAPS_ILIP_ERR_BUFLEN		 (64)
#define GAPS_ILIP_RESP_BUFLEN_MAX	 (2048 * 6)
#define GAPS_ILIP_RESP_BUFLEN_MIN	 (256)

/**
 * gaps_ilip_attr_t netlink attributes for qdma(variables):
 * the index in this enum is used as a reference for the type,
 * userspace application has to indicate the corresponding type
 * the policy is used for security considerations
 */
enum gaps_ilip_attr_t {
	GAPS_ILIP_ATTR_GENMSG,		/**< generatl message */

	GAPS_ILIP_ATTR_DRV_INFO,		/**< device info */

	GAPS_ILIP_ATTR_DEV_IDX,		/**< device index */
	GAPS_ILIP_ATTR_PCI_BUS,		/**< pci bus number */
	GAPS_ILIP_ATTR_PCI_DEV,		/**< pci device number */
	GAPS_ILIP_ATTR_PCI_FUNC,		/**< pci function id */

	GAPS_ILIP_ATTR_DEV_STAT_MMH2C_PKTS1,	/**< number of MM H2C packets */
	GAPS_ILIP_ATTR_DEV_STAT_MMH2C_PKTS2,	/**< number of MM H2C packets */
	GAPS_ILIP_ATTR_DEV_STAT_MMC2H_PKTS1,	/**< number of MM C2H packets */
	GAPS_ILIP_ATTR_DEV_STAT_MMC2H_PKTS2,	/**< number of MM C2H packets */
	GAPS_ILIP_ATTR_DEV_STAT_STH2C_PKTS1,	/**< number of ST H2C packets */
	GAPS_ILIP_ATTR_DEV_STAT_STH2C_PKTS2,	/**< number of ST H2C packets */
	GAPS_ILIP_ATTR_DEV_STAT_STC2H_PKTS1,	/**< number of ST C2H packets */
	GAPS_ILIP_ATTR_DEV_STAT_STC2H_PKTS2,	/**< number of ST C2H packets */

	GAPS_ILIP_ATTR_DEV_CFG_BAR,		/**< device config bar number */
	GAPS_ILIP_ATTR_DEV_USR_BAR,		/**< device user bar number */
	GAPS_ILIP_ATTR_DEV_QSET_MAX,		/**< max queue sets */
	GAPS_ILIP_ATTR_DEV_QSET_QBASE,	/**< queue base start */

	GAPS_ILIP_ATTR_VERSION_INFO,		/**< version info */
	GAPS_ILIP_ATTR_DEVICE_TYPE,		/**< device type */
	GAPS_ILIP_ATTR_IP_TYPE,		/**< ip type */
	GAPS_ILIP_ATTR_DEV_NUMQS,		/**< num of queues */
	GAPS_ILIP_ATTR_DEV_NUM_PFS,		/**< num of PFs */
	GAPS_ILIP_ATTR_DEV_MM_CHANNEL_MAX,	/**< mm channels */
	GAPS_ILIP_ATTR_DEV_MAILBOX_ENABLE,	/**< mailbox enable */
	GAPS_ILIP_ATTR_DEV_FLR_PRESENT,	/**< flr present */
	GAPS_ILIP_ATTR_DEV_ST_ENABLE,		/**< device st capability */
	GAPS_ILIP_ATTR_DEV_MM_ENABLE,		/**< device mm capability */
	GAPS_ILIP_ATTR_DEV_MM_CMPT_ENABLE,	/**< device mm cmpt capability */

	GAPS_ILIP_ATTR_REG_BAR_NUM,		/**< register bar number */
	GAPS_ILIP_ATTR_REG_ADDR,		/**< register address */
	GAPS_ILIP_ATTR_REG_VAL,		/**< register value */

	GAPS_ILIP_ATTR_CSR_INDEX,		/**< csr index */
	GAPS_ILIP_ATTR_CSR_COUNT,		/**< csr count */

	GAPS_ILIP_ATTR_QIDX,			/**< queue index */
	GAPS_ILIP_ATTR_NUM_Q,			/**< number of queues */
	GAPS_ILIP_ATTR_QFLAG,			/**< queue config flags */

	GAPS_ILIP_ATTR_CMPT_DESC_SIZE,	/**< completion descriptor size */
	GAPS_ILIP_ATTR_SW_DESC_SIZE,		/**< software descriptor size */
	GAPS_ILIP_ATTR_QRNGSZ_IDX,		/**< queue ring index */
	GAPS_ILIP_ATTR_C2H_BUFSZ_IDX,		/**< c2h buffer idex */
	GAPS_ILIP_ATTR_CMPT_TIMER_IDX,	/**< completion timer index */
	GAPS_ILIP_ATTR_CMPT_CNTR_IDX,		/**< completion counter index */
	GAPS_ILIP_ATTR_CMPT_TRIG_MODE,	/**< completion trigger mode */
	GAPS_ILIP_ATTR_MM_CHANNEL,		/**< mm channel */
	GAPS_ILIP_ATTR_CMPT_ENTRIES_CNT,      /**< completion entries count */

	GAPS_ILIP_ATTR_RANGE_START,		/**< range start */
	GAPS_ILIP_ATTR_RANGE_END,		/**< range end */

	GAPS_ILIP_ATTR_INTR_VECTOR_IDX,	/**< interrupt vector index */
	GAPS_ILIP_ATTR_INTR_VECTOR_START_IDX, /**< interrupt vector start index */
	GAPS_ILIP_ATTR_INTR_VECTOR_END_IDX,	/**< interrupt vector end index */
	GAPS_ILIP_ATTR_RSP_BUF_LEN,		/**< response buffer length */
	GAPS_ILIP_ATTR_GLOBAL_CSR,		/**< global csr data */
	GAPS_ILIP_ATTR_PIPE_GL_MAX,		/**< max no. of gl for pipe */
	GAPS_ILIP_ATTR_PIPE_FLOW_ID,          /**< pipe flow id */
	GAPS_ILIP_ATTR_PIPE_SLR_ID,           /**< pipe slr id */
	GAPS_ILIP_ATTR_PIPE_TDEST,            /**< pipe tdest */
	GAPS_ILIP_ATTR_DEV_STM_BAR,		/**< device STM bar number */
	GAPS_ILIP_ATTR_Q_STATE,
	GAPS_ILIP_ATTR_ERROR,
	GAPS_ILIP_ATTR_DEV,
	GAPS_ILIP_ATTR_SESSION_ID,		/**< session ID */
#ifdef ERR_DEBUG
	GAPS_ILIP_ATTR_QPARAM_ERR_INFO,	/**< queue param info */
#endif
	GAPS_ILIP_ATTR_MAX,
};

/* commands, 0 ~ 0x7F */
/**
 * gaps_ilip_op_t - XNL command types
 */
enum gaps_ilip_op_t {
	GAPS_ILIP_CMD_DEV_LIST,	/**< list all the qdma devices */
	GAPS_ILIP_CMD_DEV_INFO,	/**< dump the device information */
	GAPS_ILIP_CMD_DEV_STAT,	/**< dump the device statistics */
	GAPS_ILIP_CMD_DEV_STAT_CLEAR,	/**< reset the device statistics */

	GAPS_ILIP_CMD_REG_DUMP,	/**< dump the register information */
	GAPS_ILIP_CMD_REG_RD,		/**< read a register value */
	GAPS_ILIP_CMD_REG_WRT,	/**< write value to a register */

	GAPS_ILIP_CMD_GLOBAL_CSR,	/**< get all global csr register values */
	GAPS_ILIP_CMD_DEV_CAP,	/**< list h/w capabilities , hw and sw version */
	GAPS_ILIP_CMD_MAX,		/**< max number of XNL commands*/
};

#endif // _MERCURY_GAPS_ILIP_H
