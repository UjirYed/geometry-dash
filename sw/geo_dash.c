/* * Device driver for the VGA video generator
 *
 * A Platform device implemented using the misc subsystem
 *
 * Riju Dey, Rachinta Marpaung, Charles Chen, Sasha Isler
 * Modified from Stephen Edwards
 * Columbia University
 *
 * References:
 * Linux source: Documentation/driver-model/platform.txt
 *               drivers/misc/arm-charlcd.c
 * http://www.linuxforu.com/tag/linux-device-drivers/
 * http://free-electrons.com/docs/
 *
 * "make" to build
 * insmod vga_ball.ko
 *
 * Check code style with
 * checkpatch.pl --file --no-tree vga_ball.c
 */

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

#define DRIVER_NAME "geo_dash"

#define AUDIO_FIFO_BASE_ADDR 0x00014040

// Assuming that we have 16-bit registers.
#define PLAYER_Y_POS(base)   ((base) + 0x00)  // 16-bit
#define X_SHIFT(base)        ((base) + 0x02)  // 16-bit

#define BACKGROUND_R(base)   ((base) + 0x04)  // lower 8 bits used
#define BACKGROUND_G(base)   ((base) + 0x06)  // lower 8 bits used
#define BACKGROUND_B(base)   ((base) + 0x08)  // lower 8 bits used

#define MAP_BLOCK(base)      ((base) + 0x0A)  // lower 8 bits used
#define FLAGS(base)          ((base) + 0x0C)  // lower 8 bits used
#define OUTPUT_FLAGS(base)   ((base) + 0x0E)  // lower 8 bits used
#define FIFO_IN              ((base) + AUDIO_FIFO_BASE_ADDR) // this is where the FIFO should be relative to base addr....?
/*
Information about our geometry_dash device. Acts as a mirror of hardware state.
*/

struct geo_dash_dev {
    struct resource res; /* Our registers. */
    void __iomem *virtbase; /* Where our registers can be accessed in memory. */
  	void __iomem *audio_fifo_base;
    short x_shift;
} dev;

static void write_player_y_position(unsigned short *value) {
    iowrite16(*value, PLAYER_Y_POS(dev.virtbase));
}

static void write_x_shift(unsigned short *value) {
    iowrite16(*value, X_SHIFT(dev.virtbase));
    dev.x_shift = *value;
}

static void write_background_r(uint8_t *value) {
    iowrite16((uint16_t)(*value), BACKGROUND_R(dev.virtbase));
}

static void write_background_g(uint8_t *value) {
    iowrite16((uint16_t)(*value), BACKGROUND_G(dev.virtbase));
}

static void write_background_b(uint8_t *value) {
    iowrite16((uint16_t)(*value), BACKGROUND_B(dev.virtbase));
}

static void write_map_block(uint8_t *value) {
    iowrite16((uint16_t)(*value), MAP_BLOCK(dev.virtbase));
}

static void write_flags(uint8_t *value) {
    iowrite16((uint16_t)(*value), FLAGS(dev.virtbase));
}

static void write_output_flags(uint8_t *value) {
    iowrite16((uint16_t)(*value), OUTPUT_FLAGS(dev.virtbase));
}

static void write_audio_fifo(uint16_t sample) {
    printk("[write_audio_fifo]: attempting to write to audio fifo\n");
    iowrite16(sample, FIFO_IN(dev.virtbase));
}

static long geo_dash_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
    geo_dash_arg_t vla;

    // Copy user struct into kernel space
    if (copy_from_user(&vla, (geo_dash_arg_t *) arg, sizeof(vla)))
        return -EFAULT;

    switch (cmd) {
        case WRITE_X_SHIFT:
            write_x_shift(&vla.x_shift);
            break;

        case WRITE_PLAYER_Y_POS:
            write_player_y_position(&vla.player_y);
            break;

        case WRITE_BACKGROUND_R:
            write_background_r(&vla.bg_r);
            break;

        case WRITE_BACKGROUND_G:
            write_background_g(&vla.bg_g);
            break;

        case WRITE_BACKGROUND_B:
            write_background_b(&vla.bg_b);
            break;

        case WRITE_MAP_BLOCK:
            write_map_block(&vla.map_block);
            break;

        case WRITE_FLAGS:
            write_flags(&vla.flags);
            break;

        case WRITE_OUTPUT_FLAGS:
            write_output_flags(&vla.output_flags);
            break;

		case WRITE_AUDIO_FIFO:
			write_audio_fifo(vla.audio);
			break;

        default:
            return -EINVAL;  // Unknown command
    }

    return 0;
}



