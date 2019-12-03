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
static unsigned long mem_addr;

static int __init uio_dev_init(void)
{
    info = kzalloc(sizeof(struct uio_info), GFP_KERNEL);
    if (!info) {
        return -ENOMEM;
    }

    mem_addr = __get_free_pages(GFP_KERNEL, 0);
    if (!mem_addr) {
        return -ENOMEM;
    }

    memset((void*) mem_addr, 0, PAGE_SIZE);

    uio_pdev = platform_device_register_simple("uiogaps-platform", -1, NULL, 0);
    if (IS_ERR(uio_pdev)) {
	    return PTR_ERR(uio_pdev);
    }

    info->name = "uiogaps";
    info->version = "0.1.0";
    info->mem[0].memtype = UIO_MEM_LOGICAL;
    info->mem[0].addr = (phys_addr_t) mem_addr;
    info->mem[0].size = PAGE_SIZE;

    return uio_register_device(&uio_pdev->dev, info);
}

static void __exit uio_dev_exit(void)
{
    free_pages(mem_addr, 0);
    uio_unregister_device(info);
    kfree(info);
    platform_device_unregister(uio_pdev);
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Michael Spiegel <michael.spiegel@twosixlabs.com>");

module_init(uio_dev_init);
module_exit(uio_dev_exit);
