/*
 * Copyright (c) 2018, NVIDIA Corporation. All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

/**
 * @file nvg.c
 */

#include "build_config.h"
#include <stdint.h>
#include <inttypes.h>
#include <t194_nvg.h>
#include <tegrabl_debug.h>
#include <tegrabl_armv8a.h>
#include <tegrabl_t194_ccplex_nvg.h>

#define NVG_VERSION_FLAT(major, minor)	(((uint64_t)major << 32) + (uint64_t)minor)

void tegrabl_ccplex_nvg_get_hw_version(uint32_t *major, uint32_t *minor)
{
	nvg_version_data_t data;

	tegrabl_write_nvg_channel_idx((uint32_t)TEGRA_NVG_CHANNEL_VERSION);
	data.flat = tegrabl_read_nvg_channel_data();

	if (major != NULL) {
		*major = data.bits.major_version;
	}
	if (minor != NULL) {
		*minor = data.bits.minor_version;
	}
}

uint32_t tegrabl_ccplex_nvg_num_cores(void)
{
	nvg_num_cores_channel_t data;

	tegrabl_write_nvg_channel_idx((uint32_t)TEGRA_NVG_CHANNEL_NUM_CORES);
	data.flat = tegrabl_read_nvg_channel_data();

	return data.bits.num_cores;
}

uint32_t tegrabl_ccplex_nvg_logical_to_mpidr(uint32_t core)
{
	uint32_t num_cores;
	uint32_t ret = 0;
	nvg_logical_to_mpidr_channel_t ch_data;

	num_cores = tegrabl_ccplex_nvg_num_cores();
	if (core < num_cores) {
		tegrabl_write_nvg_channel_idx((uint32_t)TEGRA_NVG_CHANNEL_LOGICAL_TO_MPIDR);

		/* Write the logical core id */
		ch_data.flat = 0x0ULL;
		ch_data.write.lcore_id = core;
		tegrabl_write_nvg_channel_data(ch_data.flat);

		/* Read-back the MPIDR */
		ch_data.flat = tegrabl_read_nvg_channel_data();

		pr_info("NVG: Logical CPU: %u; MPIDR: 0x%x\n", core, ch_data.read.mpidr);

		ret = ch_data.read.mpidr;
	} else {
		pr_error("Core: %u is not present\n", core);
	}

	return ret;
}
