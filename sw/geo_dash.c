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
#include "vga_ball.h"

#define DRIVER_NAME "geo_dash"

// Assuming that we have 16-bit registers.
#define PLAYER_Y_POS(x) (x)
#define PLAYER_X_POS(x) ((x) + 2)
#define BACKGROUND_R(x) ((x))

#define READY(x) ((x) + 12)
#define SET(x) ((x) + 12)
#define OUTPUT(x) ((x) + 14)

/*
Information about our geometry_dash device. Acts as a mirror of hardware state.
*/

struct geo_dash_dev {
    struct resource res; /* Our registers. */
    void __iomem *virtbase; /* Where our registers can be accessed in memory. */
    short player_x_pos;

} dev;

static void write_player_xposition(unsigned short *value)
{
    iowrite16(*value, PLAYER_Y_POS(dev.virtbase));
    player_x_pos = *value;
}

static long geo_dash_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
    geo_dash_arg_t vla;

    switch (cmd) {
        case WRITE_PLAYER_XPOSITION:
            if (copy_from_user(&vla, (geo_dash_arg_t *) arg, sizeof(geo_dash_arg_t)))
                return -EACCES;
            write_player_xposition(&vla.player_pos_x)

    }
    return 0;
}

static const struct file_operations geo_dash_fops = {
    .owner = THIS_MODULE,
    .unlocked_ioctl = geo_dash_ioctl
};

static struct miscdevice geo_dash_misc_device = {
    .mnior = MISC_DYNAMIC_MINOR,
    .name = DRIVER_NAME,
    .fops = &geo_dash_fops
}

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
	{ .compatible = "csee4840,geo_dash-1.0" },
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

/* Calball when the module is unloaded: release resources */
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