static const struct file_operations geo_dash_fops = {
    .owner = THIS_MODULE,
    .unlocked_ioctl = geo_dash_ioctl
};

static struct miscdevice geo_dash_misc_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = DRIVER_NAME,
    .fops = &geo_dash_fops
};

/*
 * Initialization code: get resources (registers) and display
 * a welcome message
 */
static int __init geo_dash_probe(struct platform_device *pdev)
{
        //cat_invaders_color_t beige = { 0xf9, 0xe4, 0xb7 };
		//audio_t audio_begin = { 0x00, 0x00, 0x00 };
	int ret;

	/* Register ourselves as a misc device: creates /dev/geo_dash */
	ret = misc_register(&geo_dash_misc_device);

	/* Get the address of our registers from the device tree */
	ret = of_address_to_resource(pdev->dev.of_node, 0, &dev.res);
	if (ret) {
		ret = -ENOENT;
		goto out_deregister;
	}

	/* Make sure we can use these registers */
	if (request_mem_region(dev.res.start, resource_size(&dev.res),
			       DRIVER_NAME) == NULL) {
		ret = -EBUSY;
		goto out_deregister;
	}
	
	/* Arrange access to our registers */
	dev.virtbase = of_iomap(pdev->dev.of_node, 0);
	if (dev.virtbase == NULL) {
		ret = -ENOMEM;
		goto out_release_mem_region;
	}

	struct device_node *parent = pdev->dev.of_node->parent;
	// How do we know that fifo is always going to be placed here??
	struct device_node *fifo_node = of_find_node_by_name(parent, "fifo@0x100000020");

	if (fifo_node) {
        struct resource fifo_res;
        if (of_address_to_resource(fifo_node, 0, &fifo_res) == 0) {
            dev.audio_fifo_base = ioremap(fifo_res.start, resource_size(&fifo_res));
            if (!dev.audio_fifo_base) {
                pr_err("geo_dash: Failed to ioremap FIFO\n");
                goto out_release_mem;
            }
        } else {
            pr_err("geo_dash: Failed to get FIFO resource\n");
            goto out_release_mem;
        }
    } else {
        pr_err("geo_dash: FIFO node not found\n");
        goto out_release_mem;
    }
        

	return 0;

out_release_mem_region:
	release_mem_region(dev.res.start, resource_size(&dev.res));
out_deregister:
	misc_deregister(&geo_dash_misc_device);
	return ret;
}

/* Clean-up code: release resources */
static int geo_dash_remove(struct platform_device *pdev)
{
	iounmap(dev.virtbase);
	release_mem_region(dev.res.start, resource_size(&dev.res));
	misc_deregister(&geo_dash_misc_device);
	return 0;
}

/* Which "compatible" string(s) to search for in the Device Tree */
#ifdef CONFIG_OF
static const struct of_device_id geo_dash_of_match[] = {
	{ .compatible = "csee4840,player_sprite-1.0" },
	{},
};
MODULE_DEVICE_TABLE(of, geo_dash_of_match);
#endif

/* Information for registering ourselves as a "platform" driver */
static struct platform_driver geo_dash_driver = {
	.driver	= {
		.name	= DRIVER_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(geo_dash_of_match),
	},
	.remove	= __exit_p(geo_dash_remove),
};

/* Called when the module is loaded: set things up */
static int __init geo_dash_init(void)
{
	pr_info(DRIVER_NAME ": init\n");
	return platform_driver_probe(&geo_dash_driver, geo_dash_probe);
}

/* Called when the module is unloaded: release resources */
static void __exit geo_dash_exit(void)
{
	platform_driver_unregister(&geo_dash_driver);
	pr_info(DRIVER_NAME ": exit\n");
}

module_init(geo_dash_init);
module_exit(geo_dash_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Stephen A. Edwards, Columbia University");
MODULE_DESCRIPTION("geometry dash driver");



