/* linux/arch/arm/mach-s3c64xx/mach-gt_i5700.c
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *	http://armlinux.simtec.co.uk/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
*/

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/timer.h>
#include <linux/init.h>
#include <linux/serial_core.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/i2c.h>
#include <linux/i2c-gpio.h>
#include <linux/leds.h>
#include <linux/fb.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/max8698.h>
#include <linux/android_pmem.h>
#include <linux/reboot.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_gpio.h>
#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>
#include <linux/switch.h>
#include <linux/fsa9480.h>
#include <linux/clk.h>
#include <linux/mmc/host.h>
#include <linux/gpio_keys.h>

#include <video/s6d05a.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/irq.h>

#include <mach/hardware.h>
#include <mach/regs-fb.h>
#include <mach/map.h>

#include <asm/irq.h>
#include <asm/mach-types.h>
#include <asm/cpu-single.h>
#include <asm/setup.h>

#include <plat/regs-serial.h>
#include <plat/iic.h>
#include <plat/fb.h>
#include <plat/clock.h>
#include <plat/devs.h>
#include <plat/cpu.h>
#include <plat/adc.h>
#include <plat/ts.h>
#include <plat/sdhci.h>
#include <plat/keypad.h>

#include <mach/regs-modem.h>
#include <mach/regs-gpio.h>
#include <mach/regs-sys.h>
#include <mach/regs-srom.h>
#include <mach/gpio-cfg.h>
#include <mach/s3c6410.h>
#include <mach/gt_i5700.h>
#include <mach/regs-gpio-memport.h>
#include <mach/regs-gpio.h>
#include <mach/regs-clock.h>

/*
 * UART
 */

#define UCON S3C2410_UCON_DEFAULT
#define ULCON S3C2410_LCON_CS8 | S3C2410_LCON_PNONE
#define UFCON S3C2410_UFCON_RXTRIG8 | S3C2410_UFCON_FIFOMODE

#define S3C64XX_KERNEL_PANIC_DUMP_SIZE 0x8000 /* 32kbytes */
void *S3C64XX_KERNEL_PANIC_DUMP_ADDR;

static struct s3c2410_uartcfg spica_uartcfgs[] __initdata = {
	[0] = {	/* Phone */
		.hwport	     = 0,
		.flags	     = 0,
		.ucon	     = UCON,
		.ulcon	     = ULCON,
		.ufcon	     = UFCON,
	},
	[1] = {	/* Bluetooth */
		.hwport	     = 1,
		.flags	     = 0,
		.ucon	     = UCON,
		.ulcon	     = ULCON,
		.ufcon	     = UFCON,
	},
	[2] = {	/* Serial */
		.hwport	     = 2,
		.flags	     = 0,
		.ucon	     = UCON,
		.ulcon	     = ULCON,
		.ufcon	     = UFCON,
	},
};

/*
 * USB gadget
 */

#include <linux/usb/android_composite.h>
#define S3C_VENDOR_ID			0x18d1
#define S3C_UMS_PRODUCT_ID		0x4E21
#define S3C_UMS_ADB_PRODUCT_ID		0x4E22
#define S3C_RNDIS_PRODUCT_ID		0x4E23
#define S3C_RNDIS_ADB_PRODUCT_ID	0x4E24
#define MAX_USB_SERIAL_NUM	17

static char *usb_functions_ums[] = {
	"usb_mass_storage",
};

static char *usb_functions_rndis[] = {
	"rndis",
};

static char *usb_functions_rndis_adb[] = {
	"rndis",
	"adb",
};
static char *usb_functions_ums_adb[] = {
	"usb_mass_storage",
	"adb",
};

static char *usb_functions_all[] = {
	"rndis",
	"usb_mass_storage",
	"adb",
};

static struct android_usb_product usb_products[] = {
	{
		.product_id	= S3C_UMS_PRODUCT_ID,
		.num_functions	= ARRAY_SIZE(usb_functions_ums),
		.functions	= usb_functions_ums,
	},
	{
		.product_id	= S3C_UMS_ADB_PRODUCT_ID,
		.num_functions	= ARRAY_SIZE(usb_functions_ums_adb),
		.functions	= usb_functions_ums_adb,
	},
	{
		.product_id	= S3C_RNDIS_PRODUCT_ID,
		.num_functions	= ARRAY_SIZE(usb_functions_rndis),
		.functions	= usb_functions_rndis,
	},
	{
		.product_id	= S3C_RNDIS_ADB_PRODUCT_ID,
		.num_functions	= ARRAY_SIZE(usb_functions_rndis_adb),
		.functions	= usb_functions_rndis_adb,
	},
};

static char device_serial[MAX_USB_SERIAL_NUM] = "0123456789ABCDEF";
/* standard android USB platform data */

