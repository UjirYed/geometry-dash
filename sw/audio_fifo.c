
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
#include "audio_fifo.h"

// ===============================================
// ===== audio_fifo structures and constants =====
// ===============================================

#define AUDIO_FIFO_NAME "audio_fifo"
#define FIFO_ISTATUS_OFFSET    0x4 // relative to CSR base

struct audio_fifo_dev {
    struct resource res;
	struct resource res_fifo;
    struct resource res_csr;
    void __iomem *virtbase;
	void __iomem *virtbase_csr;
} audio_dev;

static void write_audio_fifo(uint32_t sample) {
    iowrite32(sample, audio_dev.virtbase);
}

static uint32_t read_fifo_fill_level(void) {
    return ioread32(audio_dev.virtbase_csr);
}

static uint32_t read_fifo_status(void) {
    return ioread32(audio_dev.virtbase_csr + FIFO_ISTATUS_OFFSET) & 0x3F; // Only i_status bits
}

static long audio_fifo_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
    printk(KERN_INFO "audio_fifo_ioctl called with cmd 0x%x\n", cmd);
	if (!audio_dev.virtbase) {
		pr_err("audio_fifo_ioctl: virtbase is NULL\n");
		return -EIO;
	}
    switch (cmd) {
		case WRITE_AUDIO_FIFO: {
      printk(KERN_INFO "WRITE_AUDIO_FIFO");
			audio_fifo_arg_t vla;
			if (copy_from_user(&vla, (audio_fifo_arg_t __user *)arg, sizeof(vla)))
				return -EFAULT;
			write_audio_fifo(vla.audio);
			break;
		}
	
		case READ_AUDIO_STATUS: {
      printk(KERN_INFO "READ_AUDIO_STATUS");
			uint32_t status = read_fifo_status();
			if (copy_to_user((uint32_t __user *)arg, &status, sizeof(status)))
				return -EFAULT;
			break;
		}
	
		case READ_AUDIO_FILL_LEVEL: {
      printk(KERN_INFO "READ_AUDIO_FILL_LEVEL");
			uint32_t level = read_fifo_fill_level();
			if (copy_to_user((uint32_t __user *)arg, &level, sizeof(level)))
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

    pr_info("audio_fifo: probe started\n");

    // Register misc device
    ret = misc_register(&audio_fifo_misc_device);
    if (ret) {
        pr_err("audio_fifo: failed to register misc device\n");
        return ret;
    }

    // Get FIFO memory resource
    ret = of_address_to_resource(pdev->dev.of_node, 0, &audio_dev.res_fifo);
    if (ret) {
        pr_err("audio_fifo: failed to get FIFO resource\n");
        goto out_deregister;
    }
    if (!request_mem_region(audio_dev.res_fifo.start, resource_size(&audio_dev.res_fifo), AUDIO_FIFO_NAME)) {
        ret = -EBUSY;
        goto out_deregister;
    }

    // Get CSR memory resource
    ret = of_address_to_resource(pdev->dev.of_node, 1, &audio_dev.res_csr);
    if (ret) {
        pr_err("audio_fifo: failed to get CSR resource\n");
        goto out_release_fifo;
    }
    if (!request_mem_region(audio_dev.res_csr.start, resource_size(&audio_dev.res_csr), AUDIO_FIFO_NAME "_csr")) {
        ret = -EBUSY;
        goto out_release_fifo;
    }

    // Map FIFO base
    audio_dev.virtbase = of_iomap(pdev->dev.of_node, 0);
    if (!audio_dev.virtbase) {
        pr_err("audio_fifo: failed to map FIFO registers\n");
        ret = -ENOMEM;
        goto out_release_csr;
    }

    // Map CSR base
    audio_dev.virtbase_csr = of_iomap(pdev->dev.of_node, 1);
    if (!audio_dev.virtbase_csr) {
        pr_err("audio_fifo: failed to map CSR registers\n");
        ret = -ENOMEM;
        goto out_unmap_fifo;
    }

    pr_info("audio_fifo: probe successful\n");
    pr_info("audio_fifo: FIFO mapped to %p, CSR mapped to %p\n", audio_dev.virtbase, audio_dev.virtbase_csr);
    return 0;

// Cleanup paths
out_unmap_fifo:
    iounmap(audio_dev.virtbase);
out_release_csr:
    release_mem_region(audio_dev.res_csr.start, resource_size(&audio_dev.res_csr));
out_release_fifo:
    release_mem_region(audio_dev.res_fifo.start, resource_size(&audio_dev.res_fifo));
out_deregister:
    misc_deregister(&audio_fifo_misc_device);
    return ret;
}


static int __exit audio_fifo_remove(struct platform_device *pdev) {
	iounmap(audio_dev.virtbase);
	iounmap(audio_dev.virtbase_csr);
	release_mem_region(audio_dev.res_fifo.start, resource_size(&audio_dev.res_fifo));
	release_mem_region(audio_dev.res_csr.start, resource_size(&audio_dev.res_csr));
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

module_init(audio_fifo_init);
module_exit(audio_fifo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Stephen A. Edwards, Columbia University");
MODULE_DESCRIPTION("audio FIFO driver");
