/*
 * TCA9539 16-bit I2C I/O Expander Driver
 *
 * Copyright (c) 2017-2018, NVIDIA CORPORATION. All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#ifndef _TEGRABL_TCA_PLAT_CONFIG_H_
#define _TEGRABL_TCA_PLAT_CONFIG_H_

#define I2C2_NODE	"/i2c@c240000"

static struct tca9539_driver_property tca9539_chips[] = {
	{
		.chip_id = TEGRA_GPIO_TCA9539_CHIPID_BASE,
		.i2c_inst = TEGRABL_INSTANCE_I2C2,
		.i2c_name = I2C2_NODE,
		.i2c_addr = 0x23,
	},
	{
		.chip_id = TEGRA_GPIO_TCA9539_CHIPID_BASE + 1,
		.i2c_inst = TEGRABL_INSTANCE_I2C2,
		.i2c_name = I2C2_NODE,
		.i2c_addr = 0x22,
	},
};

#endif
