/*
 * Copyright (c) 2017-2019, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited
 */

#define MODULE TEGRABL_ERR_NO_MODULE

#include <debug.h>
#include <inttypes.h>
#include <tegrabl_ar_macro.h>
#include <arch/arm64.h>
#include <arch/mmu.h>
#include <arch/ops.h>
#include <string.h>
#include <console.h>
#include <err.h>
#include <tegrabl_debug.h>
#include <tegrabl_console.h>
#include <tegrabl_addressmap.h>
#include <tegrabl_cpubl_params.h>
#include <tegrabl_brbct.h>
#include <arscratch.h>
#include <tegrabl_page_allocator.h>
#include <platform_c.h>
#include <tegrabl_sdram_usage.h>
#include <tegrabl_blockdev.h>
#include <tegrabl_ufs_bdev.h>
#include <tegrabl_devicetree.h>
#include <tegrabl_soc_misc.h>
#include <tegrabl_partition_manager.h>
#include <tegrabl_malloc.h>
#include <tegrabl_sdmmc_bdev.h>
#include <tegrabl_qspi_flash_param.h>
#include <tegrabl_qspi.h>
#include <tegrabl_qspi_flash.h>
#include <tegrabl_clock.h>
#include <tegrabl_mce.h>
#include <tegrabl_partition_loader.h>
#include <tegrabl_comb_uart.h>
#include <tegrabl_sata.h>
#include <tegrabl_nct.h>
#include <tegrabl_bpmp_fw_interface.h>
#include <tegrabl_ipc_soc.h>
#include <tegrabl_wdt.h>
#include <tegrabl_exit.h>
#include <tegrabl_t19x_ccplex_cache.h>
#include <tegrabl_pmic.h>
#include <tegrabl_regulator.h>
#if defined(CONFIG_ENABLE_PMIC_MAX20024)
#include <tegrabl_pmic_max77620.h>
#endif
#include <tegrabl_i2c.h>
#include <tegrabl_power_i2c.h>
#include <tegrabl_gpio.h>
#include <tegrabl_tca9539_gpio.h>
#include <tegrabl_display.h>

#ifdef CONFIG_ENABLE_PLATFORM_MUX
#include <platform_config.h>
#endif
#include <tegrabl_odmdata_lib.h>
#include <tegrabl_sd_pdata.h>
#include <tegrabl_sd_bdev.h>
#include <tegrabl_device_prod.h>

#include <tegrabl_plugin_manager.h>
#include <config_storage.h>

#if defined(CONFIG_ENABLE_STAGED_SCRUBBING)
#include <tegrabl_linuxboot_helper.h>
#endif

#include <tegrabl_cbo.h>
#include <tegrabl_odmdata_soc.h>
#include <arfuse.h>
#include <tegrabl_drf.h>
#include <stdint.h>
#include <stdbool.h>
#include <arpmc_misc.h>
#include <tegrabl_reset_prepare.h>
#include <tegrabl_io.h>

static bool is_comb_uart_initialized = false;

#define SCRATCH_WDT_GLOBAL_DISABLE SCRATCH_SECURE_RSV77_SCRATCH_0
#define SCRATCH_READ(reg) NV_READ32((uint32_t)NV_ADDRESS_MAP_SCRATCH_BASE + ((uint32_t)(reg)))
#define DISABLE_WDT (0xd15ab1e0U)

#define LOCAL_TRACE 0
extern int __version_start;

extern int _end;
struct tboot_cpubl_params *boot_params;

#define CPUBL_PARAMS_SIZE	(64 * 1024LLU)

struct mmio_mapping_info {
	    uint64_t addr;
	    uint64_t size;
};

