/*
 * Copyright (c) 2016-2020, NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#define MODULE TEGRABL_ERR_CONFIG_STORAGE

#include "build_config.h"
#include <tegrabl_blockdev.h>
#include <tegrabl_sdmmc_bdev.h>
#include <tegrabl_qspi_flash.h>
#include <tegrabl_sata.h>
#include <tegrabl_ufs_bdev.h>
#include <tegrabl_ufs.h>
#include <tegrabl_debug.h>
#include <tegrabl_profiler.h>
#include <tegrabl_soc_misc.h>
#include <config_storage.h>
#include <tegrabl_clock.h>
#include <tegrabl_qspi.h>
#include <tegrabl_compiler.h>
#include <tegrabl_storage_platform_params.h>
#include <tegrabl_storage_device_params.h>
#include <tegrabl_partition_manager.h>
#include <tegrabl_error.h>
#include <tegrabl_sdram_usage.h>
#if defined(CONFIG_ENABLE_SDCARD)
#include <tegrabl_sd_param.h>
#include <tegrabl_sd_pdata.h>
#include <tegrabl_sd_bdev.h>
#endif
#if defined(CONFIG_ENABLE_USB_MS)
#include <tegrabl_usbh.h>
#include <tegrabl_usbmsd_bdev.h>
#include <tegrabl_usbmsd.h>
#endif
#include <tegrabl_board_info.h>
#include <tegrabl_malloc.h>
#include <stdlib.h>
#include <string.h>

#if defined(CONFIG_ENABLE_UFS)
static void set_safe_ufs_params(struct tegrabl_ufs_platform_params *ufs)
{
	ufs->max_hs_mode = UFS_NO_HS_GEAR;
	ufs->max_pwm_mode = UFS_PWM_GEAR_4;
	ufs->max_active_lanes = UFS_TWO_LANES_ACTIVE;
	ufs->page_align_size = UFS_DEFAULT_PAGE_ALIGN_SIZE;
	ufs->enable_hs_modes = false;
	ufs->enable_fast_auto_mode = false;
	ufs->enable_hs_rate_b = false;
	ufs->enable_hs_rate_a = false;
	ufs->skip_hs_mode_switch = true;
	ufs->ufs_init_done = false;
}
#endif

#if defined(CONFIG_ENABLE_QSPI)
static void set_safe_qspi_flash_params(struct tegrabl_qspi_flash_platform_params *qspi_flash)
{
	qspi_flash->clk_src = TEGRABL_CLK_SRC_CLK_M;
	qspi_flash->clk_div = 0U;
	qspi_flash->clk_src_freq = 19200000U;
	qspi_flash->interface_freq = 19200000U;
	qspi_flash->max_bus_width = QSPI_BUS_WIDTH_X1;
	qspi_flash->enable_ddr_read = false;
#if defined(CONFIG_ENABLE_QSPI_DMA_BPMP)
	qspi_flash->dma_type = DMA_BPMP;
#else
	qspi_flash->dma_type = DMA_GPC;
#endif
	qspi_flash->fifo_access_mode = QSPI_MODE_DMA;
	qspi_flash->read_dummy_cycles = 0;
	qspi_flash->trimmer1_val = 0;
	qspi_flash->trimmer2_val = 0;
}
#endif

#if defined(CONFIG_ENABLE_EMMC)
static void set_safe_sdmmc_params(struct tegrabl_sdmmc_platform_params *sdmmc)
{
	sdmmc->clk_src = TEGRABL_CLK_SRC_PLLP_OUT0;
	sdmmc->best_mode = TEGRABL_SDMMC_MODE_DDR52;
	sdmmc->tap_value = 9;
	sdmmc->trim_value = 5;
	sdmmc->pd_offset = 0;
	sdmmc->pu_offset = 0;
	sdmmc->dqs_trim_hs400 = false;
	sdmmc->enable_strobe_hs400 = false;
	sdmmc->is_skip_init = true;
}
#endif

#if defined(CONFIG_ENABLE_SATA)
static void set_safe_sata_params(struct tegrabl_sata_platform_params *sata)
{
	sata->transfer_speed = 0;
	sata->is_skip_init = false;
}
#endif

struct tegrabl_device_info {
	tegrabl_storage_type_t device_type;
        uint8_t instance;
};

/* Rey/XNX device/instance SKU mapping table */
static const struct tegrabl_device_info dev_cfg_info[] = {
	{ TEGRABL_STORAGE_SDCARD,	0 },
	{ TEGRABL_STORAGE_SDMMC_USER,	3 }
};

