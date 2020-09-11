/*
 * Copyright (c) 2015-2018, NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#define MODULE TEGRABL_ERR_ODM_DATA

#include <tegrabl_odmdata_soc.h>
#include <tegrabl_odmdata_lib.h>

struct odmdata_params odmdata_array[ODMDATA_PROP_TYPE_MAX] = {
	[ENABLE_DEBUG_CONSOLE] = {
		.mask = DEBUG_CONSOLE_MASK,
		.val = ENABLE_DEBUG_CONSOLE_VAL,
		.name = "enable-debug-console"
	},
	[DISABLE_DEBUG_CONSOLE] = {
		.mask = DEBUG_CONSOLE_MASK,
		.val = ENABLE_HIGHSPEED_UART_VAL,
		.name = "enable-high-speed-uart"
	},
	[NORMAL_BUILD] = {
		.mask = MODS_BUILD_MASK,
		.val = NORMAL_BUILD_VAL,
		.name = "normal-build"
	},
	[MODS_BUILD] = {
		.mask = MODS_BUILD_MASK,
		.val = MODS_BUILD_VAL,
		.name = "mods-build"
	},
	[DISABLE_DENVER_WDT] = {
		.mask = DENVER_WDT_MASK,
		.val = DISABLE_DENVER_WDT_VAL,
		.name = "disable-denver-wdt"
	},
	[ENABLE_DENVER_WDT] = {
		.mask = DENVER_WDT_MASK,
		.val = ENABLE_DENVER_WDT_VAL,
		.name = "enable-denver-wdt"
	},
	[DISABLE_PMIC_WDT] = {
		.mask = PMIC_WDT_MASK,
		.val = DISABLE_PMIC_WDT_VAL,
		.name = "disable-pmic-wdt"
	},
	[ENABLE_PMIC_WDT] = {
		.mask = PMIC_WDT_MASK,
		.val = ENABLE_PMIC_WDT_VAL,
		.name = "enable-pmic-wdt"
	},
	[NO_BATTERY] = {
		.mask = BATTERY_ADAPTER_MASK,
		.val = NO_BATTERY_VAL,
		.name = "no-battery"
	},
	[BATTERY_CONNECTED] = {
		.mask = BATTERY_ADAPTER_MASK,
		.val = BATTERY_CONNECTED_VAL,
		.name = "battery-connected"
	},
	[PCIE_XBAR_2_1_1_1_2] = {
		.mask = PCIE_XBAR_MASK,
		.val = PCIE_XBAR_2_1_1_1_2_VAL,
		.name = "pcie-xbar-2-1-1-1-2"
	},
	[PCIE_XBAR_4_1_0_1_2] = {
		.mask = PCIE_XBAR_MASK,
		.val = PCIE_XBAR_4_1_0_1_2_VAL,
		.name = "pcie-xbar-4-1-0-1-2"
	},
	[PCIE_XBAR_4_1_1_1_2] = {
		.mask = PCIE_XBAR_MASK,
		.val = PCIE_XBAR_4_1_1_1_2_VAL,
		.name = "pcie-xbar-4-1-1-1-2"
	},
	[PCIE_XBAR_4_0_0_1_2] = {
		.mask = PCIE_XBAR_MASK,
		.val = PCIE_XBAR_4_0_0_1_2_VAL,
		.name = "pcie-xbar-4-0-0-1-2"
	},
	[PCIE_XBAR_4_1_0_1_2_C1L6] = {
		.mask = PCIE_XBAR_MASK,
		.val = PCIE_XBAR_4_1_0_1_2_C1L6_VAL,
		.name = "pcie-xbar-4-1-0-1-2-c1l6"
	},
	[PCIE_XBAR_4_0_1_1_2] = {
		.mask = PCIE_XBAR_MASK,
		.val = PCIE_XBAR_4_0_1_1_2_VAL,
		.name = "pcie-xbar-4-0-1-1-2"
	},
	[PCIE_XBAR_4_1_1_1_2_C1L6] = {
		.mask = PCIE_XBAR_MASK,
		.val = PCIE_XBAR_4_1_1_1_2_C1L6_VAL,
		.name = "pcie-xbar-4-1-1-1-2-c1l6"
	},
	[PCIE_XBAR_2_1_1_0_4] = {
		.mask = PCIE_XBAR_MASK,
		.val = PCIE_XBAR_2_1_1_0_4_VAL,
		.name = "pcie-xbar-2-1-1-0-4"
	},
	[PCIE_XBAR_2_1_1_1_4] = {
		.mask = PCIE_XBAR_MASK,
		.val = PCIE_XBAR_2_1_1_1_4_VAL,
		.name = "pcie-xbar-2-1-1-1-4"
	},
	[PCIE_XBAR_4_1_0_0_4] = {
		.mask = PCIE_XBAR_MASK,
		.val = PCIE_XBAR_4_1_0_0_4_VAL,
		.name = "pcie-xbar-4-1-0-0-4"
	},
	[PCIE_XBAR_4_1_0_1_4] = {
		.mask = PCIE_XBAR_MASK,
		.val = PCIE_XBAR_4_1_0_1_4_VAL,
		.name = "pcie-xbar-4-1-0-1-4"
	},
	[PCIE_XBAR_4_1_1_0_4] = {
		.mask = PCIE_XBAR_MASK,
		.val = PCIE_XBAR_4_1_1_0_4_VAL,
		.name = "pcie-xbar-4-1-1-0-4"
	},
	[PCIE_XBAR_4_1_1_1_4] = {
		.mask = PCIE_XBAR_MASK,
		.val = PCIE_XBAR_4_1_1_1_4_VAL,
		.name = "pcie-xbar-4-1-1-1-4"
	},
	[PCIE_XBAR_8_1_0_0_0] = {
		.mask = PCIE_XBAR_MASK,
		.val = PCIE_XBAR_8_1_0_0_0_VAL,
		.name = "pcie-xbar-8-1-0-0-0"
	},
	[PCIE_XBAR_8_1_0_1_0] = {
		.mask = PCIE_XBAR_MASK,
		.val = PCIE_XBAR_8_1_0_1_0_VAL,
		.name = "pcie-xbar-8-1-0-1-0"
	},
	[PCIE_XBAR_8_0_0_0_2] = {
		.mask = PCIE_XBAR_MASK,
		.val = PCIE_XBAR_8_0_0_0_2_VAL,
		.name = "pcie-xbar-8-0-0-0-2"
	},
	[PCIE_XBAR_8_1_1_0_0] = {
		.mask = PCIE_XBAR_MASK,
		.val = PCIE_XBAR_8_1_1_0_0_VAL,
		.name = "pcie-xbar-8-1-1-0-0"
	},
	[PCIE_XBAR_8_1_1_1_0] = {
		.mask = PCIE_XBAR_MASK,
		.val = PCIE_XBAR_8_1_1_1_0_VAL,
		.name = "pcie-xbar-8-1-1-1-0"
	},
	[PCIE_XBAR_8_0_1_0_2] = {
		.mask = PCIE_XBAR_MASK,
		.val = PCIE_XBAR_8_0_1_0_2_VAL,
		.name = "pcie-xbar-8-0-1-0-2"
	},
	[PCIE_XBAR_8_1_0_0_1] = {
		.mask = PCIE_XBAR_MASK,
		.val = PCIE_XBAR_8_1_0_0_1_VAL,
		.name = "pcie-xbar-8-1-0-0-1"
	},
	[PCIE_XBAR_8_1_0_1_1] = {
		.mask = PCIE_XBAR_MASK,
		.val = PCIE_XBAR_8_1_0_1_1_VAL,
		.name = "pcie-xbar-8-1-0-1-1"
	},
	[PCIE_XBAR_8_1_0_0_2] = {
		.mask = PCIE_XBAR_MASK,
		.val = PCIE_XBAR_8_1_0_0_2_VAL,
		.name = "pcie-xbar-8-1-0-0-2"
	},
	[PCIE_XBAR_8_1_0_1_2] = {
		.mask = PCIE_XBAR_MASK,
		.val = PCIE_XBAR_8_1_0_1_2_VAL,
		.name = "pcie-xbar-8-1-0-1-2"
	},
	[PCIE_XBAR_8_1_1_0_1] = {
		.mask = PCIE_XBAR_MASK,
		.val = PCIE_XBAR_8_1_1_0_1_VAL,
		.name = "pcie-xbar-8-1-1-0-1"
	},
	[PCIE_XBAR_8_1_1_1_1] = {
		.mask = PCIE_XBAR_MASK,
		.val = PCIE_XBAR_8_1_1_1_1_VAL,
		.name = "pcie-xbar-8-1-1-1-1"
	},
	[DISABLE_PCIE_C0_ENDPOINT] = {
		.mask = PCIE_C0_MASK,
		.val = PCIE_C0_DISABLED_VAL,
		.name = "disable-pcie-c0-endpoint"
	},
	[ENABLE_PCIE_C0_ENDPOINT] = {
		.mask = PCIE_C0_MASK,
		.val = PCIE_C0_ENABLED_VAL,
		.name = "enable-pcie-c0-endpoint"
	},
	[DISABLE_PCIE_C4_ENDPOINT] = {
		.mask = PCIE_C4_MASK,
		.val = PCIE_C4_DISABLED_VAL,
		.name = "disable-pcie-c4-endpoint"
	},
	[ENABLE_PCIE_C4_ENDPOINT] = {
		.mask = PCIE_C4_MASK,
		.val = PCIE_C4_ENABLED_VAL,
		.name = "enable-pcie-c4-endpoint"
	},
	[DISABLE_UFS_UPHY] = {
		.mask = UFS_HSIO_UPHY_MASK,
		.val = UFS_HSIO_UPHY_DISABLED_VAL,
		.name = "disable-ufs-uphy"
	},
	[ENABLE_UFS_UPHY_L11] = {
		.mask = UFS_HSIO_UPHY_MASK,
		.val = UFS_HSIO_UPHY_L11_VAL,
		.name = "enable-ufs-uphy-l11"
	},
	[ENABLE_UFS_UPHY_L10] = {
		.mask = UFS_HSIO_UPHY_MASK,
		.val = UFS_HSIO_UPHY_L10_VAL,
		.name = "enable-ufs-uphy-l10"
	},
	[ENABLE_UFS_UPHY_L10_L11] = {
		.mask = UFS_HSIO_UPHY_MASK,
		.val = UFS_HSIO_UPHY_L10_L11_VAL,
		.name = "enable-ufs-uphy-l10-l11"
	},
	[DISABLE_SATA] = {
		.mask = SATA_MASK,
		.val = SATA_DISABLED_VAL,
		.name = "disable-sata"
	},
	[ENABLE_SATA] = {
		.mask = SATA_MASK,
		.val = SATA_ENABLED_VAL,
		.name = "enable-sata"
	},
	[DISABLE_NVHS_UPHY] = {
		.mask = NVHS_UPHY_MASK,
		.val = NVHS_UPHY_DISABLED_VAL,
		.name = "disable-nvhs-uphy"
	},
	[ENABLE_NVHS_UPHY_PCIE_C5] = {
		.mask = NVHS_UPHY_MASK,
		.val = NVHS_UPHY_PCIE_C5_VAL,
		.name = "enable-nvhs-uphy-pcie-c5"
	},
	[ENABLE_NVHS_UPHY_SLVS] = {
		.mask = NVHS_UPHY_MASK,
		.val = NVHS_UPHY_SLVS_VAL,
		.name = "enable-nvhs-uphy-slvs"
	},
	[ENABLE_NVHS_UPHY_NVLINK] = {
		.mask = NVHS_UPHY_MASK,
		.val = NVHS_UPHY_NVLINK_VAL,
		.name = "enable-nvhs-uphy-nvlink"
	},
	[BOOTLOADER_LOCK] = {
		.mask = BOOTLOADER_LOCK_MASK,
		.val = BOOTLOADER_LOCK_VAL,
		.name = "bootloader_locked",
	},
	[BOOTLOADER_UNLOCK] = {
		.mask = BOOTLOADER_LOCK_MASK,
		.val = BOOTLOADER_UNLOCK_VAL,
		.name = "bootloader_unlocked",
	},
	[DISABLE_PCIE_C5_ENDPOINT] = {
		.mask = PCIE_C5_MASK,
		.val = PCIE_C5_DISABLED_VAL,
		.name = "disable-pcie-c5-endpoint"
	},
	[ENABLE_PCIE_C5_ENDPOINT] = {
		.mask = PCIE_C5_MASK,
		.val = PCIE_C5_ENABLED_VAL,
		.name = "enable-pcie-c5-endpoint"
	},
};