struct mmio_mapping_info mmio_mappings[] = {
	{ NV_ADDRESS_MAP_UARTA_BASE, NV_ADDRESS_MAP_UARTA_SIZE },
	{ NV_ADDRESS_MAP_UARTB_BASE, NV_ADDRESS_MAP_UARTB_SIZE },
	{ NV_ADDRESS_MAP_UARTC_BASE, NV_ADDRESS_MAP_UARTC_SIZE },
	{ NV_ADDRESS_MAP_UARTD_BASE, NV_ADDRESS_MAP_UARTD_SIZE },
	{ NV_ADDRESS_MAP_UARTE_BASE, NV_ADDRESS_MAP_UARTE_SIZE },
	{ NV_ADDRESS_MAP_UARTF_BASE, NV_ADDRESS_MAP_UARTF_SIZE },
	{ NV_ADDRESS_MAP_UARTG_BASE, NV_ADDRESS_MAP_UARTG_SIZE },
	{ NV_ADDRESS_MAP_SDMMC1_BASE, NV_ADDRESS_MAP_SDMMC1_SIZE },
	{ NV_ADDRESS_MAP_SDMMC3_BASE, NV_ADDRESS_MAP_SDMMC3_SIZE },
	{ NV_ADDRESS_MAP_SDMMC4_BASE, NV_ADDRESS_MAP_SDMMC4_SIZE },
	{ NV_ADDRESS_MAP_SATA_BASE, NV_ADDRESS_MAP_SATA_SIZE},
	{ NV_ADDRESS_MAP_UFSHC_0_BASE, NV_ADDRESS_MAP_UFSHC_0_SIZE},
	{ NV_ADDRESS_MAP_UFSHC_0_UNIPRO_BASE, NV_ADDRESS_MAP_UFSHC_0_UNIPRO_SIZE},
	{ NV_ADDRESS_MAP_UFS_CAR_BASE, NV_ADDRESS_MAP_UFS_CAR_SIZE},
	{ NV_ADDRESS_MAP_MPHY_L0_BASE, NV_ADDRESS_MAP_MPHY_L0_SIZE},
	{ NV_ADDRESS_MAP_MPHY_L1_BASE, NV_ADDRESS_MAP_MPHY_L1_SIZE},
	{ NV_ADDRESS_MAP_CAR_BASE, NV_ADDRESS_MAP_CAR_SIZE },
	{ NV_ADDRESS_MAP_I2C1_BASE, NV_ADDRESS_MAP_I2C1_SIZE },
	{ NV_ADDRESS_MAP_I2C2_BASE, NV_ADDRESS_MAP_I2C2_SIZE },
	{ NV_ADDRESS_MAP_I2C3_BASE, NV_ADDRESS_MAP_I2C3_SIZE },
	{ NV_ADDRESS_MAP_I2C4_BASE, NV_ADDRESS_MAP_I2C4_SIZE },
	{ NV_ADDRESS_MAP_I2C5_BASE, NV_ADDRESS_MAP_I2C5_SIZE },
	{ NV_ADDRESS_MAP_I2C6_BASE, NV_ADDRESS_MAP_I2C6_SIZE },
	{ NV_ADDRESS_MAP_I2C7_BASE, NV_ADDRESS_MAP_I2C7_SIZE },
	{ NV_ADDRESS_MAP_I2C8_BASE, NV_ADDRESS_MAP_I2C8_SIZE },
	{ NV_ADDRESS_MAP_I2C9_BASE, NV_ADDRESS_MAP_I2C9_SIZE },
	{ NV_ADDRESS_MAP_I2C10_BASE, NV_ADDRESS_MAP_I2C10_SIZE },
	{ NV_ADDRESS_MAP_CCPLEX_GIC_BASE, NV_ADDRESS_MAP_CCPLEX_GIC_MAIN_SIZE },
	{ NV_ADDRESS_MAP_TOP0_HSP_DB_0_BASE, NV_ADDRESS_MAP_TOP0_HSP_DB_0_SIZE },
	{ NV_ADDRESS_MAP_PADCTL_A5_BASE, NV_ADDRESS_MAP_PADCTL_A5_SIZE },
	{ NV_ADDRESS_MAP_XUSB_DEV_BASE, NV_ADDRESS_MAP_XUSB_DEV_SIZE },
	{ NV_ADDRESS_MAP_XUSB_PADCTL_BASE, NV_ADDRESS_MAP_XUSB_PADCTL_SIZE },
	{ NV_ADDRESS_MAP_XUSB_HOST_BASE, NV_ADDRESS_MAP_XUSB_HOST_SIZE},
	{ NV_ADDRESS_MAP_XUSB_HOST_PF_CFG_BASE, NV_ADDRESS_MAP_XUSB_HOST_PF_CFG_SIZE},
	{ NV_ADDRESS_MAP_XUSB_HOST_PF_BAR0_BASE, NV_ADDRESS_MAP_XUSB_HOST_PF_BAR0_SIZE},
	{ NV_ADDRESS_MAP_PMC_BASE, NV_ADDRESS_MAP_PMC_SIZE },
	{ NV_ADDRESS_MAP_PMC_MISC_BASE, NV_ADDRESS_MAP_PMC_MISC_SIZE },
	{ NV_ADDRESS_MAP_MISC_BASE, NV_ADDRESS_MAP_MISC_SIZE },
	{ NV_ADDRESS_MAP_MCB_BASE, NV_ADDRESS_MAP_MCB_SIZE },
	{ NV_ADDRESS_MAP_EMCB_BASE, NV_ADDRESS_MAP_EMCB_SIZE },
	{ NV_ADDRESS_MAP_TSCUS_BASE, NV_ADDRESS_MAP_TSCUS_SIZE },
	{ NV_ADDRESS_MAP_FUSE_BASE, NV_ADDRESS_MAP_FUSE_SIZE },
	{ NV_ADDRESS_MAP_SCRATCH_BASE, NV_ADDRESS_MAP_SCRATCH_SIZE },
	{ NV_ADDRESS_MAP_TOP_TKE_BASE, NV_ADDRESS_MAP_TOP_TKE_SIZE },
	{ NV_ADDRESS_MAP_SCRATCH_BASE, NV_ADDRESS_MAP_SCRATCH_SIZE },
	{ NV_ADDRESS_MAP_AON_HSP_BASE, NV_ADDRESS_MAP_AON_HSP_SIZE },
	{ NV_ADDRESS_MAP_TOP0_HSP_BASE, NV_ADDRESS_MAP_TOP0_HSP_SIZE },
	{ NV_ADDRESS_MAP_GPCDMA_BASE, NV_ADDRESS_MAP_GPCDMA_SIZE },
	{ NV_ADDRESS_MAP_SYSRAM_0_BASE, NV_ADDRESS_MAP_SYSRAM_0_IMPL_SIZE },
	{ NV_ADDRESS_MAP_PADCTL_A17_BASE, NV_ADDRESS_MAP_PADCTL_A17_SIZE },
	{ IPC_COMM_ADDR_START_MMU_ALIGNED, IPC_COMM_ADDR_SIZE_MMU_ALIGNED },
	{ NV_ADDRESS_MAP_DPAUX_BASE, NV_ADDRESS_MAP_DPAUX_SIZE },
	{ NV_ADDRESS_MAP_DPAUX1_BASE, NV_ADDRESS_MAP_DPAUX1_SIZE },
	{ NV_ADDRESS_MAP_DPAUX2_BASE, NV_ADDRESS_MAP_DPAUX2_SIZE },
	{ NV_ADDRESS_MAP_DPAUX3_BASE, NV_ADDRESS_MAP_DPAUX3_SIZE },
	{ NV_ADDRESS_MAP_AON_GPIO_0_BASE, NV_ADDRESS_MAP_AON_GPIO_0_SIZE },
	{ NV_ADDRESS_MAP_GPIO_CTL_BASE, NV_ADDRESS_MAP_GPIO_CTL_SIZE },
	{ NV_ADDRESS_MAP_PADCTL_A13_BASE, NV_ADDRESS_MAP_PADCTL_A13_SIZE},
	{ NV_ADDRESS_MAP_PADCTL_A16_BASE, NV_ADDRESS_MAP_PADCTL_A16_SIZE },
	{ NV_ADDRESS_MAP_SE0_BASE, NV_ADDRESS_MAP_SE0_SIZE },
	{ NV_ADDRESS_MAP_SE1_BASE, NV_ADDRESS_MAP_SE1_SIZE },
	{ NV_ADDRESS_MAP_SE2_BASE, NV_ADDRESS_MAP_SE2_SIZE },
	{ NV_ADDRESS_MAP_SE3_BASE, NV_ADDRESS_MAP_SE3_SIZE },
	{ NV_ADDRESS_MAP_SE4_BASE, NV_ADDRESS_MAP_SE4_SIZE },
	{ NV_ADDRESS_MAP_QSPI0_BASE, NV_ADDRESS_MAP_QSPI0_SIZE },
	{ NV_ADDRESS_MAP_QSPI1_BASE, NV_ADDRESS_MAP_QSPI1_SIZE },
	{ NV_ADDRESS_MAP_SOR_BASE, NV_ADDRESS_MAP_SOR_SIZE },
	{ NV_ADDRESS_MAP_SOR1_BASE, NV_ADDRESS_MAP_SOR1_SIZE },
	{ NV_ADDRESS_MAP_SOR2_BASE, NV_ADDRESS_MAP_SOR2_SIZE },
	{ NV_ADDRESS_MAP_SOR3_BASE, NV_ADDRESS_MAP_SOR3_SIZE },
	{ NV_ADDRESS_MAP_DISPLAY_BASE, NV_ADDRESS_MAP_DISPLAY_SIZE },
	{ NV_ADDRESS_MAP_DISPLAYB_BASE, NV_ADDRESS_MAP_DISPLAYB_SIZE },
	{ NV_ADDRESS_MAP_DISPLAYC_BASE, NV_ADDRESS_MAP_DISPLAYC_SIZE },
#if defined(CONFIG_ENABLE_ETHERNET_BOOT)
	{ NV_ADDRESS_MAP_ETHER_QOS_0_BASE, NV_ADDRESS_MAP_ETHER_QOS_0_SIZE },
	{ NV_ADDRESS_MAP_PADCTL_A21_BASE, NV_ADDRESS_MAP_PADCTL_A21_SIZE },
	{ NV_ADDRESS_MAP_PADCTL_A4_BASE, NV_ADDRESS_MAP_PADCTL_A4_SIZE},
	{ NV_ADDRESS_MAP_LIC_CH0_BASE, NV_ADDRESS_MAP_LIC_CH0_SIZE},
#endif
	{ NV_ADDRESS_MAP_PWM1_BASE, NV_ADDRESS_MAP_PWM1_SIZE },
	{ NV_ADDRESS_MAP_PWM2_BASE, NV_ADDRESS_MAP_PWM2_SIZE },
	{ NV_ADDRESS_MAP_PWM3_BASE, NV_ADDRESS_MAP_PWM3_SIZE },
	{ NV_ADDRESS_MAP_PWM4_BASE, NV_ADDRESS_MAP_PWM4_SIZE },
	{ NV_ADDRESS_MAP_PWM5_BASE, NV_ADDRESS_MAP_PWM5_SIZE },
	{ NV_ADDRESS_MAP_PWM6_BASE, NV_ADDRESS_MAP_PWM6_SIZE },
	{ NV_ADDRESS_MAP_PWM7_BASE, NV_ADDRESS_MAP_PWM7_SIZE },
	{ NV_ADDRESS_MAP_PWM8_BASE, NV_ADDRESS_MAP_PWM8_SIZE },
	{ NV_ADDRESS_MAP_MSS_QUAL_BASE, NV_ADDRESS_MAP_MSS_QUAL_SIZE },
	{ NV_ADDRESS_MAP_MCB_BASE, NV_ADDRESS_MAP_MCB_SIZE },
};

