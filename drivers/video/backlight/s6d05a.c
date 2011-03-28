/*
 * drivers/video/backlight/s6d05a.c
 *
 * Copyright (C) 2011 Tomasz Figa <tomasz.figa at gmail.com>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 */

#include <linux/wait.h>
#include <linux/fb.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/leds.h>
#include <linux/spi/spi.h>
#include <linux/backlight.h>
#include <linux/regulator/consumer.h>

#include <video/s6d05a.h>

#include <mach/gpio.h>

#define BACKLIGHT_MODE_NORMAL	0x000
#define BACKLIGHT_MODE_ALC	0x100
#define BACKLIGHT_MODE_CABC	0x200

#define BACKLIGHT_LEVEL_VALUE	0x0FF

#define BACKLIGHT_LEVEL_MIN	0
#define BACKLIGHT_LEVEL_MAX	BACKLIGHT_LEVEL_VALUE

#define BACKLIGHT_LEVEL_DEFAULT	BACKLIGHT_LEVEL_MAX

/* Driver data */
struct s6d05a_data {
	struct backlight_device	*bl;

	unsigned cs_gpio;
	unsigned sck_gpio;
	unsigned sda_gpio;
	unsigned reset_gpio;
	struct regulator *vdd3;
	struct regulator *vci;

	int state;
	int brightness;

	struct s6d05a_command *power_on_seq;
	u32 power_on_seq_len;

	struct s6d05a_command *power_off_seq;
	u32 power_off_seq_len;
};

/*
 * Power on command sequence
 */

static struct s6d05a_command s6d05a_power_on_seq[] = {
	{ PWRCTL,  9,  { 0x00, 0x00, 0x2A, 0x00, 0x00, 0x33, 0x29, 0x29, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   0 },
	{ SLPOUT,  0,  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },  15 },