/* Information should be changed as real product for commercial release */
static struct android_usb_platform_data android_usb_pdata = {
	.vendor_id		= S3C_VENDOR_ID,
	.product_id		= S3C_UMS_PRODUCT_ID,
	.manufacturer_name	= "Samsung",
	.product_name		= "Galaxy GT-I5700",
	.serial_number		= device_serial,
	.num_products		= ARRAY_SIZE(usb_products),
	.products		= usb_products,
	.num_functions		= ARRAY_SIZE(usb_functions_all),
	.functions		= usb_functions_all,
};

struct platform_device spica_android_usb = {
	.name	= "android_usb",
	.id	= -1,
	.dev	= {
		.platform_data	= &android_usb_pdata,
	},
};

static struct usb_mass_storage_platform_data ums_pdata = {
	.vendor			= "Android",
	.product		= "UMS Composite",
	.release		= 1,
	.nluns			= 1,
};

struct platform_device spica_usb_mass_storage = {
	.name	= "usb_mass_storage",
	.id	= -1,
	.dev	= {
		.platform_data = &ums_pdata,
	},
};

static struct usb_ether_platform_data rndis_pdata = {
	/* ethaddr is filled by board_serialno_setup */
	.vendorID	= 0x18d1,
	.vendorDescr	= "Samsung",
};

struct platform_device spica_usb_rndis = {
	.name	= "rndis",
	.id	= -1,
	.dev	= {
		.platform_data = &rndis_pdata,
	},
};

/*
 * I2C devices
 */

/* I2C 0 (hardware) -	FSA9480 (USB transceiver),
 *			BMA023 (accelerometer),
 * 			AK8973B (magnetometer) */
static struct s3c2410_platform_i2c spica_misc_i2c __initdata = {
	.flags		= 0,
	.slave_addr	= 0x10,
	.frequency	= 100*1000,
	.sda_delay	= 100,
	.bus_num	= 0,
};

static void fsa9480_usb_cb(bool attached)
{
	struct usb_gadget *gadget = platform_get_drvdata(&s3c_device_usb_hsotg);

	if (gadget) {
		if (attached)
			usb_gadget_vbus_connect(gadget);
		else
			usb_gadget_vbus_disconnect(gadget);
	}
}

static void fsa9480_charger_cb(bool attached)
{
	/* TODO */
}

static void fsa9480_deskdock_cb(bool attached)
{
	/* TODO */
}

static void fsa9480_cardock_cb(bool attached)
{
	/* TODO */
}

static void fsa9480_reset_cb(void)
{
	/* TODO */
}

static struct fsa9480_platform_data spica_fsa9480_pdata = {
	.usb_cb = fsa9480_usb_cb,
	.charger_cb = fsa9480_charger_cb,
	.deskdock_cb = fsa9480_deskdock_cb,
	.cardock_cb = fsa9480_cardock_cb,
	.reset_cb = fsa9480_reset_cb,
};

static struct i2c_board_info spica_misc_i2c_devs[] __initdata = {
	{
		.type		= "fsa9480",
		.addr		= 0x25,
		.irq		= IRQ_EINT(9),
		.platform_data	= &spica_fsa9480_pdata,
	},
	{
		.type		= "bma023",
		.addr		= 0x38,
		.irq		= IRQ_EINT(3),
	},
	{
		.type		= "ak8973",
		.addr		= 0x1c,
	},
};

/* I2C 1 (hardware) -	UNKNOWN (camera sensor) */
static struct s3c2410_platform_i2c spica_cam_i2c __initdata = {
	.flags		= 0,
	.slave_addr	= 0x10,
	.frequency	= 100*1000,
	.sda_delay	= 100,
	.bus_num	= 1,
};

static struct i2c_board_info spica_cam_i2c_devs[] __initdata = {
	/* TODO */
};

/* I2C 2 (GPIO) -	MAX8698EWO-T (voltage regulator) */
static struct i2c_gpio_platform_data spica_pmic_i2c_pdata = {
	.sda_pin		= GPIO_PWR_I2C_SDA,
	.scl_pin		= GPIO_PWR_I2C_SCL,
	.udelay			= 2, /* 250KHz */
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only	= 1,
};

static struct platform_device spica_pmic_i2c = {
	.name			= "i2c-gpio",
	.id			= 2,
	.dev.platform_data	= &spica_pmic_i2c_pdata,
};

static struct regulator_init_data spica_ldo2_data = {
	.constraints	= {
		.name			= "VAP_ALIVE_1.2V",
		.min_uV			= 1200000,
		.max_uV			= 1200000,
		.apply_uV		= 0,
		.always_on		= 1,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL,
		.state_mem		= {
			.enabled = 1,
		},
	},
};

static struct regulator_consumer_supply ldo3_consumer[] = {
	REGULATOR_SUPPLY("pd_io", "s3c-usbgadget")
};

static struct regulator_init_data spica_ldo3_data = {
	.constraints	= {
		.name			= "VAP_OTGI_1.2V",
		.min_uV			= 1200000,
		.max_uV			= 1200000,
		.apply_uV		= 0,
		.valid_ops_mask 	= REGULATOR_CHANGE_STATUS,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL,
		.state_mem		= {
			.disabled = 1,
		},
	},
	.num_consumer_supplies	= ARRAY_SIZE(ldo3_consumer),
	.consumer_supplies	= ldo3_consumer,
};