static inline void platform_init_boot_param(void)
{
	uint32_t temp = 0;

	temp = NV_READ32(NV_ADDRESS_MAP_SCRATCH_BASE + SCRATCH_SCRATCH_7);

	if (boot_params)
		return;

	boot_params = (struct tboot_cpubl_params *)(uintptr_t)
				  (temp < SDRAM_START_ADDRESS ?
				  (uint64_t) temp << PAGE_SIZE_LOG2 : temp);
}

static bool cpubl_wdt_global_disable(void)
{
	uint32_t scratch77 = 0;
	uint32_t reg = 0;
	uint32_t jtag_enable = 0;
	uint32_t arm_jtag_dis = 0;
	uint32_t nv_production_mode = 0;

	scratch77 = SCRATCH_READ(SCRATCH_WDT_GLOBAL_DISABLE);
	reg = REG_READ(PMC_MISC, PMC_MISC_DEBUG_AUTHENTICATION);
	jtag_enable = NV_DRF_VAL(PMC_MISC, DEBUG_AUTHENTICATION, JTAG_ENABLE, reg);
	reg = REG_READ(FUSE, FUSE_ARM_JTAG_DIS);
	arm_jtag_dis = NV_DRF_VAL(FUSE, ARM_JTAG_DIS, ARM_JTAG_DIS, reg);
	reg = REG_READ(FUSE, FUSE_PRODUCTION_MODE);
	nv_production_mode = NV_DRF_VAL(FUSE, PRODUCTION_MODE, PRODUCTION_MODE, reg);

	return (scratch77 == DISABLE_WDT) &&
		((nv_production_mode == 0U) ||	((jtag_enable == 1U) && (arm_jtag_dis == 0U)));

}