/* Correct device_type/instance on XNX using board ID SKU */
static tegrabl_storage_type_t correct_device_and_instance(tegrabl_storage_type_t device_type,
							  uint8_t *instance)
{
	TEGRABL_UNUSED(instance);
#if defined(CONFIG_OS_IS_L4T)
	tegrabl_error_t err = TEGRABL_NO_ERROR;
	struct board_id_info *id_info;
	char cvm[5], sku[5];
	uint8_t sku_num;

	pr_debug("%s: entry DEVICE_TYPE = %d\n", __func__, device_type);
	/* 1. get board ID SKU (00 (SD) or 01 (eMMC) */
	id_info = tegrabl_malloc(sizeof(struct board_id_info));
	if (id_info == NULL) {
		pr_error("Failed to allocate memory for board-id info!\n");
		err = TEGRABL_ERROR(TEGRABL_ERR_NO_MEMORY, 0);
		/* Just return device_type unchanged, hope for the best */
		goto done;
	}

	err = tegrabl_get_board_ids(id_info);
	if (err != TEGRABL_NO_ERROR) {
		pr_error("Failed to get board id info!\n");
		/* Just return device_type unchanged, hope for the best */
		goto done;
	}

	pr_debug("board-id 0 = %s\n", (char *)id_info->part[0].part_no);

	/* SKU is in 2nd part of board-id 0.part_no, i.e. '3668-000x-' */
	strncpy(cvm, (char *)id_info->part[0].part_no, 4);
	strncpy(sku, (char *)id_info->part[0].part_no+5, 4);
	pr_debug("Board = %s, SKU = %s\n", cvm, sku);
	/* Only Rey boards at this time */
	if ((strncmp(cvm, "3668", 4) != 0))
		goto done;

	sku_num = atoi(sku);
	if (sku_num >= ARRAY_SIZE(dev_cfg_info)) {
		pr_error("Invalid SKU: %d!\n", sku_num);
		err = TEGRABL_ERROR(TEGRABL_ERR_INVALID, 0);
		goto done;
	}

	if (dev_cfg_info[sku_num].device_type != device_type) {
		/* device_type from image doesn't match SKU device_type */
		pr_debug("SKU %d: image device_type = %d, SKU device_type = %d!\n",
			 sku_num, device_type, dev_cfg_info[sku_num].device_type);
		device_type = dev_cfg_info[sku_num].device_type;
		*instance = dev_cfg_info[sku_num].instance;
	}
done:
	pr_debug("%s: final DEVICE_TYPE = %d, INSTANCE = %d\n", __func__,
		 device_type, (uint8_t)*instance);
#endif	/* CONFIG_OS_IS_L4T */

	return device_type;
}