	{ DISCTL,  11, { 0x16, 0x16, 0x0F, 0x0A, 0x05, 0x0A, 0x05, 0x10, 0x00, 0x16, 0x16, 0x00, 0x00, 0x00, 0x00 },   0 },
	{ PWRCTL,  9,  { 0x00, 0x01, 0x2A, 0x00, 0x00, 0x33, 0x29, 0x29, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   0 },
	{ VCMCTL,  5,  { 0x1A, 0x1A, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   0 },
	{ SRCCTL,  6,  { 0x00, 0x00, 0x0A, 0x01, 0x01, 0x1D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   0 },
	{ GATECTL, 3,  { 0x44, 0x3B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },  15 },

	{ PWRCTL,  9,  { 0x00, 0x03, 0x2A, 0x00, 0x00, 0x33, 0x29, 0x29, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },  15 },
	{ PWRCTL,  9,  { 0x00, 0x07, 0x2A, 0x00, 0x00, 0x33, 0x29, 0x29, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },  15 },
	{ PWRCTL,  9,  { 0x00, 0x0F, 0x2A, 0x00, 0x02, 0x33, 0x29, 0x29, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },  15 },
	{ PWRCTL,  9,  { 0x00, 0x1F, 0x2A, 0x00, 0x02, 0x33, 0x29, 0x29, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },  15 },
	{ PWRCTL,  9,  { 0x00, 0x3F, 0x2A, 0x00, 0x08, 0x33, 0x29, 0x29, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },  25 },
	{ PWRCTL,  9,  { 0x00, 0x7F, 0x2A, 0x00, 0x08, 0x33, 0x29, 0x29, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },  35 },

	{ MADCTL,  1,  { 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   0 },
	{ COLMOD,  1,  { 0x66, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   0 },
	{ GAMCTL1, 15, { 0x00, 0x00, 0x00, 0x14, 0x27, 0x2D, 0x2C, 0x2D, 0x10, 0x11, 0x10, 0x16, 0x04, 0x22, 0x22 },   0 },
	{ GAMCTL2, 15, { 0x00, 0x00, 0x00, 0x14, 0x27, 0x2D, 0x2C, 0x2D, 0x10, 0x11, 0x10, 0x16, 0x04, 0x22, 0x22 },   0 },
	{ GAMCTL3, 15, { 0x96, 0x00, 0x00, 0x00, 0x00, 0x15, 0x1E, 0x23, 0x16, 0x0D, 0x07, 0x10, 0x00, 0x81, 0x42 },   0 },
	{ GAMCTL4, 15, { 0x80, 0x16, 0x00, 0x00, 0x00, 0x15, 0x1E, 0x23, 0x16, 0x0D, 0x07, 0x10, 0x00, 0x81, 0x42 },   0 },
	{ GAMCTL5, 15, { 0x00, 0x00, 0x34, 0x30, 0x2F, 0x2F, 0x2E, 0x2F, 0x0E, 0x0D, 0x09, 0x0E, 0x00, 0x22, 0x12 },   0 },
	{ GAMCTL6, 15, { 0x00, 0x00, 0x34, 0x30, 0x2F, 0x2F, 0x2E, 0x2F, 0x0E, 0x0D, 0x09, 0x0E, 0x00, 0x22, 0x12 },   0 },
	{ BCMODE,  1,  { 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   0 },
	{ MIECTL3, 2,  { 0x7C, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   0 },
	{ WRDISBV, 1,  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   0 },
//	{ MIECTL1, 3,  { 0x80, 0x80, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   0 },
//	{ MIECTL2, 3,  { 0x20, 0x01, 0x8F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   0 },
//	{ WRCABC,  1,  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   0 },
	{ DCON,    1,  { 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },  40 },

	{ DCON,    1,  { 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   0 },
	{ WRCTRLD, 1,  { 0x2C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   0 },
};

/*
 *	Power off command sequence
 */

static struct s6d05a_command s6d05a_power_off_seq[] = {
	{    DCON,  1, { 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },  40 },
	{    DCON,  1, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },  25 },
	{  PWRCTL,  9, { 0x00, 0x00, 0x2A, 0x00, 0x00, 0x33, 0x29, 0x29, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   0 },
	{   SLPIN,  0, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 200 },
};

/*
 *	Hardware interface
 */

#define S6D05A_SPI_DELAY_USECS	10

static inline void s6d05a_write_word(struct s6d05a_data *data, u16 word)
{
	unsigned mask = 1 << 8;

	/* Make sure chip select is not asserted */
	gpio_set_value(data->cs_gpio, 1);
	udelay(S6D05A_SPI_DELAY_USECS);

	/* Clock starts with high level */
	gpio_set_value(data->sck_gpio, 1);
	udelay(S6D05A_SPI_DELAY_USECS);

	/* Assert chip select */
	gpio_set_value(data->cs_gpio, 0);
	udelay(S6D05A_SPI_DELAY_USECS);

	do {
		/* Clock low */
		gpio_set_value(data->sck_gpio, 0);
		udelay(S6D05A_SPI_DELAY_USECS);

		/* Output data */
		gpio_set_value(data->sda_gpio, word & mask);
		udelay(S6D05A_SPI_DELAY_USECS);

		/* Clock high */
		gpio_set_value(data->sck_gpio, 1);
		udelay(S6D05A_SPI_DELAY_USECS);

		mask >>= 1;
	} while (mask);

	/* Deassert chip select */
	gpio_set_value(data->cs_gpio, 1);
	udelay(S6D05A_SPI_DELAY_USECS);
}

static inline void s6d05a_send_command(struct s6d05a_data *data,
						struct s6d05a_command *cmd)
{
	unsigned i;

	/* Send command */
	s6d05a_write_word(data, cmd->command);

	/* Send parameters */
	for (i = 0; i < cmd->param_count; ++i)
		s6d05a_write_word(data, cmd->parameter[i] | 0x100);

	/* Wait if needed */
	if (cmd->delay_msecs)
		msleep(cmd->delay_msecs);
}

static void s6d05a_send_command_seq(struct s6d05a_data *data,
					struct s6d05a_command *cmd, int count)
{
	while (count--)
		s6d05a_send_command(data, cmd++);
}

/*
 *	Backlight interface
 */

static void s6d05a_set_power(struct s6d05a_data *data, int power)
{
	if (power) {
		/* Power On Sequence */

		/* Assert reset */
		gpio_set_value(data->reset_gpio, 0);
		udelay(10);

		/* Enable VCI if needed */
		if (data->vci)
			regulator_enable(data->vci);
		udelay(10);

		/* Enable VDD3 if needed */
		if (data->vdd3)
			regulator_enable(data->vdd3);
		udelay(10);

		/* Release reset */
		gpio_set_value(data->reset_gpio, 1);

		/* Wait at least 10 ms */
		msleep(10);

		/* Send power on command sequence */
		s6d05a_send_command_seq(data, data->power_on_seq,
						data->power_on_seq_len);
	} else {
		/* Power Off Sequence */

		/* Send power off command sequence */
		s6d05a_send_command_seq(data, data->power_off_seq,
						data->power_off_seq_len);

		/* Reset Assert */
		gpio_set_value(data->reset_gpio, 0);


		/* Disable VDD3 if possible */
		if (data->vdd3)
			regulator_disable(data->vdd3);

		/* Disable VCI if possible */
		if (data->vci)
			regulator_disable(data->vci);
	}

	data->state = power;
}

static void s6d05a_set_backlight(struct s6d05a_data *data, u8 value)
{
	struct s6d05a_command cmd = { WRDISBV, 1, { value, }, 0 };

	s6d05a_send_command(data, &cmd);
}

static int s6d05a_bl_update_status(struct backlight_device *bl)
{
	struct s6d05a_data *data = bl_get_data(bl);
	int new_state = 1;

	if (bl->props.power != FB_BLANK_UNBLANK)
		new_state = 0;
	if (bl->props.state & BL_CORE_FBBLANK)
		new_state = 0;
	if (bl->props.state & BL_CORE_SUSPENDED)
		new_state = 0;

	data->brightness = bl->props.brightness;

	if (new_state == data->state) {
		s6d05a_set_backlight(data, data->brightness);
		return 0;
	}

	s6d05a_set_power(data, new_state);

	if (new_state)
		s6d05a_set_backlight(data, data->brightness);

	return 0;
}

static int s6d05a_bl_get_brightness(struct backlight_device *bl)
{
	struct s6d05a_data *data = bl_get_data(bl);

	if (!data->state)
		return 0;

	return data->brightness;
}

static struct backlight_ops s6d05a_bl_ops = {
	.options	= BL_CORE_SUSPENDRESUME,
	.update_status	= s6d05a_bl_update_status,
	.get_brightness	= s6d05a_bl_get_brightness,
};

static struct backlight_properties s6d05a_bl_props = {
	.max_brightness	= BACKLIGHT_LEVEL_MAX,
};

/*
 * SPI driver
 */

static int __devinit s6d05a_probe(struct platform_device *pdev)
{
	struct s6d05a_data *data;
	struct s6d05a_platform_data *pdata = pdev->dev.platform_data;
	struct backlight_device *bl;
	struct regulator *vdd3, *vci;
	int ret = 0;

	if (!pdata)
		return -ENOENT;

	/* Configure GPIO */
	ret = gpio_request(pdata->reset_gpio, "s6d05a reset");
	if (ret)
		return ret;

	ret = gpio_direction_output(pdata->reset_gpio, 1);
	if (ret)
		goto err_reset;

	ret = gpio_request(pdata->cs_gpio, "s6d05a cs");
	if (ret)
		goto err_reset;

	ret = gpio_direction_output(pdata->cs_gpio, 1);
	if (ret)
		goto err_cs;

	ret = gpio_request(pdata->sck_gpio, "s6d05a sck");
	if (ret)
		goto err_cs;

	ret = gpio_direction_output(pdata->sck_gpio, 0);
	if (ret)
		goto err_sck;

	ret = gpio_request(pdata->sda_gpio, "s6d05a sda");
	if (ret)
		goto err_sck;

	ret = gpio_direction_output(pdata->sda_gpio, 0);
	if (ret)
		goto err_sda;

	/* Allocate memory for driver data */
	data = kzalloc(sizeof(struct s6d05a_data), GFP_KERNEL);
	if (data == NULL) {
		dev_err(&pdev->dev, "No memory for device state\n");
		ret = -ENOMEM;
		goto err_sda;
	}

	/* Register backlight device */
	bl = backlight_device_register(dev_driver_string(&pdev->dev),
			&pdev->dev, data, &s6d05a_bl_ops, &s6d05a_bl_props);
	if (IS_ERR(bl)) {
		dev_err(&pdev->dev, "Failed to register backlight\n");
		ret = PTR_ERR(bl);
		goto err2;
	}

	/* Set up driver data */
	data->reset_gpio = pdata->reset_gpio;
	data->cs_gpio = pdata->cs_gpio;
	data->sck_gpio = pdata->sck_gpio;
	data->sda_gpio = pdata->sda_gpio;
	data->bl = bl;
	data->power_on_seq = s6d05a_power_on_seq;
	data->power_on_seq_len = ARRAY_SIZE(s6d05a_power_on_seq);
	data->power_off_seq = s6d05a_power_off_seq;
	data->power_off_seq_len = ARRAY_SIZE(s6d05a_power_off_seq);

	/* Check for power on sequence override */
	if (pdata->power_on_seq) {
		data->power_on_seq = pdata->power_on_seq;
		data->power_on_seq_len = pdata->power_on_seq_len;
	}

	/* Check for power off sequence override */
	if (pdata->power_off_seq) {
		data->power_off_seq = pdata->power_off_seq;
		data->power_off_seq_len = pdata->power_off_seq_len;
	}

	if (pdata->vci_regulator) {
		vci = regulator_get(&pdev->dev, pdata->vci_regulator);
		if (!IS_ERR(vci))
			data->vci = vci;
	}

	if (pdata->vdd3_regulator) {
		vdd3 = regulator_get(&pdev->dev, pdata->vdd3_regulator);
		if (!IS_ERR(vdd3))
			data->vdd3 = vdd3;
	}

	platform_set_drvdata(pdev, data);

	/* Initialize the LCD */
	bl->props.power = FB_BLANK_UNBLANK;
	bl->props.brightness = BACKLIGHT_LEVEL_MAX;
	backlight_update_status(bl);

	return 0;

err2:
	kfree(data);
err_sda:
	gpio_free(pdata->sda_gpio);
err_sck:
	gpio_free(pdata->sck_gpio);
err_cs:
	gpio_free(pdata->cs_gpio);
err_reset:
	gpio_free(pdata->reset_gpio);

	return ret;
}

static int __devexit s6d05a_remove(struct platform_device *pdev)
{
	struct s6d05a_data *data = platform_get_drvdata(pdev);

	s6d05a_set_power(data, 0);
	backlight_device_unregister(data->bl);
	gpio_free(data->reset_gpio);
	gpio_free(data->cs_gpio);
	gpio_free(data->sck_gpio);
	gpio_free(data->sda_gpio);
	if (data->vci)
		regulator_put(data->vci);
	if (data->vdd3)
		regulator_put(data->vdd3);
	kfree(data);

	return 0;
}

#ifdef CONFIG_PM
static int s6d05a_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct s6d05a_data *data = platform_get_drvdata(pdev);

	s6d05a_set_power(data, 0);

	return 0;
}

static int s6d05a_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct s6d05a_data *data = platform_get_drvdata(pdev);

	s6d05a_bl_update_status(data->bl);

	return 0;
}

static struct dev_pm_ops s6d05a_pm_ops = {
	.suspend	= s6d05a_suspend,
	.resume		= s6d05a_resume,
};
#endif

static struct platform_driver s6d05a_driver = {
	.driver = {
		.name	= "s6d05a-lcd",
		.owner	= THIS_MODULE,
#ifdef CONFIG_PM
		.pm	= &s6d05a_pm_ops,
#endif
	},
	.probe		= s6d05a_probe,
	.remove		= s6d05a_remove,
};

/*
 * Module
 */

static int __init s6d05a_init(void)
{
	return platform_driver_register(&s6d05a_driver);
}
module_init(s6d05a_init);

static void __exit s6d05a_exit(void)
{
	platform_driver_unregister(&s6d05a_driver);
}
module_exit(s6d05a_exit);

MODULE_DESCRIPTION("S6D05A LCD Controller Driver");
MODULE_AUTHOR("Tomasz Figa <tomasz.figa at gmail.com>");
MODULE_LICENSE("GPL");