static tegrabl_error_t platform_disable_global_wdt(bool is_storage_flush)
{
	tegrabl_error_t err = TEGRABL_NO_ERROR;
	uint32_t odm_data = tegrabl_odmdata_get();
	uint32_t val = 0xffffffff;
	bool disable_wdt_flag;

	disable_wdt_flag = cpubl_wdt_global_disable();

	if (true == disable_wdt_flag) {
		val = ~((1 << DENVER_WDT_BIT_OFFSET) | (1 << PMIC_WDT_BIT_OFFSET));
		tegrabl_odmdata_set(odm_data & val, is_storage_flush);
		err = TEGRABL_NO_ERROR;
	}

	return err;
}

tegrabl_error_t cpubl_params_version_check(struct tboot_cpubl_params *params)
{
	if (!params)
		return TEGRABL_ERROR(TEGRABL_ERR_INVALID, 0);

	switch (params->version) {
	default:
		break;
	}

	return TEGRABL_NO_ERROR;
}

void wdt_enable(void)
{
#if defined(CONFIG_ENABLE_WDT)
	/* Program cpu_wdt with 5th_expiry if WDT enable conditions are met*/
	if (tegrabl_odmdata_get_config_by_name("enable-denver-wdt")) {
		tegrabl_wdt_enable(TEGRABL_WDT_LCCPLEX, TEGRABL_WDT_EXPIRY_5, CONFIG_WDT_PERIOD_IN_EXECUTION,
						   TEGRABL_WDT_SRC_TSCCNT_29_0);
	}
#endif
}