tegrabl_error_t init_storage_device(struct tegrabl_device_config_params *device_config,
							tegrabl_storage_type_t device_type,
							uint8_t instance)
{
	tegrabl_error_t err = TEGRABL_NO_ERROR;
	const char *device = "";
	const char *source = "";
	uint8_t flag = 0U;
#if defined(CONFIG_ENABLE_SDCARD)
	struct tegrabl_sd_platform_params sd_params;
	uint32_t sd_instance = 0;
	bool is_present = 0;
#endif

	TEGRABL_UNUSED(device_config);

#if defined(CONFIG_ENABLE_EMMC)
	struct tegrabl_sdmmc_platform_params sdmmc_params;
#endif
#if defined(CONFIG_ENABLE_QSPI)
	struct tegrabl_qspi_flash_platform_params qflash_params;
#endif
#if defined(CONFIG_ENABLE_UFS)
	struct tegrabl_ufs_platform_params ufs_params;
#endif
#if defined(CONFIG_ENABLE_SATA)
	struct tegrabl_sata_platform_params sata_params;
#endif

	switch (device_type) {
#if defined(CONFIG_ENABLE_EMMC)
	case TEGRABL_STORAGE_SDMMC_BOOT:
	case TEGRABL_STORAGE_SDMMC_USER:
		device = "sdmmc";
		if (device_config->sdmmc.magic_header != MAGIC_HEADER) {
			set_safe_sdmmc_params(&sdmmc_params);
			source = "safe params";
		} else {
			sdmmc_params.clk_src = device_config->sdmmc.clk_src;
			sdmmc_params.clk_freq = device_config->sdmmc.clk_freq;
			sdmmc_params.best_mode = device_config->sdmmc.best_mode;
			sdmmc_params.tap_value = 9;
			sdmmc_params.trim_value = 5;
			sdmmc_params.pd_offset = device_config->sdmmc.pd_offset;
			sdmmc_params.pu_offset = device_config->sdmmc.pu_offset;
			sdmmc_params.dqs_trim_hs400 = device_config->sdmmc.dqs_trim_hs400;
			sdmmc_params.enable_strobe_hs400 = device_config->sdmmc.enable_strobe_hs400;
			sdmmc_params.is_skip_init = true;
			source = "boot args";
		}
		err = sdmmc_bdev_open(instance, &sdmmc_params);
		if (err != TEGRABL_NO_ERROR) {
			pr_error("Failed to open sdmmc-%d, err = %x\n", instance, err);
			goto fail;
		}
		break;
#endif

#if defined(CONFIG_ENABLE_QSPI)
	case TEGRABL_STORAGE_QSPI_FLASH:
		device = "qspi flash";
		/* covert/copy device config params to o qspi_flash platform parameters */
		if (device_config->qspi_flash.magic_header != MAGIC_HEADER) {
			set_safe_qspi_flash_params(&qflash_params);
			source = "safe params";
		} else {
			qflash_params.clk_src = device_config->qspi_flash.clk_src;
			qflash_params.clk_div = device_config->qspi_flash.clk_div;
			qflash_params.clk_src_freq = device_config->qspi_flash.clk_src_freq;
			qflash_params.interface_freq = device_config->qspi_flash.interface_freq;
			qflash_params.max_bus_width = device_config->qspi_flash.max_bus_width;
			qflash_params.enable_ddr_read = device_config->qspi_flash.enable_ddr_read;
#if defined(CONFIG_ENABLE_QSPI_DMA_BPMP)
			qflash_params.dma_type = DMA_BPMP;
#else
			qflash_params.dma_type = DMA_GPC;
#endif
			qflash_params.fifo_access_mode = device_config->qspi_flash.fifo_access_mode;
			qflash_params.read_dummy_cycles = device_config->qspi_flash.read_dummy_cycles;
			qflash_params.trimmer1_val = device_config->qspi_flash.trimmer1_val;
			qflash_params.trimmer2_val = device_config->qspi_flash.trimmer2_val;
			source = "boot args";
		}
		err = tegrabl_qspi_flash_open(instance, &qflash_params);
		if (err != TEGRABL_NO_ERROR) {
			pr_error("Failed to open QSPI flash=%d, err = %x\n", instance, err);
			goto fail;
		}
		break;
#endif
#if defined(CONFIG_ENABLE_UFS)
	case TEGRABL_STORAGE_UFS:
	case TEGRABL_STORAGE_UFS_USER:
		device = "ufs";
		if (device_config->ufs.magic_header != MAGIC_HEADER) {
			set_safe_ufs_params(&ufs_params);
			source = "safe params";
		} else {
			ufs_params.max_hs_mode = device_config->ufs.max_hs_mode;
			ufs_params.max_pwm_mode = device_config->ufs.max_pwm_mode;
			ufs_params.max_active_lanes = device_config->ufs.max_active_lanes;
			ufs_params.page_align_size = device_config->ufs.page_align_size;
			ufs_params.enable_hs_modes = device_config->ufs.enable_hs_modes;
			ufs_params.enable_fast_auto_mode = device_config->ufs.enable_fast_auto_mode;
			ufs_params.enable_hs_rate_b = device_config->ufs.enable_hs_rate_b;
			ufs_params.enable_hs_rate_a = device_config->ufs.enable_hs_rate_a;
			ufs_params.skip_hs_mode_switch = device_config->ufs.skip_hs_mode_switch;
			ufs_params.ufs_init_done = true;
			source = "boot args";
		}
		err = tegrabl_ufs_bdev_open(true, &ufs_params);
		if (err != TEGRABL_NO_ERROR) {
			pr_error("Failed to open UFS-%d, err=%x\n", instance, err);
			goto fail;
		}
		break;
#endif
#if defined(CONFIG_ENABLE_SATA)
	case TEGRABL_STORAGE_SATA:
		device = "sata";
		if (device_config->sata.magic_header != MAGIC_HEADER) {
			set_safe_sata_params(&sata_params);
			source = "safe params";
		} else {
			sata_params.transfer_speed = device_config->sata.transfer_speed;
			source = "boot args";
		}
		err = tegrabl_sata_bdev_open(instance, NULL, &sata_params);
		if (err != TEGRABL_NO_ERROR) {
			pr_error("Failed to open SATA-%d, err = %x\n", instance, err);
			goto fail;
		}
		break;
#endif

#if defined(CONFIG_ENABLE_SDCARD)
	case TEGRABL_STORAGE_SDCARD:
		err = tegrabl_sd_get_platform_params(&sd_instance, &sd_params);
		if (err != TEGRABL_NO_ERROR) {
			pr_warn("Error: failed to get sd-card params\n");
		} else {
			err = sd_bdev_is_card_present(&sd_params.cd_gpio, &is_present);
			if (err != TEGRABL_NO_ERROR) {
				pr_warn("No SD-card present !!\n");
				/* NO_ERROR to attempt booting from other sources */
				err = TEGRABL_NO_ERROR;
			} else if (is_present) {
				err = sd_bdev_open(sd_instance, &sd_params);
				if (err != TEGRABL_NO_ERROR) {
					pr_warn("Error opening sdcard-%d\n", sd_instance);
				}
			}
		}
		break;
#endif
#if defined(CONFIG_ENABLE_USB_MS)
	case TEGRABL_STORAGE_USB_MS:
		pr_debug("Calling tegrabl_usbh_init ..\n");
		err = tegrabl_usbh_init();
		if (err != TEGRABL_NO_ERROR) {
			pr_error("Error in init of XUSB host driver, err: %X\n", err);
			goto fail;
		}

		pr_debug("Calling tegrabl_usbmsd_bdev_open ..\n");
		err = tegrabl_usbmsd_bdev_open(instance);
		if (err != TEGRABL_NO_ERROR) {
			pr_error("Error opening USBMSD driver %d, err: %X\n", instance, err);
			goto fail;
		}
		break;
#endif	/* USB_MS */
	default:
		pr_error("Failed: Unknown device %d\n", (uint32_t)device_type);
		flag = 1U;
		break;
	}
	if (flag == 1U) {
		goto fail;
	}

	pr_info("%s-%d params source = %s\n", device, instance, source);

fail:
	return err;
}

