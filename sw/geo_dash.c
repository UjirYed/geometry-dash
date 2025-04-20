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
Information about our geometry_dash device.
*/

struct geo_dash_dev {
    struct resource res; /* Our registers. */
    void __iomem *virtbase; /* Where our registers can be accessed in memory. */
    vga_ball_

} dev;