status_t platform_early_init(void)
{
	status_t error = TEGRABL_NO_ERROR;
	carve_out_type_t carveout;
#if defined(CONFIG_ENABLE_COMB_UART)
	uint32_t mailbox;
#endif

	platform_init_boot_param();
	if (boot_params == NULL) {
		pr_critical("boot_param is null\n");
		error = TEGRABL_ERR_INVALID;
		goto fail;
	}

	error = cpubl_params_version_check(boot_params);
	if (error != TEGRABL_NO_ERROR) {
		pr_critical("Failed to check cpubl param version");
		goto fail;
	}

	/* IPC initialization to use BPMP for clk config*/
	error = tegrabl_ipc_init();
	if (error != TEGRABL_NO_ERROR) {
		goto fail;
	}

	error = tegrabl_brbct_init(boot_params->brbct_carveout);
	if (error != TEGRABL_NO_ERROR) {
		pr_critical("Failed to initialize brbct\n");
		goto fail;
	}

	wdt_enable();

#if defined(CONFIG_ENABLE_UART)
	if (boot_params->enable_combined_uart == 0U) {
		error = tegrabl_console_register(TEGRABL_CONSOLE_UART, boot_params->uart_instance, NULL);
		if ((error != TEGRABL_NO_ERROR) && (TEGRABL_ERROR_REASON(error) != TEGRABL_ERR_NOT_SUPPORTED)) {
			goto fail;
		}
	}
#endif

#if defined(CONFIG_ENABLE_COMB_UART)
	mailbox = COMB_UART_CPU_MAILBOX;

	if (boot_params->enable_combined_uart == 1U) {
		error = tegrabl_console_register(TEGRABL_CONSOLE_COMB_UART, 0, &mailbox);
		if (error != TEGRABL_NO_ERROR) {
			if (TEGRABL_ERROR_REASON(error) != TEGRABL_ERR_NOT_SUPPORTED) {
				goto fail;
			}
		} else {
			is_comb_uart_initialized = true;
		}
	}
#endif

	if (boot_params->enable_log != 0U) {
		error = tegrabl_debug_init();
		if (error != TEGRABL_NO_ERROR) {
			goto fail;
		}
	} else {
		tegrabl_debug_deinit();
	}


	pr_info("Welcome to Cboot\n");
	pr_info("Cboot Version: %s\n", (char *) &__version_start);
	pr_info("CPU-BL Params @ %p\n", boot_params);

	for (carveout = 0; carveout < CARVEOUT_NUM; carveout++) {
		pr_info("%2u) Base:0x%08"PRIx64" Size:0x%08"PRIx64"\n", carveout,
				boot_params->carveout_info[carveout].base,
				boot_params->carveout_info[carveout].size);
	}

#ifdef CONFIG_ENABLE_PLATFORM_MUX
	/* Apply platform config */
	pr_info("Applying platform configs\n");
	platform_config();
#endif

	/* Enable SError */
	arm64_enable_serror();

	/* initialize the interrupt controller */
	platform_init_interrupts();

	/* initialize the timer block */
	platform_init_timer();

fail:
	return error;
}