/**
* @brief Placeholder for storing active boot device information
*/
static tegrabl_storage_type_t boot_device = TEGRABL_STORAGE_INVALID;

tegrabl_storage_type_t mb2_get_boot_device(void)
{
	return boot_device;
}

tegrabl_error_t config_storage(struct tegrabl_device_config_params *device_config,
							   struct tegrabl_device *devices)
{
	tegrabl_error_t err;
	tegrabl_storage_type_t device;
	uint32_t boot_dev_instance;
	uint32_t i = 0;
	static tegrabl_storage_type_t mb1_bct_to_blockdev_type[TEGRABL_BOOT_DEV_MAX] = {
		[TEGRABL_BOOT_DEV_SDMMC_BOOT] = TEGRABL_STORAGE_SDMMC_BOOT,
		[TEGRABL_BOOT_DEV_SDMMC_USER] = TEGRABL_STORAGE_SDMMC_USER,
		[TEGRABL_BOOT_DEV_SDMMC_RPMB] = TEGRABL_STORAGE_SDMMC_RPMB,
		[TEGRABL_BOOT_DEV_QSPI] = TEGRABL_STORAGE_QSPI_FLASH,
		[TEGRABL_BOOT_DEV_SATA] = TEGRABL_STORAGE_SATA,
		[TEGRABL_BOOT_DEV_UFS] = TEGRABL_STORAGE_UFS,
		[TEGRABL_BOOT_DEV_UFS_USER] = TEGRABL_STORAGE_UFS_USER,
		[TEGRABL_BOOT_DEV_SDCARD] = TEGRABL_STORAGE_SDCARD,
	};
	uint8_t instance;

