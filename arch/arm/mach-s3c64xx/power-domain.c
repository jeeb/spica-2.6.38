/* linux/arch/arm/mach-s3c64xx/power-domain.c
 *
 * Copyright (c)	2011 Tomasz Figa <tomasz.figa at gmail.com>
 * 			2010 Samsung Electronics Co., Ltd.
 *			http://www.samsung.com/
 *
 * S3C64xx - Power domain support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/err.h>
#include <linux/io.h>
#include <linux/mutex.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <mach/map.h>
#include <mach/regs-sys.h>
#include <mach/regs-syscon-power.h>
#include <plat/devs.h>

struct s3c64xx_pd_data {
	struct regulator_desc desc;
	struct regulator_dev *dev;
	int microvolts;
	int ctrlbit;
};

static struct regulator_consumer_supply s3c64xx_domain_g_supply[] = {
	REGULATOR_SUPPLY("pd", "s3c-g3d"),
};

static struct regulator_consumer_supply s3c64xx_domain_v_supply[] = {
	REGULATOR_SUPPLY("pd", "s3c-mfc"),
};

static struct regulator_consumer_supply s3c64xx_domain_i_supply[] = {
	REGULATOR_SUPPLY("pd", "s3c-jpeg"),
	REGULATOR_SUPPLY("pd", "s3c-fimc"),
};

static struct regulator_consumer_supply s3c64xx_domain_p_supply[] = {
	REGULATOR_SUPPLY("pd", "s3c-g2d"),
	REGULATOR_SUPPLY("pd", "s3c-tvenc"),
	REGULATOR_SUPPLY("pd", "s3c-scaler"),
};

static struct regulator_consumer_supply s3c64xx_domain_f_supply[] = {
	REGULATOR_SUPPLY("pd", "s3c-rotator"),
	REGULATOR_SUPPLY("pd", "s3c-pp"),
	REGULATOR_SUPPLY("pd", "s3c-fb"),
};

static struct regulator_init_data s3c64xx_domain_g_data = {
	.constraints = {
		.valid_modes_mask	= REGULATOR_MODE_NORMAL,
		.valid_ops_mask		= REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies	= ARRAY_SIZE(s3c64xx_domain_g_supply),
	.consumer_supplies	= s3c64xx_domain_g_supply,
};

static struct regulator_init_data s3c64xx_domain_v_data = {
	.constraints = {
		.valid_modes_mask	= REGULATOR_MODE_NORMAL,
		.valid_ops_mask		= REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies	= ARRAY_SIZE(s3c64xx_domain_v_supply),
	.consumer_supplies	= s3c64xx_domain_v_supply,
};

static struct regulator_init_data s3c64xx_domain_i_data = {
	.constraints = {
		.valid_modes_mask	= REGULATOR_MODE_NORMAL,
		.valid_ops_mask		= REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies	= ARRAY_SIZE(s3c64xx_domain_i_supply),
	.consumer_supplies	= s3c64xx_domain_i_supply,
};

static struct regulator_init_data s3c64xx_domain_p_data = {
	.constraints = {
		.valid_modes_mask	= REGULATOR_MODE_NORMAL,
		.valid_ops_mask		= REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies	= ARRAY_SIZE(s3c64xx_domain_p_supply),
	.consumer_supplies	= s3c64xx_domain_p_supply,
};

static struct regulator_init_data s3c64xx_domain_f_data = {
	.constraints = {
		.valid_modes_mask	= REGULATOR_MODE_NORMAL,
		.valid_ops_mask		= REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies	= ARRAY_SIZE(s3c64xx_domain_f_supply),
	.consumer_supplies	= s3c64xx_domain_f_supply,
};

struct s3c64xx_pd_config {
	const char *supply_name;
	int microvolts;
	struct regulator_init_data *init_data;
	int ctrlbit;
};

static struct s3c64xx_pd_config s3c64xx_domain_g_pdata = {
	.supply_name = "domain_g",
	.microvolts = 5000000,
	.init_data = &s3c64xx_domain_g_data,
	.ctrlbit = S3C64XX_NORMALCFG_DOMAIN_G_ON,
};

static struct s3c64xx_pd_config s3c64xx_domain_v_pdata = {
	.supply_name = "domain_v",
	.microvolts = 5000000,
	.init_data = &s3c64xx_domain_v_data,
	.ctrlbit = S3C64XX_NORMALCFG_DOMAIN_V_ON,
};

static struct s3c64xx_pd_config s3c64xx_domain_i_pdata = {
	.supply_name = "domain_i",
	.microvolts = 5000000,
	.init_data = &s3c64xx_domain_i_data,
	.ctrlbit = S3C64XX_NORMALCFG_DOMAIN_I_ON,
};

static struct s3c64xx_pd_config s3c64xx_domain_p_pdata = {
	.supply_name = "domain_p",
	.microvolts = 5000000,
	.init_data = &s3c64xx_domain_p_data,
	.ctrlbit = S3C64XX_NORMALCFG_DOMAIN_P_ON,
};

static struct s3c64xx_pd_config s3c64xx_domain_f_pdata = {
	.supply_name = "domain_f",
	.microvolts = 5000000,
	.init_data = &s3c64xx_domain_f_data,
	.ctrlbit = S3C64XX_NORMALCFG_DOMAIN_F_ON,
};

struct platform_device s3c64xx_domain_g = {
	.name          = "reg-s3c64xx-pd",
	.id            = 0,
	.dev = {
		.platform_data = &s3c64xx_domain_g_pdata,
	},
};

struct platform_device s3c64xx_domain_v = {
	.name          = "reg-s3c64xx-pd",
	.id            = 1,
	.dev = {
		.platform_data = &s3c64xx_domain_v_pdata,
	},
};

struct platform_device s3c64xx_domain_i = {
	.name          = "reg-s3c64xx-pd",
	.id            = 2,
	.dev = {
		.platform_data = &s3c64xx_domain_i_pdata,
	},
};

struct platform_device s3c64xx_domain_p = {
	.name          = "reg-s3c64xx-pd",
	.id            = 3,
	.dev = {
		.platform_data = &s3c64xx_domain_p_pdata,
	},
};

struct platform_device s3c64xx_domain_f = {
	.name          = "reg-s3c64xx-pd",
	.id            = 4,
	.dev = {
		.platform_data = &s3c64xx_domain_f_pdata,
	},
};

static int s3c64xx_pd_pwr_done(int ctrl)
{
	unsigned int cnt;
	cnt = 1000;

	do {
		if (__raw_readl(S3C64XX_BLK_PWR_STAT) & ctrl)
			return 0;
		udelay(1);
	} while (cnt-- > 0);

	return -ETIME;
}

static int s3c64xx_pd_pwr_off(int ctrl)
{
	unsigned int cnt;
	cnt = 1000;

	do {
		if (!(__raw_readl(S3C64XX_BLK_PWR_STAT) & ctrl))
			return 0;
		udelay(1);
	} while (cnt-- > 0);

	return -ETIME;
}

static int s3c64xx_pd_ctrl(int ctrlbit, int enable)
{
	u32 pd_reg = __raw_readl(S3C64XX_NORMAL_CFG);

	if (enable) {
		__raw_writel((pd_reg | ctrlbit), S3C64XX_NORMAL_CFG);
		if (s3c64xx_pd_pwr_done(ctrlbit))
			return -ETIME;
	} else {
		__raw_writel((pd_reg & ~(ctrlbit)), S3C64XX_NORMAL_CFG);
		if (s3c64xx_pd_pwr_off(ctrlbit))
			return -ETIME;
	}
	return 0;
}

static int s3c64xx_pd_is_enabled(struct regulator_dev *dev)
{
	struct s3c64xx_pd_data *data = rdev_get_drvdata(dev);

	return (__raw_readl(S3C64XX_BLK_PWR_STAT) & data->ctrlbit) ? 1 : 0;
}

static int s3c64xx_pd_enable(struct regulator_dev *dev)
{
	struct s3c64xx_pd_data *data = rdev_get_drvdata(dev);
	int ret;

	ret = s3c64xx_pd_ctrl(data->ctrlbit, 1);
	if (ret < 0) {
		printk(KERN_ERR "failed to enable power domain\n");
		return ret;
	}

	return 0;
}

static int s3c64xx_pd_disable(struct regulator_dev *dev)
{
	struct s3c64xx_pd_data *data = rdev_get_drvdata(dev);
	int ret;

	ret = s3c64xx_pd_ctrl(data->ctrlbit, 0);
	if (ret < 0) {
		printk(KERN_ERR "faild to disable power domain\n");
		return ret;
	}

	return 0;
}

static int s3c64xx_pd_get_voltage(struct regulator_dev *dev)
{
	struct s3c64xx_pd_data *data = rdev_get_drvdata(dev);

	return data->microvolts;
}

static int s3c64xx_pd_list_voltage(struct regulator_dev *dev,
				      unsigned selector)
{
	struct s3c64xx_pd_data *data = rdev_get_drvdata(dev);

	if (selector != 0)
		return -EINVAL;

	return data->microvolts;
}

static struct regulator_ops s3c64xx_pd_ops = {
	.is_enabled = s3c64xx_pd_is_enabled,
	.enable = s3c64xx_pd_enable,
	.disable = s3c64xx_pd_disable,
	.get_voltage = s3c64xx_pd_get_voltage,
	.list_voltage = s3c64xx_pd_list_voltage,
};

static int __devinit reg_s3c64xx_pd_probe(struct platform_device *pdev)
{
	struct s3c64xx_pd_config *config = pdev->dev.platform_data;
	struct s3c64xx_pd_data *drvdata;
	int ret;

	drvdata = kzalloc(sizeof(struct s3c64xx_pd_data), GFP_KERNEL);
	if (drvdata == NULL) {
		dev_err(&pdev->dev, "Failed to allocate device data\n");
		ret = -ENOMEM;
		goto err;
	}

	drvdata->desc.name = kstrdup(config->supply_name, GFP_KERNEL);
	if (drvdata->desc.name == NULL) {
		dev_err(&pdev->dev, "Failed to allocate supply name\n");
		ret = -ENOMEM;
		goto err;
	}

	drvdata->desc.type = REGULATOR_VOLTAGE;
	drvdata->desc.owner = THIS_MODULE;
	drvdata->desc.ops = &s3c64xx_pd_ops;
	drvdata->desc.n_voltages = 1;

	drvdata->microvolts = config->microvolts;

	drvdata->ctrlbit = config->ctrlbit;

	drvdata->dev = regulator_register(&drvdata->desc, &pdev->dev,
					  config->init_data, drvdata);
	if (IS_ERR(drvdata->dev)) {
		ret = PTR_ERR(drvdata->dev);
		dev_err(&pdev->dev, "Failed to register regulator: %d\n", ret);
		goto err_name;
	}

	platform_set_drvdata(pdev, drvdata);

	dev_dbg(&pdev->dev, "%s supplying %duV\n", drvdata->desc.name,
		drvdata->microvolts);

	return 0;

err_name:
	kfree(drvdata->desc.name);
err:
	kfree(drvdata);
	return ret;
}

static int __devexit reg_s3c64xx_pd_remove(struct platform_device *pdev)
{
	struct s3c64xx_pd_data *drvdata = platform_get_drvdata(pdev);

	regulator_unregister(drvdata->dev);
	kfree(drvdata->desc.name);
	kfree(drvdata);

	return 0;
}

static struct platform_driver regulator_s3c64xx_pd_driver = {
	.probe		= reg_s3c64xx_pd_probe,
	.remove		= __devexit_p(reg_s3c64xx_pd_remove),
	.driver		= {
		.name		= "reg-s3c64xx-pd",
		.owner		= THIS_MODULE,
	},
};

static struct platform_device *s3c64xx_domains[] = {
	&s3c64xx_domain_g,
	&s3c64xx_domain_v,
	&s3c64xx_domain_i,
	&s3c64xx_domain_p,
	&s3c64xx_domain_f,
};

static int __init regulator_s3c64xx_pd_init(void)
{
	platform_add_devices(s3c64xx_domains, ARRAY_SIZE(s3c64xx_domains));
	return platform_driver_register(&regulator_s3c64xx_pd_driver);
}
subsys_initcall(regulator_s3c64xx_pd_init);

static void __exit regulator_s3c64xx_pd_exit(void)
{
	platform_driver_unregister(&regulator_s3c64xx_pd_driver);
}
module_exit(regulator_s3c64xx_pd_exit);

MODULE_AUTHOR("Tomasz Figa <tomasz.figa at gmail.com>");
MODULE_DESCRIPTION("S3C64xx power domain regulator");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:reg-s3c64xx-pd");
