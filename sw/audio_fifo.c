
#include <linux/module.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/ioctl.h>
#include "geo_dash.h"

// ===============================================
// ===== audio_fifo structures and constants =====
// ===============================================

#define AUDIO_FIFO_NAME "audio_fifo"
#define FIFO_FILL_LEVEL_OFFSET 0x00
#define FIFO_ISTATUS_OFFSET    0x04
#define FIFO_WRITE_OFFSET      0x40

struct audio_fifo_dev {
    struct resource res;
    void __iomem *virtbase;
} audio_dev;

static void write_audio_fifo(uint32_t sample) {
    iowrite32(sample, audio_dev.virtbase + FIFO_WRITE_OFFSET);
}

static uint32_t read_fifo_fill_level(void) {
    return ioread32(audio_dev.virtbase + FIFO_FILL_LEVEL_OFFSET);
}

static uint32_t read_fifo_status(void) {
    return ioread32(audio_dev.virtbase + FIFO_ISTATUS_OFFSET) & 0x3F; // Only i_status bits
}

static long audio_fifo_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
	printk(KERN_INFO "audio_fifo_ioctl called with cmd 0x%x\n", cmd);

    switch (cmd) {
        case WRITE_AUDIO_FIFO: {
            geo_dash_arg_t vla;
            if (copy_from_user(&vla, (geo_dash_arg_t *) arg, sizeof(vla)))
                return -EFAULT;
            write_audio_fifo(vla.audio);
            break;
        }

        case READ_AUDIO_STATUS: {
            uint32_t status = read_fifo_status();
            if (copy_to_user((uint32_t *)arg, &status, sizeof(status)))
                return -EFAULT;
            break;
        }

        case READ_AUDIO_FILL_LEVEL: {
            uint32_t level = read_fifo_fill_level();
            if (copy_to_user((uint32_t *)arg, &level, sizeof(level)))
                return -EFAULT;
            break;
        }

        default:
            return -EINVAL;
    }

    return 0;
}

static const struct file_operations audio_fifo_fops = {
    .owner = THIS_MODULE,
    .unlocked_ioctl = audio_fifo_ioctl
};

static struct miscdevice audio_fifo_misc_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = AUDIO_FIFO_NAME,
    .fops = &audio_fifo_fops
};

static int __init audio_fifo_probe(struct platform_device *pdev) {
    int ret;
	pr_info("audio_fifo: probe successful\n");

    ret = misc_register(&audio_fifo_misc_device);
    if (ret) {
        pr_err("audio_fifo: failed to register misc device\n");
        return ret;
    }

    ret = of_address_to_resource(pdev->dev.of_node, 0, &audio_dev.res);
    if (ret) {
        pr_err("audio_fifo: failed to get resource\n");
        goto out_deregister;
    }

    if (request_mem_region(audio_dev.res.start, resource_size(&audio_dev.res), AUDIO_FIFO_NAME) == NULL) {
        ret = -EBUSY;
        goto out_deregister;
    }

    audio_dev.virtbase = of_iomap(pdev->dev.of_node, 0);
    if (!audio_dev.virtbase) {
        ret = -ENOMEM;
        goto out_release;
    }

    pr_info("audio_fifo: probe successful\n");
    return 0;

out_release:
    release_mem_region(audio_dev.res.start, resource_size(&audio_dev.res));
out_deregister:
    misc_deregister(&audio_fifo_misc_device);
    return ret;
}

static int __exit audio_fifo_remove(struct platform_device *pdev) {
    iounmap(audio_dev.virtbase);
    release_mem_region(audio_dev.res.start, resource_size(&audio_dev.res));
    misc_deregister(&audio_fifo_misc_device);
    pr_info("audio_fifo: removed\n");
    return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id audio_fifo_of_match[] = {
    { .compatible = "ALTR,fifo-21.1" },
    { .compatible = "ALTR,fifo-1.0" },
    {},
};
MODULE_DEVICE_TABLE(of, audio_fifo_of_match);
#endif

static struct platform_driver audio_fifo_driver = {
    .probe = audio_fifo_probe,
    .remove = audio_fifo_remove,
    .driver = {
        .name = AUDIO_FIFO_NAME,
        .owner = THIS_MODULE,
        .of_match_table = of_match_ptr(audio_fifo_of_match),
    },
};

/* Called when the module is loaded: set things up */
static int __init audio_fifo_init(void)
{
    pr_info("audio_fifo: init\n");
    return platform_driver_register(&audio_fifo_driver);
}

static void __exit audio_fifo_exit(void)
{
    platform_driver_unregister(&audio_fifo_driver);
    pr_info("audio_fifo: exit\n");
}

module_init(geo_dash_init);
module_exit(geo_dash_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Stephen A. Edwards, Columbia University");
MODULE_DESCRIPTION("audio FIFO driver");