	TEGRABL_UNUSED(device_config);

	err = tegrabl_blockdev_init();
	if (err != TEGRABL_NO_ERROR) {
		goto fail;
	}
	tegrabl_profiler_record("Blockdev Init", 0, DETAILED);

	/* get the boot device */
	err = tegrabl_soc_get_bootdev(&boot_device, &boot_dev_instance);
	if (err != TEGRABL_NO_ERROR) {
		pr_error("Get boot device failed\n");
		goto fail;
	}

	pr_info("Boot_device: %s instance: %u\n", tegrabl_blockdev_get_name(boot_device), boot_dev_instance);

	err = init_storage_device(device_config, boot_device, (uint8_t)boot_dev_instance);
	if (err != TEGRABL_NO_ERROR) {
		pr_error("Failed to initialize boot device\n");
		goto fail;
	}

	for (i = 0; i < TEGRABL_MAX_STORAGE_DEVICES; i++) {
		if (devices[i].type == TEGRABL_BOOT_DEV_NONE) {
			break;
		}

		if (devices[i].type >= TEGRABL_BOOT_DEV_MAX) {
			continue;
		}

		device = mb1_bct_to_blockdev_type[devices[i].type];

		/* Skip re-initializing boot device */
		if ((device == boot_device) && (devices[i].instance == boot_dev_instance)) {
			continue;
		}

		/* Correct device_type/instance  on XNX using board ID SKU */
		instance = devices[i].instance;
		device = correct_device_and_instance(device, &instance);

		err = init_storage_device(device_config, device, instance);
		if (err != TEGRABL_NO_ERROR) {
			pr_error("Failed to initialize device %d-%d\n", device, instance);
			goto fail;
		}
	}

fail:
	return err;
}

tegrabl_error_t config_storage_deinit(void)
{
	tegrabl_error_t error = TEGRABL_NO_ERROR;
	tegrabl_bdev_t *dev = NULL;

	dev = tegrabl_blockdev_next_device(dev);
	while (dev != NULL) {
		if (tegrabl_blockdev_get_storage_type(dev) == TEGRABL_STORAGE_UFS) {
			error = tegrabl_blockdev_ioctl(dev, TEGRABL_IOCTL_BLOCK_DEV_SUSPEND, "ufs_hibern8");
			if (error != TEGRABL_NO_ERROR) {
				pr_error("UFS De-Init failed\n");
			}
			break;
		}
		dev = tegrabl_blockdev_next_device(dev);
	}
	return error;
}