static struct regulator_init_data spica_ldo4_data = {
	.constraints	= {
		.name			= "VLED_3.3V",
		.min_uV			= 3300000,
		.max_uV			= 3300000,
		.apply_uV		= 0,
		.always_on		= 1,
		.valid_ops_mask 	= REGULATOR_CHANGE_STATUS,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL,
		.state_mem		= {
			.disabled = 1,
		},
	},
};

static struct regulator_consumer_supply ldo5_consumer[] = {
	{	.supply = "tf_vdd3", },
};

static struct regulator_init_data spica_ldo5_data = {
	.constraints	= {
		.name			= "VTF_3.0V",
		.min_uV			= 3000000,
		.max_uV			= 3000000,
		.apply_uV		= 0,
		.valid_ops_mask 	= REGULATOR_CHANGE_STATUS,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL,
		.state_mem		= {
			.disabled = 1,
		},
	},
	.num_consumer_supplies	= ARRAY_SIZE(ldo5_consumer),
	.consumer_supplies	= ldo5_consumer,
};

static struct regulator_consumer_supply ldo6_consumer[] = {
	{	.supply	= "lcd_vdd3", },
};

static struct regulator_init_data spica_ldo6_data = {
	.constraints	= {
		.name			= "VLCD_1.8V",
		.min_uV			= 1800000,
		.max_uV			= 1800000,
		.apply_uV		= 0,
		.valid_ops_mask 	= REGULATOR_CHANGE_STATUS,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL,
		.state_mem		= {
			.disabled = 1,
		},
	},
	.num_consumer_supplies	= ARRAY_SIZE(ldo6_consumer),
	.consumer_supplies	= ldo6_consumer,
};

static struct regulator_consumer_supply ldo7_consumer[] = {
	{	.supply	= "lcd_vci", },
};

static struct regulator_init_data spica_ldo7_data = {
	.constraints	= {
		.name			= "VLCD_3.0V",
		.min_uV			= 3000000,
		.max_uV			= 3000000,
		.apply_uV		= 0,
		.valid_ops_mask 	= REGULATOR_CHANGE_STATUS,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL,
		.state_mem		= {
			.disabled = 1,
		},
	},
	.num_consumer_supplies	= ARRAY_SIZE(ldo7_consumer),
	.consumer_supplies	= ldo7_consumer,
};

static struct regulator_consumer_supply ldo8_consumer[] = {
	REGULATOR_SUPPLY("pd_core", "s3c-usbgadget")
};

static struct regulator_init_data spica_ldo8_data = {
	.constraints	= {
		.name			= "VAP_OTG_3.3V",
		.min_uV			= 3300000,
		.max_uV			= 3300000,
		.apply_uV		= 0,
		.valid_ops_mask		= REGULATOR_CHANGE_STATUS,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL,
		.state_mem		= {
			.disabled = 1,
		},
	},
	.num_consumer_supplies	= ARRAY_SIZE(ldo8_consumer),
	.consumer_supplies	= ldo8_consumer,
};

static struct regulator_init_data spica_ldo9_data = {
	.constraints	= {
		.name			= "VAP_SYS_3.0V",
		.min_uV			= 3000000,
		.max_uV			= 3000000,
		.apply_uV		= 0,
		.always_on		= 1,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL,
	},
};

static struct regulator_consumer_supply buck1_consumer[] = {
	{	.supply	= "vddarm", },
};

static struct regulator_init_data spica_buck1_data = {
	.constraints	= {
		.name			= "VAP_ARM",
		.min_uV			= 750000,
		.max_uV			= 1500000,
		.apply_uV		= 0,
		.always_on		= 1,
		.valid_ops_mask		= REGULATOR_CHANGE_VOLTAGE |
					  REGULATOR_CHANGE_STATUS,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL,
		.state_mem		= {
			.uV	= 1200000,
			.mode	= REGULATOR_MODE_NORMAL,
			.disabled = 1,
		},
	},
	.num_consumer_supplies	= ARRAY_SIZE(buck1_consumer),
	.consumer_supplies	= buck1_consumer,
};

static struct regulator_consumer_supply buck2_consumer[] = {
	{	.supply	= "vddint", },
};

static struct regulator_init_data spica_buck2_data = {
	.constraints	= {
		.name			= "VAP_CORE_1.3V",
		.min_uV			= 750000,
		.max_uV			= 1500000,
		.apply_uV		= 0,
		.always_on		= 1,
		.valid_ops_mask		= REGULATOR_CHANGE_VOLTAGE |
					  REGULATOR_CHANGE_STATUS,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL,
		.state_mem		= {
			.uV	= 1300000,
			.mode	= REGULATOR_MODE_NORMAL,
			.disabled = 1,
		},
	},
	.num_consumer_supplies	= ARRAY_SIZE(buck2_consumer),
	.consumer_supplies	= buck2_consumer,
};

