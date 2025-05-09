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

// =============================================
// ===== geo_dash structures and constants =====
// =============================================

#define DRIVER_NAME "player_sprite_0"
// Assuming that we have 16-bit registers.
#define PLAYER_Y_POS(base)   ((base) + 0x00)  // 16-bit
#define X_SHIFT(base)        ((base) + 0x02)  // 16-bit

#define BACKGROUND_R(base)   ((base) + 0x04)  // lower 8 bits used
#define BACKGROUND_G(base)   ((base) + 0x06)  // lower 8 bits used
#define BACKGROUND_B(base)   ((base) + 0x08)  // lower 8 bits used

#define MAP_BLOCK(base)      ((base) + 0x0A)  // lower 8 bits used
#define FLAGS(base)          ((base) + 0x0C)  // lower 8 bits used
#define OUTPUT_FLAGS(base)   ((base) + 0x0E)  // lower 8 bits used

/*
Information about our geometry_dash device. Acts as a mirror of hardware state.
*/

struct geo_dash_dev {
    struct resource res; /* Our registers. */
    void __iomem *virtbase; /* Where our registers can be accessed in memory. */
    short x_shift;
} geo_dash_dev;

static void write_player_y_position(unsigned short *value) {
    iowrite16(*value, PLAYER_Y_POS(geo_dash_dev.virtbase));
}

static void write_x_shift(unsigned short *value) {
    iowrite16(*value, X_SHIFT(geo_dash_dev.virtbase));
    geo_dash_dev.x_shift = *value;
}

static void write_background_r(uint8_t *value) {
    iowrite16((uint16_t)(*value), BACKGROUND_R(geo_dash_dev.virtbase));
}

static void write_background_g(uint8_t *value) {
    iowrite16((uint16_t)(*value), BACKGROUND_G(geo_dash_dev.virtbase));
}

static void write_background_b(uint8_t *value) {
    iowrite16((uint16_t)(*value), BACKGROUND_B(geo_dash_dev.virtbase));
}

static void write_map_block(uint8_t *value) {
    iowrite16((uint16_t)(*value), MAP_BLOCK(geo_dash_dev.virtbase));
}

static void write_flags(uint8_t *value) {
    iowrite16((uint16_t)(*value), FLAGS(geo_dash_dev.virtbase));
}

static void write_output_flags(uint8_t *value) {
    iowrite16((uint16_t)(*value), OUTPUT_FLAGS(geo_dash_dev.virtbase));
}

static long geo_dash_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
    geo_dash_arg_t vla;
	printk(KERN_INFO "geo_dash_ioctl called with cmd 0x%x\n", cmd);

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
	pr_info("geo_dash: probe successful\n");

	/* Register ourselves as a misc device: creates /dev/geo_dash */
	ret = misc_register(&geo_dash_misc_device);

	/* Get the address of our registers from the device tree */
	ret = of_address_to_resource(pdev->dev.of_node, 0, &geo_dash_dev.res);
	if (ret) {
		ret = -ENOENT;
		goto out_deregister;
	}

	/* Make sure we can use these registers */
	if (request_mem_region(geo_dash_dev.res.start, resource_size(&geo_dash_dev.res),
			       DRIVER_NAME) == NULL) {
		ret = -EBUSY;
		goto out_deregister;
	}
	
	/* Arrange access to our registers */
	geo_dash_dev.virtbase = of_iomap(pdev->dev.of_node, 0);
	if (geo_dash_dev.virtbase == NULL) {
		ret = -ENOMEM;
		goto out_release_mem_region;
	}

	return 0;

out_release_mem_region:
	release_mem_region(geo_dash_dev.res.start, resource_size(&geo_dash_dev.res));
out_deregister:
	misc_deregister(&geo_dash_misc_device);
	return ret;
}

/* Clean-up code: release resources */
static int geo_dash_remove(struct platform_device *pdev)
{
	iounmap(geo_dash_dev.virtbase);
	release_mem_region(geo_dash_dev.res.start, resource_size(&geo_dash_dev.res));
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
	.probe = geo_dash_probe, // move it here
	.remove = __exit_p(geo_dash_remove),
	.driver = {
		.name = DRIVER_NAME,
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(geo_dash_of_match),
	},
};


// ============================================
// =========== module init and exit ===========
// ============================================

/* Called when the module is loaded: set things up */
static int __init geo_dash_init(void)
{
    int ret;

    pr_info(DRIVER_NAME ": init\n");

    ret = platform_driver_register(&geo_dash_driver);
    if (ret)
        return ret;

    return 0;
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
