/*
 * This work was authored by Two Six Labs, LLC and is sponsored by a subcontract
 * agreement with Galois, Inc.  This material is based upon work supported by
 * the Defense Advanced Research Projects Agency (DARPA) under Contract No.
 * HR0011-19-C-0103.
 *
 * The Government has unlimited rights to use, modify, reproduce, release,
 * perform, display, or disclose computer software or computer software
 * documentation marked with this legend. Any reproduction of technical data,
 * computer software, or portions thereof marked with this legend must also
 * reproduce this marking.
 *
 * Copyright 2019 Two Six Labs, LLC.  All rights reserved.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/uio.h>
#include <linux/uio_driver.h>
#include <linux/fs.h>

static struct platform_device *uio_pdev;
static struct uio_info *info;
static void* mem_addr[4];

#define MEM_ORDER (4)
#define MEM_SIZE (PAGE_SIZE * (1 << MEM_ORDER))

static int __init uio_dev_init(void)
{
    int i;

    info = kzalloc(sizeof(struct uio_info), GFP_KERNEL);
    if (!info) {
        return -ENOMEM;
    }

    for (i = 0; i < 4; i++) {
        mem_addr[i] = vmalloc_user(MEM_SIZE);
        if (!mem_addr[i]) {
            return -ENOMEM;
        }
    }

    uio_pdev = platform_device_register_simple("uiogaps-platform", -1, NULL, 0);
    if (IS_ERR(uio_pdev)) {
	    return PTR_ERR(uio_pdev);
    }

    info->name = "uiogaps";
    info->version = "0.1.0";
    for (i = 0; i < 4; i++) {
        info->mem[i].memtype = UIO_MEM_VIRTUAL;
        info->mem[i].addr = (phys_addr_t) mem_addr[i];
        info->mem[i].size = MEM_SIZE;
    }

    return uio_register_device(&uio_pdev->dev, info);
}

static void __exit uio_dev_exit(void)
{
    int i;

    uio_unregister_device(info);
    for (i = 0; i < 4; i++) {
        vfree(mem_addr[i]);
    }
    kfree(info);
    platform_device_unregister(uio_pdev);
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Michael Spiegel <michael.spiegel@twosixlabs.com>");

module_init(uio_dev_init);
module_exit(uio_dev_exit);