static struct regulator_init_data spica_buck3_data = {
	.constraints	= {
		.name			= "VAP_MEM_1.8V",
		.min_uV			= 1800000,
		.max_uV			= 1800000,
		.apply_uV		= 0,
		.always_on		= 1,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL,
	},
};

static struct max8698_regulator_data spica_regulators[] = {
	{ MAX8698_LDO2,  &spica_ldo2_data },
	{ MAX8698_LDO3,  &spica_ldo3_data },
	{ MAX8698_LDO4,  &spica_ldo4_data },
	{ MAX8698_LDO5,  &spica_ldo5_data },
	{ MAX8698_LDO6,  &spica_ldo6_data },
	{ MAX8698_LDO7,  &spica_ldo7_data },
	{ MAX8698_LDO8,  &spica_ldo8_data },
	{ MAX8698_LDO9,  &spica_ldo9_data },
	{ MAX8698_BUCK1, &spica_buck1_data },
	{ MAX8698_BUCK2, &spica_buck2_data },
	{ MAX8698_BUCK3, &spica_buck3_data },
};

static struct max8698_platform_data spica_max8698_pdata = {
	.regulators	= spica_regulators,
	.num_regulators	= ARRAY_SIZE(spica_regulators),
};

static struct i2c_board_info spica_pmic_i2c_devs[] __initdata = {
	{
		.type		= "max8698",
		.addr		= 0x66,
		.platform_data	= &spica_max8698_pdata,
	},
};

/* I2C 3 (GPIO) -	MAX9877AERP-T (audio amplifier),
 *			AK4671EG-L (audio codec) */
static struct i2c_gpio_platform_data spica_audio_i2c_pdata = {
	.sda_pin		= GPIO_FM_I2C_SDA,
	.scl_pin		= GPIO_FM_I2C_SCL,
	.udelay			= 2, /* 250KHz */
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only	= 1,
};

static struct platform_device spica_audio_i2c = {
	.name			= "i2c-gpio",
	.id			= 3,
	.dev.platform_data	= &spica_audio_i2c_pdata,
};

static struct i2c_board_info spica_audio_i2c_devs[] __initdata = {
	{
		.type		= "ak4671",
		.addr		= 0x12,
	},
	{
		.type		= "max9877",
		.addr		= 0x4d,
	},
};

/* I2C 4 (GPIO) -	AT42QT5480-CU (touchscreen controller) */
static struct i2c_gpio_platform_data spica_touch_i2c_pdata = {
	.sda_pin		= GPIO_TOUCH_I2C_SDA,
	.scl_pin		= GPIO_TOUCH_I2C_SCL,
	.udelay			= 6, /* 83,3KHz */
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only	= 0,
};

static struct platform_device spica_touch_i2c = {
	.name			= "i2c-gpio",
	.id			= 4,
	.dev.platform_data	= &spica_touch_i2c_pdata,
};

static struct i2c_board_info spica_touch_i2c_devs[] __initdata = {
	{
		.type		= "qt5480",
		.addr		= 0x30,
	},
};

/*
 * Android PMEM
 */

#ifdef CONFIG_ANDROID_PMEM
static struct android_pmem_platform_data pmem_pdata = {
	.name		= "pmem",
	.no_allocator	= 1,
	.cached		= 1,
	.buffered	= 1,
	.start		= RESERVED_PMEM_START,
	.size		= RESERVED_PMEM,
};

static struct android_pmem_platform_data pmem_gpu1_pdata = {
	.name		= "pmem_gpu1",
	.no_allocator	= 0,
	.cached		= 1,
	.buffered	= 1,
        .start		= GPU1_RESERVED_PMEM_START,
        .size		= RESERVED_PMEM_GPU1,
};

static struct android_pmem_platform_data pmem_render_pdata = {
	.name		= "pmem_render",
	.no_allocator	= 1,
	.cached		= 0,
        .start		= RENDER_RESERVED_PMEM_START,
        .size		= RESERVED_PMEM_RENDER,
};

static struct android_pmem_platform_data pmem_stream_pdata = {
	.name		= "pmem_stream",
	.no_allocator	= 1,
	.cached		= 0,
	.start		= STREAM_RESERVED_PMEM_START,
	.size		= RESERVED_PMEM_STREAM,
};

static struct android_pmem_platform_data pmem_preview_pdata = {
	.name		= "pmem_preview",
	.no_allocator	= 1,
	.cached		= 0,
        .start		= PREVIEW_RESERVED_PMEM_START,
        .size		= RESERVED_PMEM_PREVIEW,
};

static struct android_pmem_platform_data pmem_picture_pdata = {
	.name		= "pmem_picture",
	.no_allocator	= 1,
	.cached		= 0,
        .start		= PICTURE_RESERVED_PMEM_START,
        .size		= RESERVED_PMEM_PICTURE,
};

