/*
 * Copyright (c) 2015-2018, NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#ifndef INCLUDED_TEGRABL_CPUBL_PARAMS_H
#define INCLUDED_TEGRABL_CPUBL_PARAMS_H

#if defined(__cplusplus)
extern "C"
{
#endif

#include <tegrabl_compiler.h>
#include <tegrabl_boot_type.h>
#include <tegrabl_storage_device_params.h>
#include <tegrabl_rollback_protection.h>

#define TBOOT_CPUBL_PARAMS_VERSION 0
#define TEGRABL_MAX_VERSION_STRING 128 /* chars including null */
#define CPUBL_PARAMS_RESERVED_SIZE 2048U

#define TEGRABL_MAX_I2C_BUSES_INDEX 9U

/**
 * Consolidated structure to pass information sharing from MB2(Tboot-BPMP) to
 * CPU bootloader
 */
#define NUM_DRAM_BAD_PAGES 1024U

TEGRABL_PACKED(
struct tboot_cpubl_params {

	union {
		uint8_t byte_array[CPUBL_PARAMS_RESERVED_SIZE];

		struct {
			/**< version */
			TEGRABL_DECLARE_ALIGNED(uint32_t version, 4);

			/**< Uart instance */
			TEGRABL_DECLARE_ALIGNED(uint32_t uart_instance, 4);

			/**< Enable logs/ verbose */
			TEGRABL_DECLARE_ALIGNED(
				union {
					uint32_t verbose;
					uint32_t enable_log;
				}, 4);

			/**< device config params from mb1 bct */
			TEGRABL_DECLARE_ALIGNED(
			struct tegrabl_device_config_params device_config, 8);

			/**< Address of i2c bus frequecy from mb1 bct */
			TEGRABL_DECLARE_ALIGNED(
					uint64_t i2c_bus_frequency_address, 8);

			/**< Address of controller pad settings */
			TEGRABL_DECLARE_ALIGNED(uint64_t controller_prod_settings, 8);

			/**< Total size of controller pad settings */
			TEGRABL_DECLARE_ALIGNED(uint64_t controller_prod_settings_size, 8);

			/**< Parameters for Secure_OS/TLK passed via GPR */
			TEGRABL_DECLARE_ALIGNED(uint64_t secure_os_params[4], 8);
			TEGRABL_DECLARE_ALIGNED(uint64_t secure_os_start, 8);

			/**< If tos loaded by mb2 has secureos or not. */
			TEGRABL_DECLARE_ALIGNED(uint32_t secureos_type, 4);

			/**< SDRAM size in bytes */
			TEGRABL_DECLARE_ALIGNED(uint64_t sdram_size, 8);

			/**< bootloader dtb load address */
			TEGRABL_DECLARE_ALIGNED(uint64_t bl_dtb_load_address, 8);

			/**< physical address and size of the carveouts */
			TEGRABL_DECLARE_ALIGNED(
				struct tegrabl_carveout_info carveout_info[CARVEOUT_NUM], 8);

			/**< Indicate whether DRAM ECC page blacklisting feature is enabled
				 or not
			 */
			TEGRABL_DECLARE_ALIGNED(
			union {
				uint64_t feature_flag_raw;
				struct {
					uint64_t enable_dram_page_blacklisting:1;
					uint64_t enable_combined_uart:1;
					uint64_t enable_dram_staged_scrubbing:1;
					uint64_t enable_sce:1;
					uint64_t switch_bootchain:1;
					uint64_t reset_to_recovery:1;
					uint64_t enable_rce:1;
				};
			}, 8);

			/**< Start address of SDRAM params used in MB1 as per RAMCODE */
			TEGRABL_DECLARE_ALIGNED(uint64_t sdram_params_offset, 8);

			/**< Start address of DRAM ECC page‐blacklisting information
				 structure
			 */
			TEGRABL_DECLARE_ALIGNED(
				uint64_t dram_page_blacklist_info_address, 8);

			/**< Start address of Golden register data region */
			TEGRABL_DECLARE_ALIGNED(uint64_t golden_register_address, 8);

			/**< Size of Golden register data region */
			TEGRABL_DECLARE_ALIGNED(uint32_t golden_register_size, 8);

			/**< Start address of Profiling data */
			TEGRABL_DECLARE_ALIGNED(uint64_t profiling_data_address, 8);

			/**< Size of Profiling data */
			TEGRABL_DECLARE_ALIGNED(uint32_t profiling_data_size, 8);

			/**< Start offset of unallocated/unused data in CPU‐BL carveout */
			TEGRABL_DECLARE_ALIGNED(uint64_t cpubl_carveout_safe_end_offset, 8);

			/**< Start offset of unallocated/unused data in MISC carveout */
			TEGRABL_DECLARE_ALIGNED(
								uint64_t misc_carveout_safe_start_offset, 8);

			/**< Boot type set by nv3pserver based on boot command from host. */
			TEGRABL_DECLARE_ALIGNED(uint32_t recovery_boot_type, 8);

			/**< Boot mode can be cold boot, uart, recovery or RCM */
			TEGRABL_DECLARE_ALIGNED(uint32_t boot_type, 8);

			/**< Uart_base Address for debug prints */
			TEGRABL_DECLARE_ALIGNED(uint64_t early_uart_addr, 8);

			/**< mb1 bct version information */
			TEGRABL_DECLARE_ALIGNED(uint32_t mb1_bct_version, 8);

			/**< mb1 version */
			TEGRABL_DECLARE_ALIGNED(
					uint8_t mb1_version[TEGRABL_MAX_VERSION_STRING], 8);

			/**< mb2 version */
			TEGRABL_DECLARE_ALIGNED(
					uint8_t mb2_version[TEGRABL_MAX_VERSION_STRING], 8);

			/**< CPUBL version needed by Hypervisor */
			TEGRABL_DECLARE_ALIGNED(
					uint8_t cpubl_version[TEGRABL_MAX_VERSION_STRING],
													8);

			/**< Reset reason as read from PMIC */
			TEGRABL_DECLARE_ALIGNED(uint32_t pmic_rst_reason, 8);

			/**< Pointer to BRBCT location in sdram */
			TEGRABL_DECLARE_ALIGNED(uint64_t brbct_carveout, 8);

			/**< Storage devices to be used */
			TEGRABL_DECLARE_ALIGNED(struct tegrabl_device storage_devices[TEGRABL_MAX_STORAGE_DEVICES], 8);

			/**< Minimum ratchet version of oem-fw bins */
			TEGRABL_DECLARE_ALIGNED(uint8_t min_ratchet[MAX_OEM_FW_RATCHET_INDEX], 8);
		};
	};
}
);

/* Check if cpubl params structure size matches with pre-defined size */
TEGRABL_COMPILE_ASSERT(sizeof(struct tboot_cpubl_params) == CPUBL_PARAMS_RESERVED_SIZE,
		"cpubl param size mismatch");

#if defined(__cplusplus)
}
#endif

#endif /*  INCLUDED_TEGRABL_CPUBL_PARAMS_H */