int platform_use_identity_mmu_mappings(void)
{
	/* Use only the mappings specified in this file. */
	return 0;
}

void platform_init_mmu_mappings(void)
{
	uint32_t idx;
	uintptr_t addr;

	for (idx = 0; idx < ARRAY_SIZE(mmio_mappings); idx++) {
		arm64_mmu_map(mmio_mappings[idx].addr, mmio_mappings[idx].addr,
				mmio_mappings[idx].size, MMU_FLAG_EXECUTE_NOT |
				MMU_FLAG_READWRITE | MMU_FLAG_DEVICE);
	}

	platform_init_boot_param();
	/*
	 * Map the complete CARVEOUT_CPUBL_PARAMS to access CPUBL Params, BR-BCT
	 * profiler, GR
	*/
	arm64_mmu_map((uintptr_t)boot_params, (uintptr_t)boot_params,
					CPUBL_PARAMS_SIZE,
					MMU_FLAG_CACHED | MMU_FLAG_READWRITE | MMU_FLAG_EXECUTE_NOT);

	arm64_mmu_map(
			(uintptr_t)boot_params->carveout_info[CARVEOUT_MISC].base,
			(uintptr_t)boot_params->carveout_info[CARVEOUT_MISC].base,
			boot_params->carveout_info[CARVEOUT_MISC].size,
			MMU_FLAG_CACHED | MMU_FLAG_READWRITE | MMU_FLAG_EXECUTE_NOT);

	arm64_mmu_map(
			(uintptr_t)boot_params->carveout_info[CARVEOUT_CPUBL].base,
			(uintptr_t)boot_params->carveout_info[CARVEOUT_CPUBL].base,
			boot_params->carveout_info[CARVEOUT_CPUBL].size,
			MMU_FLAG_CACHED | MMU_FLAG_READWRITE);

	/* Align BL-DTB */
	addr = ((uintptr_t)boot_params->bl_dtb_load_address);
	addr = addr & ~((uint64_t)(PAGE_SIZE - 1));
	arm64_mmu_map(
			addr,
			addr,
			(1024 * 1024),
			MMU_FLAG_CACHED | MMU_FLAG_READWRITE | MMU_FLAG_EXECUTE_NOT);

	arm64_mmu_map((uintptr_t)boot_params->carveout_info[CARVEOUT_OS].base,
				  (uintptr_t)boot_params->carveout_info[CARVEOUT_OS].base,
				  boot_params->carveout_info[CARVEOUT_OS].size,
				  MMU_FLAG_CACHED | MMU_FLAG_READWRITE | MMU_FLAG_EXECUTE_NOT);
}

void platform_uninit(void)
{
#if defined(CONFIG_ENABLE_WDT)
	/* disable cpu-wdt before kernel handoff */
	tegrabl_wdt_disable(TEGRABL_WDT_LCCPLEX);
#endif /* CONFIG_ENABLE_WDT */

	platform_uninit_timer();

	arch_disable_ints();

#if WITH_MMU
	tegrabl_t19x_ccplex_flush_cache_all();
	arch_disable_mmu();
#endif

	arm64_disable_serror();
}