static struct android_pmem_platform_data pmem_jpeg_pdata = {
	.name		= "pmem_jpeg",
	.no_allocator	= 1,
	.cached		= 0,
        .start		= JPEG_RESERVED_PMEM_START,
        .size		= RESERVED_PMEM_JPEG,
};

static struct android_pmem_platform_data pmem_skia_pdata = {
	.name		= "pmem_skia",
	.no_allocator	= 1,
	.cached		= 0,
        .start		= SKIA_RESERVED_PMEM_START,
        .size		= RESERVED_PMEM_SKIA,
};

static struct platform_device pmem_device = {
	.name		= "android_pmem",
	.id		= 0,
	.dev		= { .platform_data = &pmem_pdata },
};
 
static struct platform_device pmem_gpu1_device = {
	.name		= "android_pmem",
	.id		= 1,
	.dev		= { .platform_data = &pmem_gpu1_pdata },
};

static struct platform_device pmem_render_device = {
	.name		= "android_pmem",
	.id		= 2,
	.dev		= { .platform_data = &pmem_render_pdata },
};

static struct platform_device pmem_stream_device = {
	.name		= "android_pmem",
	.id		= 3,
	.dev		= { .platform_data = &pmem_stream_pdata },
};

static struct platform_device pmem_preview_device = {
	.name		= "android_pmem",
	.id		= 5,
	.dev		= { .platform_data = &pmem_preview_pdata },
};

static struct platform_device pmem_picture_device = {
	.name		= "android_pmem",
	.id		= 6,
	.dev		= { .platform_data = &pmem_picture_pdata },
};

static struct platform_device pmem_jpeg_device = {
	.name		= "android_pmem",
	.id		= 7,
	.dev		= { .platform_data = &pmem_jpeg_pdata },
};

static struct platform_device pmem_skia_device = {
	.name		= "android_pmem",
	.id		= 8,
	.dev		= { .platform_data = &pmem_skia_pdata },
};

static struct platform_device *pmem_devices[] = {
	&pmem_device,
	&pmem_gpu1_device,
	&pmem_render_device,
	&pmem_stream_device,
	&pmem_preview_device,
	&pmem_picture_device,
	&pmem_jpeg_device,
	&pmem_skia_device
};

static void __init spica_add_mem_devices(void)
{
	unsigned i;
	for (i = 0; i < ARRAY_SIZE(pmem_devices); ++i)
		if (pmem_devices[i]->dev.platform_data) {
			struct android_pmem_platform_data *pmem =
					pmem_devices[i]->dev.platform_data;

			if (pmem->size)
				platform_device_register(pmem_devices[i]);
		}
}
#endif

/*
 * LCD screen
 */
static struct s6d05a_platform_data spica_s6d05a_pdata = {
	.reset_gpio	= GPIO_LCD_RST_N,
	.cs_gpio	= GPIO_LCD_CS_N,
	.sck_gpio	= GPIO_LCD_SCLK,
	.sda_gpio	= GPIO_LCD_SDI,
	.vci_regulator	= "lcd_vci",
	.vdd3_regulator	= "lcd_vdd3",
};

static struct platform_device spica_s6d05a = {
	.name		= "s6d05a-lcd",
	.id		= -1,
	.dev		= {
		.platform_data		= &spica_s6d05a_pdata,
	},
};

/*
 * SDHCI platform data
 */

static struct regulator *spica_tf_regulator;

static irqreturn_t spica_tf_cd_thread(int irq, void *dev_id)
{
	void (*notify_func)(struct platform_device *, int state) = dev_id;
	int status = gpio_get_value(GPIO_TF_DETECT);

	if (!status)
		regulator_enable(spica_tf_regulator);

	notify_func(&s3c_device_hsmmc0, !status);

	if (status)
		regulator_disable(spica_tf_regulator);

	return IRQ_HANDLED;
}

static int spica_tf_cd_init(void (*notify_func)(struct platform_device *,
								int state))
{
	int ret, status;

	ret = gpio_request(GPIO_TF_DETECT, "TF detect");
	if (ret)
		return ret;

	ret = gpio_direction_input(GPIO_TF_DETECT);
	if (ret)
		return ret;

	spica_tf_regulator = regulator_get(&s3c_device_hsmmc0.dev, "tf_vdd3");
	if (IS_ERR(spica_tf_regulator))
		return PTR_ERR(spica_tf_regulator);

	ret = request_threaded_irq(gpio_to_irq(GPIO_TF_DETECT), NULL,
		spica_tf_cd_thread, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
		"TF card detect", notify_func);
	if (ret)
		return ret;

	status = gpio_get_value(GPIO_TF_DETECT);
	if (status)
		regulator_disable(spica_tf_regulator);
	else
		regulator_enable(spica_tf_regulator);
	notify_func(&s3c_device_hsmmc0, !status);

	return 0;
}

static int spica_tf_cd_cleanup(void (*notify_func)(struct platform_device *,
								int state))
{
	free_irq(gpio_to_irq(GPIO_TF_DETECT), notify_func);
	regulator_disable(spica_tf_regulator);
	gpio_free(GPIO_TF_DETECT);

	return 0;
}

