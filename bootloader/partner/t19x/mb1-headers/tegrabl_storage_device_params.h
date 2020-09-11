/*
 * Copyright (c) 2017-2019, NVIDIA CORPORATION. All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#ifndef INCLUDED_TEGRABL_DEVICE_CONFIG_PARAMS_H
#define INCLUDED_TEGRABL_DEVICE_CONFIG_PARAMS_H

#include <stdint.h>
#include <stdbool.h>
#include <tegrabl_compiler.h>
#include <tegrabl_sdmmc_param.h>
#include <tegrabl_qspi_flash_param.h>
#include <tegrabl_ufs.h>
#include <tegrabl_sata_param.h>

/* add magic header similar to STOrage Device CONFig */
#define MAGIC_HEADER	0x570DC09FU

/* 8 is chosen here to make the array multiple of 8 bytes */
#define TEGRABL_MAX_STORAGE_DEVICES  8U

/* macro tegrabl boot device */
#define TEGRABL_BOOT_DEV_NONE                 0U
#define TEGRABL_BOOT_DEV_SDMMC_BOOT           1U
#define TEGRABL_BOOT_DEV_SDMMC_USER           2U
#define TEGRABL_BOOT_DEV_SDMMC_RPMB           3U
#define TEGRABL_BOOT_DEV_QSPI                 4U
#define TEGRABL_BOOT_DEV_SATA                 5U
#define TEGRABL_BOOT_DEV_UFS                  6U
#define TEGRABL_BOOT_DEV_UFS_USER             7U
#define TEGRABL_BOOT_DEV_SDCARD               8U
#define TEGRABL_BOOT_DEV_MAX                  9U
#define TEGRABL_BOOT_DEV_BOOT_DEVICE_SIZE     0x7fffffffU


TEGRABL_PACKED(
struct tegrabl_device {
	uint8_t type;
	uint8_t instance;
}
);

TEGRABL_PACKED(
struct tegrabl_device_config_qspi_flash_params {
	uint32_t magic_header;
	uint32_t clk_src;
	uint32_t clk_div;
	uint32_t clk_src_freq;
	uint32_t interface_freq;
	uint32_t max_bus_width;
	bool enable_ddr_read;
	uint32_t dma_type;
	uint32_t fifo_access_mode;
	uint32_t read_dummy_cycles;
	uint32_t trimmer1_val;
	uint32_t trimmer2_val;
	uint8_t reserved[8];
}
);

TEGRABL_PACKED(
struct tegrabl_device_config_sdmmc_params {
	uint32_t magic_header;
	uint32_t clk_src;
	uint32_t clk_freq;
	uint32_t best_mode;
	uint32_t pd_offset;
	uint32_t pu_offset;
	bool dqs_trim_hs400;
	bool enable_strobe_hs400;
	uint8_t reserved[8];
}
);

TEGRABL_PACKED(
struct tegrabl_device_config_sata_params {
	uint32_t magic_header;
	uint8_t transfer_speed;
	uint8_t reserved[8];
}
);

TEGRABL_PACKED(
struct tegrabl_device_config_ufs_params {
	uint32_t magic_header;
	uint8_t max_hs_mode;
	uint8_t max_pwm_mode;
	uint8_t max_active_lanes;
	uint32_t page_align_size;
	bool enable_hs_modes;
	bool enable_fast_auto_mode;
	bool enable_hs_rate_b;
	bool enable_hs_rate_a;
	bool skip_hs_mode_switch;
	uint8_t reserved[8];
}
);

TEGRABL_PACKED(
struct tegrabl_device_config_params {
	uint32_t version;
	struct tegrabl_device_config_sdmmc_params sdmmc;
	struct tegrabl_device_config_qspi_flash_params qspi_flash;
	struct tegrabl_device_config_ufs_params ufs;
	struct tegrabl_device_config_sata_params sata;
}
);

#endif /* INCLUDED_TEGRABL_DEVICE_CONFIG_PARAMS_H */