static tegrabl_error_t platform_init_power(void)
{
	tegrabl_error_t err = TEGRABL_NO_ERROR;

	tegrabl_pmic_init();

	err = tegrabl_regulator_init();
	if (TEGRABL_NO_ERROR != err)
		goto fail;

#if defined(CONFIG_ENABLE_PMIC_MAX20024)
	/* Max20024 pmic is identical to max77620 in terms of register and i2c bus/address compatibilty.
	   Hence using max77620 driver for max20024 operations.
	 */
	err = tegrabl_max77620_init(TEGRABL_INSTANCE_POWER_I2C_BPMPFW);
#endif

fail:
	if (TEGRABL_NO_ERROR != err)
		TEGRABL_SET_HIGHEST_MODULE(err);
	return err;
}

static tegrabl_error_t platform_init_bldtb_override(void *bl_dtb)
{
	tegrabl_error_t err = TEGRABL_NO_ERROR;

#if defined(CONFIG_ENABLE_BL_DTB_OVERRIDE)

#define FDT_SIZE_BL_DT_NODES 8192
#define MAX_BL_DTB_SIZE (1024 * 1024)

	/* create new space for override */
	err = tegrabl_dt_create_space(bl_dtb, FDT_SIZE_BL_DT_NODES, MAX_BL_DTB_SIZE);
	if (err != TEGRABL_NO_ERROR) {
		pr_error("Failed to create space for BL-DTB\n");
		return err;
	}

	/* save board info to BL-DTB */
	err = tegrabl_add_plugin_manager_ids_by_path(bl_dtb, "/chosen");
	if (err != TEGRABL_NO_ERROR) {
		pr_warn("Failed to add plugin manager id to BL-DTB\n");
		return err;
	}

	/* override BL-DTB as per board info */
	err = tegrabl_plugin_manager_overlay(bl_dtb);
#endif

	return err;
}

#if defined(CONFIG_ENABLE_SHELL)
void enter_shell_upon_user_request(void)
{
	uint32_t i;

	/* wait for 2 seconds for an input from user to enter SHELL */
	pr_info("Hit any key to stop autoboot:");
	tegrabl_enable_timestamp(false);

	for (i = 4; i > 0; i--) {
		if (tegrabl_getc_wait(500) > 0) {
			tegrabl_display_printf(GREEN, "BOOTLOADER SHELL MODE\n");
			tegrabl_printf("\n");
#if defined(CONFIG_ENABLE_WDT) /*disable wdt in shell*/
			tegrabl_wdt_disable(TEGRABL_WDT_LCCPLEX);
#endif
			(void)console_init();
			console_start();
			break;
		}

		tegrabl_printf("\t%d", i);
	}
	tegrabl_printf("\n");

	/*enable wdt again, after exiting shell*/
	wdt_enable();

	tegrabl_enable_timestamp(true);
}
#endif