struct s3c_sdhci_platdata spica_hsmmc0_pdata = {
	.cd_type	= S3C_SDHCI_CD_EXTERNAL,
	.ext_cd_init	= spica_tf_cd_init,
	.ext_cd_cleanup	= spica_tf_cd_cleanup,
};

struct s3c_sdhci_platdata spica_hsmmc2_pdata = {
	.cd_type	= S3C_SDHCI_CD_PERMANENT,
};

/*
 * Framebuffer
 */

static struct s3c_fb_pd_win spica_fb_win0 = {
	/* this is to ensure we use win0 */
	.win_mode	= {
		.left_margin	= 10,
		.right_margin	= 10,
		.upper_margin	= 3,
		.lower_margin	= 8,
		.hsync_len	= 10,
		.vsync_len	= 2,
		.xres		= 320,
		.yres		= 480,
	},
	.max_bpp	= 32,
	.default_bpp	= 16,
	.virtual_y	= 480 * 2,
	.virtual_x	= 320,
};

static void spica_fb_gpio_setup_18bpp(void)
{
	/* Blue */
	s3c_gpio_cfgrange_nopull(GPIO_LCD_B_0, 6, S3C_GPIO_SFN(2));
	/* Green */
	s3c_gpio_cfgrange_nopull(GPIO_LCD_G_0, 6, S3C_GPIO_SFN(2));
	/* Red + control signals */
	s3c_gpio_cfgrange_nopull(GPIO_LCD_R_0, 10, S3C_GPIO_SFN(2));
}

static struct s3c_fb_platdata spica_lcd_pdata __initdata = {
	.setup_gpio	= spica_fb_gpio_setup_18bpp,
	.win[0]		= &spica_fb_win0,
	.vidcon0	= VIDCON0_VIDOUT_RGB | VIDCON0_PNRMODE_RGB,
	.vidcon1	= VIDCON1_INV_HSYNC | VIDCON1_INV_VSYNC
			| VIDCON1_INV_VCLK,
};

/*
 * Keyboard
 */

/* S3C6410 matrix keypad controller */

static uint32_t spica_keymap[] __initdata = {
	/* KEY(row, col, keycode) */
	KEY(0, 0, 201), KEY(0, 1, 209) /* Reserved  */ /* Reserved  */,
	KEY(1, 0, 202), KEY(1, 1, 210), KEY(1, 2, 218), KEY(1, 3, 226),
	KEY(2, 0, 203), KEY(2, 1, 211), KEY(2, 2, 219), KEY(2, 3, 227),
	KEY(3, 0, 204), KEY(3, 1, 212) /* Reserved  */, KEY(3, 3, 228),
};

static struct matrix_keymap_data spica_keymap_data __initdata = {
	.keymap		= spica_keymap,
	.keymap_size	= ARRAY_SIZE(spica_keymap),
};

static struct samsung_keypad_platdata spica_keypad_pdata __initdata = {
	.keymap_data	= &spica_keymap_data,
	.rows		= 4,
	.cols		= 4,
};

/* GPIO keys */

static struct gpio_keys_button spica_gpio_keys_data[] = {
	{
		.gpio			= GPIO_POWER_N,
		.code			= 249,
		.desc			= "Power",
		.active_low		= 1,
		.debounce_interval	= 5,
		.type                   = EV_KEY,
		.wakeup			= 1,
	},
	{
		.gpio			= GPIO_HOLD_KEY_N,
		.code			= 217,
		.desc			= "Hold",
		.active_low		= 1,
		.debounce_interval	= 5,
		.type                   = EV_KEY,
		.wakeup			= 1,
	},
};

static struct gpio_keys_platform_data spica_gpio_keys_pdata  = {
	.buttons	= spica_gpio_keys_data,
	.nbuttons	= ARRAY_SIZE(spica_gpio_keys_data),
};

static struct platform_device spica_gpio_keys = {
	.name		= "gpio-keys",
	.id		= 0,
	.num_resources	= 0,
	.dev		= {
		.platform_data	= &spica_gpio_keys_pdata,
	}
};

/*
 * Platform devices
 */

static struct platform_device *spica_devices[] __initdata = {
	&s3c_device_hsmmc0,
	&s3c_device_hsmmc2,
	&s3c_device_rtc,
	&s3c_device_i2c0,
	&s3c_device_i2c1,
	&s3c_device_fb,
	&s3c_device_usb_hsotg,
	&s3c_device_onenand,
	&samsung_device_keypad,
	&spica_pmic_i2c,
	&spica_audio_i2c,
	&spica_touch_i2c,
	&spica_s6d05a,
	&spica_android_usb,
	&spica_usb_mass_storage,
	&spica_usb_rndis,
	&spica_gpio_keys,
};

/*
 * Extended I/O map
 */

static struct map_desc spica_iodesc[] __initdata = {
};

/*
 * GPIO setup
 */

