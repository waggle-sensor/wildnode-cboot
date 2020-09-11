/*
 * Copyright (c) 2017-2019, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited
 */

#define MODULE TEGRABL_ERR_I2C

#include <stdint.h>
#include <tegrabl_utils.h>
#include <tegrabl_error.h>
#include <tegrabl_debug.h>
#include <tegrabl_io.h>
#include <tegrabl_addressmap.h>
#include <tegrabl_i2c_local.h>
#include <tegrabl_i2c_err_aux.h>
#include <tegrabl_i2c_soc_common.h>
#include <tegrabl_dpaux.h>
#include <tegrabl_device_prod.h>

#define I2C_LIMIT 65536
#define I2C_SOURCE_FREQ (136 * 1000) /* KHz */

static struct i2c_soc_info i2c_info[] = {
	{
		.base_addr = NV_ADDRESS_MAP_I2C1_BASE,
		.clk_freq = STD_SPEED,
		.is_muxed_dpaux = false,
	},
	{
		.base_addr = NV_ADDRESS_MAP_I2C2_BASE,
		.clk_freq = STD_SPEED,
		.is_muxed_dpaux = false,
	},
	{
		.base_addr = NV_ADDRESS_MAP_I2C3_BASE,
		.clk_freq = STD_SPEED,
		.is_muxed_dpaux = false,
	},
	{
		.base_addr = NV_ADDRESS_MAP_I2C4_BASE,
		.clk_freq = STD_SPEED,
		.is_muxed_dpaux = true,
		.dpaux_instance = DPAUX_INSTANCE_1,
	},
	{
		.base_addr = NV_ADDRESS_MAP_I2C5_BASE,
		.clk_freq = STD_SPEED,
		.is_bpmpfw_controlled = true,
		.is_cldvfs_required = true,
		.is_muxed_dpaux = false,
	},
	{
		.base_addr = NV_ADDRESS_MAP_I2C6_BASE,
		.clk_freq = STD_SPEED,
		.is_muxed_dpaux = true,
		.dpaux_instance = DPAUX_INSTANCE_0,
	},
	{
		.base_addr = NV_ADDRESS_MAP_I2C7_BASE,
		.clk_freq = STD_SPEED,
		.is_muxed_dpaux = true,
		.dpaux_instance = DPAUX_INSTANCE_2,
	},
	{
		.base_addr = NV_ADDRESS_MAP_I2C8_BASE,
		.clk_freq = STD_SPEED,
		.is_muxed_dpaux = false,
	},
	{
		.base_addr = NV_ADDRESS_MAP_I2C9_BASE,
		.clk_freq = STD_SPEED,
		.is_muxed_dpaux = true,
		.dpaux_instance = DPAUX_INSTANCE_3,
	},
	{
		.base_addr = NV_ADDRESS_MAP_I2C10_BASE,
		.clk_freq = STD_SPEED,
		.is_muxed_dpaux = false,
	}
};

void i2c_get_soc_info(struct i2c_soc_info **hi2c_info,
	uint32_t *num_of_instances)
{
	*hi2c_info = &i2c_info[0];
	*num_of_instances = ARRAY_SIZE(i2c_info);
}

uint32_t tegrabl_i2c_get_clk_source_rate(const struct tegrabl_i2c *hi2c)
{
	TEGRABL_UNUSED(hi2c);

	return I2C_SOURCE_FREQ;
}

void i2c_set_prod_settings(struct tegrabl_i2c *hi2c)
{
	char *mode = NULL;
	tegrabl_instance_i2c_t instance;
	tegrabl_error_t err;
	struct addr_range addr_range[2];

	TEGRABL_ASSERT(hi2c != NULL);

#if defined(CONFIG_POWER_I2C_BPMPFW)
	if (hi2c->is_enable_bpmpfw_i2c == true) {
		return;
	}
#endif

	instance = hi2c->instance;

	if (hi2c->clk_freq > FM_PLUS_SPEED) {
		mode = "hs";
	} else if (hi2c->clk_freq > FM_SPEED) {
		mode = "fmplus";
	} else if (hi2c->clk_freq > STD_SPEED) {
		mode = "fm";
	} else {
		mode = "sm";
	}

	addr_range[0].lower = i2c_info[instance].base_addr;
	addr_range[0].higher = i2c_info[instance].base_addr + I2C_LIMIT;
	addr_range[1].lower = 0;
	addr_range[1].higher = 0;

	pr_trace("instance %d speed %s\n", instance, mode);
	err = tegrabl_device_prod_configure(TEGRABL_MODULE_I2C, instance, addr_range, mode);
	TEGRABL_UNUSED(err);
}