void platform_init(void)
{
	tegrabl_error_t err = TEGRABL_NO_ERROR;
	struct tegrabl_device_config_params *dev_param = NULL;
#if defined(CONFIG_DT_SUPPORT)
	void *bl_dtb = NULL;
#endif
	bool is_cbo_read = true;

#if defined(CONFIG_ENABLE_STAGED_SCRUBBING)
	/* Staged scrubbing */
	if (boot_params->enable_dram_staged_scrubbing == 1ULL) {
		err = dram_staged_scrub();
		if (err != TEGRABL_NO_ERROR) {
			pr_error("dram scrubbing failed\n");
			goto fail;
		}
	}
#endif

	dev_param = &boot_params->device_config;
	TEGRABL_UNUSED(dev_param);

#if defined(CONFIG_ENABLE_DEVICE_PROD)
	err = tegrabl_device_prod_register(
			(uintptr_t)boot_params->controller_prod_settings,
			boot_params->controller_prod_settings_size);
	if (err != TEGRABL_NO_ERROR) {
		pr_warn("device prod register failed\n");
		err = TEGRABL_NO_ERROR;
	}
#endif

	/* Initialise the exit framework */
	err = tegrabl_exit_register();
	if (err != TEGRABL_NO_ERROR) {
		goto fail;
	}

#if defined(CONFIG_ENABLE_I2C)
	tegrabl_i2c_register();

	err = tegrabl_i2c_set_bus_freq_info(
			(uint32_t *)(uintptr_t)boot_params->i2c_bus_frequency_address,
			TEGRABL_MAX_I2C_BUSES_INDEX);
	if (err != TEGRABL_NO_ERROR) {
		pr_error("Error while saving i2c frequency info.\n");
		goto fail;
	}
#endif
	/* BL-dtb loaded mb2 is signed with sigheader */
	bl_dtb = (void *)boot_params->bl_dtb_load_address;
	err = tegrabl_dt_set_fdt_handle(TEGRABL_DT_BL, bl_dtb);
	if (err != TEGRABL_NO_ERROR) {
		pr_error("Bl-dtb init failed\n");
		goto fail;
	}
	pr_info("Bl_dtb @%p\n", bl_dtb);

	err = platform_init_bldtb_override(bl_dtb);
	if (err != TEGRABL_NO_ERROR) {
		pr_warn("Failed to override BL-DTB\n");
	}

#if defined(CONFIG_ENABLE_GPIO)
	gpio_framework_init();
	err = tegrabl_gpio_driver_init();
	if (err != TEGRABL_NO_ERROR) {
		pr_error("GPIO driver init failed\n");
		goto fail;
	}
	err = tegrabl_tca9539_init();
	if (err != TEGRABL_NO_ERROR) {
		pr_error("GPIO TCA9539 driver init failed\n");
		/* WAR for gpio not supporting device tree parsing
		 * Uncomment after adding device tree support
		 * goto fail;
		 */
	}
#endif

	err = platform_init_power();
	if (TEGRABL_NO_ERROR != err) {
		pr_debug("power init failed\n");
		goto fail;
	}

	/* configures fixed / fused storage devices */
	err = config_storage(dev_param, boot_params->storage_devices);
	if (err != TEGRABL_NO_ERROR) {
		pr_error("Error config_storage\n");
		goto fail;
	}

#if (defined(CONFIG_ENABLE_PARTITION_MANAGER))
	err = tegrabl_partition_manager_init();
	if (err != TEGRABL_NO_ERROR) {
		pr_error("Error in partition manager initialization.\n");
		goto fail;
	}
#endif

#if defined(CONFIG_ENABLE_NCT)
	err = tegrabl_nct_init();
	if (err != TEGRABL_NO_ERROR) {
		pr_warn("tegra_nct_init failed with ret:%d\n", err);
	}
#endif

	err = platform_disable_global_wdt(false);
	if (err != TEGRABL_NO_ERROR) {
		pr_warn("Failed to disable global wdt\n");
	}

#if defined(CONFIG_ENABLE_DISPLAY)
	err = tegrabl_display_init();
	if (err != TEGRABL_NO_ERROR) {
		pr_warn("display init failed\n");
	}
#endif

	pr_info("Load in CBoot Boot Options partition and parse it\n");
	err = tegrabl_read_cbo(CBO_PARTITION);
	if (err != TEGRABL_NO_ERROR) {
		pr_warn("%s: tegrabl_read_cbo failed with error %#x\n", __func__, err);
		is_cbo_read = false;
	}

	(void)tegrabl_cbo_parse_info(is_cbo_read);

#if defined(CONFIG_ENABLE_SHELL)
	enter_shell_upon_user_request();
#endif

fail:
	return;
}

status_t platform_init_heap(void)
{
	size_t heap_start;
	size_t heap_end;
	size_t heap_size;

	heap_start = (uintptr_t)&_end;
	heap_end = boot_params->cpubl_carveout_safe_end_offset;
	heap_size = heap_end - heap_start;

	pr_info("Heap: [0x%zx ... 0x%zx]\n", heap_start, heap_end);

	return tegrabl_heap_init(TEGRABL_HEAP_DEFAULT, heap_start, heap_size);
}

void tegrabl_reset_prepare(void)
{
	tegrabl_error_t err = TEGRABL_NO_ERROR;

	if (!tegrabl_is_fpga()) {
		err = tegrabl_uphy_suspend();
		if (err != TEGRABL_NO_ERROR) {
			return;
		}
	}

	if (is_comb_uart_initialized) {
#if defined(CONFIG_ENABLE_COMB_UART)
		(void)tegrabl_comb_uart_hw_flush();
#endif
	}
}