static struct s3c_gpio_config spica_gpio_table[] = {
	/*
	 * OFF PART
	 */

	/* GPA */
	{ GPIO_FLM_RXD, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_FLM_TXD, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_MSENSE_RST, 1, GPIO_LEVEL_HIGH, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT1, S3C_GPIO_PULL_NONE },
	{ GPIO_BT_RXD, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_BT_TXD, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_BT_CTS, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_BT_RTS, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },

	/* GPB */
	{ GPIO_PDA_RXD, GPIO_PDA_RXD_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_PDA_TXD, GPIO_PDA_TXD_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_I2C1_SCL, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_I2C1_SDA, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_TOUCH_EN, 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT1, S3C_GPIO_PULL_NONE },
	{ GPIO_I2C0_SCL, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
	{ GPIO_I2C0_SDA, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },

	/* GPC */
	{ GPIO_PM_SET1, GPIO_PM_SET1_AF, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_PM_SET2, GPIO_PM_SET2_AF, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_PM_SET3, GPIO_PM_SET3_AF, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_WLAN_CMD, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
	{ GPIO_WLAN_CLK, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_WLAN_WAKE, 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_BT_WAKE, 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },

	/* GPD */
	{ GPIO_I2S_CLK, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_BT_WLAN_REG_ON, 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_I2S_LRCLK, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_I2S_DI, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_I2S_DO, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },

	/* GPE */
	{ GPIO_BT_RST_N, 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_BOOT, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
	{ GPIO_WLAN_RST_N, 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_PWR_I2C_SCL, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
	{ GPIO_PWR_I2C_SDA, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },

	/* GPF */
	{ GPIO_CAM_MCLK, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_CAM_HSYNC, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_CAM_PCLK, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_MCAM_RST_N, 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_CAM_VSYNC, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_CAM_D_0, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_CAM_D_1, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_CAM_D_2, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_CAM_D_3, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_CAM_D_4, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_CAM_D_5, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_CAM_D_6, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_CAM_D_7, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_VIBTONE_PWM, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },

	/* GPG */
	{ GPIO_TF_CLK, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_TF_CMD, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
	{ GPIO_TF_D_0, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
	{ GPIO_TF_D_1, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
	{ GPIO_TF_D_2, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
	{ GPIO_TF_D_3, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },

	/* GPH */
	{ GPIO_TOUCH_I2C_SCL, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
	{ GPIO_TOUCH_I2C_SDA, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
	{ GPIO_FM_I2C_SCL, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
	{ GPIO_FM_I2C_SDA, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
	{ GPIO_VIB_EN, 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_WLAN_D_0, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
	{ GPIO_WLAN_D_1, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
	{ GPIO_WLAN_D_2, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
	{ GPIO_WLAN_D_3, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },

	/* GPI */
	{ GPIO_LCD_B_0, GPIO_LCD_B_0_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_B_1, GPIO_LCD_B_1_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_B_2, GPIO_LCD_B_2_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_B_3, GPIO_LCD_B_3_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_B_4, GPIO_LCD_B_4_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_B_5, GPIO_LCD_B_5_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_G_0, GPIO_LCD_G_0_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_G_1, GPIO_LCD_G_1_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_G_2, GPIO_LCD_G_2_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_G_3, GPIO_LCD_G_3_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_G_4, GPIO_LCD_G_4_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_G_5, GPIO_LCD_G_5_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },

	/* GPJ */
	{ GPIO_LCD_R_0, GPIO_LCD_R_0_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_R_1, GPIO_LCD_R_1_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_R_2, GPIO_LCD_R_2_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_R_3, GPIO_LCD_R_3_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_R_4, GPIO_LCD_R_4_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_R_5, GPIO_LCD_R_5_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_HSYNC, GPIO_LCD_HSYNC_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_VSYNC, GPIO_LCD_VSYNC_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_DE, GPIO_LCD_DE_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_CLK, GPIO_LCD_CLK_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },

	/*
	 * ALIVE PART
	 */

	/* GPK */
	{ GPIO_TA_EN, GPIO_TA_EN_AF, GPIO_LEVEL_HIGH, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_AUDIO_EN, GPIO_AUDIO_EN_AF, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_PHONE_ON, GPIO_PHONE_ON_AF, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_MICBIAS_EN, GPIO_MICBIAS_EN_AF, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_TOUCH_RST, GPIO_TOUCH_RST_AF, GPIO_LEVEL_HIGH, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_CAM_EN, GPIO_CAM_EN_AF, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_PHONE_RST_N, GPIO_PHONE_RST_N_AF, GPIO_LEVEL_HIGH, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_KEYSCAN_0, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_KEYSCAN_1, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_KEYSCAN_2, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_KEYSCAN_3, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ S3C64XX_GPK(12), 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, 0, 0 },
	{ S3C64XX_GPK(13), 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, 0, 0 },
	{ S3C64XX_GPK(14), 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_VREG_MSMP_26V, GPIO_VREG_MSMP_26V_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },

	/* GPL */
	{ GPIO_KEYSENSE_0, 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_KEYSENSE_1, 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_KEYSENSE_2, 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_KEYSENSE_3, 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_USIM_BOOT, GPIO_USIM_BOOT_AF, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_CAM_3M_STBY_N, GPIO_CAM_3M_STBY_N_AF, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_HOLD_KEY_N, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ S3C64XX_GPL(10), 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_TA_CONNECTED_N, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_TOUCH_INT_N, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_CP_BOOT_SEL, GPIO_CP_BOOT_SEL_AF, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_BT_HOST_WAKE, GPIO_BT_HOST_WAKE_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, 0, 0 },

	/* GPM */
	{ S3C64XX_GPM(0), 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, 0, 0 },
	{ S3C64XX_GPM(1), 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_TA_CHG_N, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_PDA_ACTIVE, GPIO_PDA_ACTIVE_AF, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, 0, 0 },
	{ S3C64XX_GPM(4), 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, 0, 0 },
	{ S3C64XX_GPM(5), 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, 0, 0 },

	/* GPN */
	{ GPIO_ONEDRAM_INT_N, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_WLAN_HOST_WAKE, GPIO_WLAN_HOST_WAKE_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, 0, 0 },
	{ GPIO_MSENSE_INT, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_ACC_INT, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_SIM_DETECT_N, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_POWER_N, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_TF_DETECT, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_PHONE_ACTIVE, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_PMIC_INT_N, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_JACK_INT_N, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_DET_35, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_EAR_SEND_END, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_RESOUT_N, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_BOOT_EINT13, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_BOOT_EINT14, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_BOOT_EINT15, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },

	/*
	 * MEMORY PART
	 */

	/* GPO */
	{ S3C64XX_GPO(4), 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ S3C64XX_GPO(5), 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },

	/* GPP */
	{ S3C64XX_GPP(8), 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ S3C64XX_GPP(10), 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ S3C64XX_GPP(14), 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },

	/* GPQ */
	{ S3C64XX_GPQ(2), 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ S3C64XX_GPQ(3), 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ S3C64XX_GPQ(4), 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ S3C64XX_GPQ(5), 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ S3C64XX_GPQ(6), 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
};

/*
 * Machine setup
 */

static void __init spica_fixup(struct machine_desc *desc,
		struct tag *tags, char **cmdline, struct meminfo *mi)
{
	mi->nr_banks = 1;

	mi->bank[0].start = PHYS_OFFSET;
	mi->bank[0].size = PHYS_UNRESERVED_SIZE;
}

static void __init spica_map_io(void)
{
	s3c64xx_init_io(spica_iodesc, ARRAY_SIZE(spica_iodesc));
	s3c24xx_init_clocks(12000000);
	s3c24xx_init_uarts(spica_uartcfgs, ARRAY_SIZE(spica_uartcfgs));
}

static void __init spica_machine_init(void)
{
	s3c_init_gpio(spica_gpio_table, ARRAY_SIZE(spica_gpio_table));

	/* Register I2C devices */
	s3c_i2c0_set_platdata(&spica_misc_i2c);
	i2c_register_board_info(spica_misc_i2c.bus_num, spica_misc_i2c_devs,
					ARRAY_SIZE(spica_misc_i2c_devs));
	s3c_i2c1_set_platdata(&spica_cam_i2c);
	i2c_register_board_info(spica_cam_i2c.bus_num, spica_cam_i2c_devs,
					ARRAY_SIZE(spica_cam_i2c_devs));
	i2c_register_board_info(spica_pmic_i2c.id, spica_pmic_i2c_devs,
					ARRAY_SIZE(spica_pmic_i2c_devs));
	i2c_register_board_info(spica_audio_i2c.id, spica_audio_i2c_devs,
					ARRAY_SIZE(spica_audio_i2c_devs));
	i2c_register_board_info(spica_touch_i2c.id, spica_touch_i2c_devs,
					ARRAY_SIZE(spica_touch_i2c_devs));

	s3c_fb_set_platdata(&spica_lcd_pdata);

	s3c_sdhci0_set_platdata(&spica_hsmmc0_pdata);
	s3c_sdhci2_set_platdata(&spica_hsmmc2_pdata);

	samsung_keypad_set_platdata(&spica_keypad_pdata);

	/* Register platform devices */
	platform_add_devices(spica_devices, ARRAY_SIZE(spica_devices));

#ifdef CONFIG_ANDROID_PMEM
	/* Register PMEM devices */
	spica_add_mem_devices();
#endif
}

/*
 * Machine definition
 */

MACHINE_START(GT_I5700, "Samsung GT-i5700 (Spica)")
	/* Maintainer: Currently none */
	.boot_params	= S3C64XX_PA_SDRAM + 0x100,
	.init_irq	= s3c6410_init_irq,
	.fixup		= spica_fixup,
	.map_io		= spica_map_io,
	.init_machine	= spica_machine_init,
	.timer		= &s3c24xx_timer,
MACHINE_END
