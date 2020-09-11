/*
 * Copyright (c) 2015-2019, NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#define MODULE	TEGRABL_ERR_SOCMISC

#include "build_config.h"
#include <stdint.h>
#include <tegrabl_error.h>
#include <tegrabl_io.h>
#include <tegrabl_debug.h>
#include <tegrabl_drf.h>
#include <tegrabl_ar_macro.h>
#include <tegrabl_blockdev.h>
#include <tegrabl_addressmap.h>
#include <tegrabl_partition_loader.h>
#include <tegrabl_soc_misc.h>
#include <tegrabl_fuse.h>
#include <tegrabl_clock.h>
#include <arpmc_impl.h>
#include <armiscreg.h>
#include <arscratch.h>
#include <arhsp_dbell.h>
#include <arfuse.h>
#include <tegrabl_odmdata_soc.h>
#include <tegrabl_i2c.h>
#include <string.h>
#include <address_map_new.h>
#include <aruphy_lane.h>
#include <aruphy_pll.h>
#include <tegrabl_uphy_soc.h>
#include <tegrabl_reset_prepare.h>

#define CONFIG_PROD_CONTROLLER_SHIFT 16U

#define TOP0_DBELL_WRITE(reg, val)		\
	tegrabl_trace_write32((uint32_t)NV_ADDRESS_MAP_TOP0_HSP_DB_0_BASE +		\
								(reg), (val))
#define TOP0_DBELL_READ(reg)			\
	tegrabl_trace_read32((uint32_t)NV_ADDRESS_MAP_TOP0_HSP_DB_0_BASE + (reg))

#define PMC_READ(reg)						\
		NV_READ32((uint32_t)NV_ADDRESS_MAP_PMC_IMPL_BASE + (uint32_t)PMC_IMPL_##reg##_0)
#define PMC_WRITE(reg, val)					\
		NV_WRITE32(NV_ADDRESS_MAP_PMC_IMPL_BASE + PMC_IMPL_##reg##_0, val)

#define SCRATCH_WRITE(reg, val)			\
		NV_WRITE32((uint32_t)NV_ADDRESS_MAP_SCRATCH_BASE + (uint32_t)SCRATCH_##reg, val)
#define SCRATCH_READ(reg)			\
		NV_READ32((uint32_t)NV_ADDRESS_MAP_SCRATCH_BASE + (uint32_t)SCRATCH_##reg)

#define MISCREG_READ(reg) NV_READ32(NV_ADDRESS_MAP_MISC_BASE + (reg))

#define DBELL_ACK_TIMEOUT_US 1000000 /* us */
#define DBELL_ACK_POLL_INTERVAL_US 5U /* us */

#define RECOVERY_BOOT_CHAIN_VALUE 0x1U
#define RECOVERY_BOOT_CHAIN_MASK  0x1U
#define BOOT_CHAIN_INVALID_MASK 0x2U
#define BOOT_CHAIN_RETRY_SHIFT	16U
#define BOOT_CHAIN_RETRY_MASK	(0xFUL << BOOT_CHAIN_RETRY_SHIFT)

#define HSM_RESET_UNCORRECTABLE_ERROR_MAGIC 0xD0A00ECCUL

uint32_t read_miscreg_strap(tegrabl_strap_field_t fld)
{
	static uint32_t boot_dev_val;
	static uint32_t ram_code_val;
	static bool is_strap_read;
	uint32_t retval;

	/* Strap register (MISCREG_STRAP_STRAPPING_OPT_A_0) is cached in
	 * SCRATCH_SECURE_RSV0_SCRATCH_0 by BootROM on L0 cold-boot */
	if (!is_strap_read) {
		uint32_t strap_val = NV_READ32(NV_ADDRESS_MAP_SCRATCH_BASE +
									   SCRATCH_SECURE_RSV0_SCRATCH_0);

		/* store the values and keep it future references */
		boot_dev_val = NV_DRF_VAL(MISCREG, STRAP_STRAPPING_OPT_A,
					  BOOT_SELECT, strap_val);

#if !defined(CONFIG_ENABLE_MEMBCT)
		/* use lower 2 bits for RAM_CODE */
		ram_code_val = (NV_DRF_VAL(MISCREG, STRAP_STRAPPING_OPT_A,
					   RAM_CODE, strap_val)) & 0x3U;
#else
		ram_code_val = (NV_DRF_VAL(MISCREG, STRAP_STRAPPING_OPT_A, RAM_CODE, strap_val));
#endif

		is_strap_read = true;
	}

	switch (fld) {
	case BOOT_SELECT_FIELD:
		retval = boot_dev_val;
		break;
	case RAM_CODE_FIELD:
		retval = ram_code_val;
		break;
	default:
		retval = 0;
		break;
	}

	return retval;
}

tegrabl_error_t tegrabl_soc_get_bootdev(
		tegrabl_storage_type_t *device, uint32_t *instance)
{
	tegrabl_error_t err = TEGRABL_NO_ERROR;
	tegrabl_fuse_boot_dev_t fdev;

	if ((device == NULL) || (instance == NULL)) {
		err = TEGRABL_ERROR(TEGRABL_ERR_INVALID, 0);
		goto fail;
	}

	/*
	  1) Read skip dev selection strap
	  2) If it is set, read boot device from fuse
	     if not, and strap option does not say _usefuse",
	     read from "SECURE_RSV0_SCRATCH_0" register.

	  TEGRABL_FUSE_BOOT_DEV_RESVD_4 is to represent
	  to read boot device from fuses
	*/
	fdev = read_miscreg_strap(BOOT_SELECT_FIELD);
	if (tegrabl_fuse_ignore_dev_sel_straps() || (fdev == TEGRABL_FUSE_BOOT_DEV_RESVD_4)) {
		/* Read boot dev value from fuse */
		err = tegrabl_fuse_read(FUSE_SEC_BOOTDEV, (uint32_t *)&fdev,
			sizeof(tegrabl_fuse_boot_dev_t));
		if (TEGRABL_NO_ERROR != err) {
			TEGRABL_SET_HIGHEST_MODULE(err);
			pr_trace("Failed to read sec boot device from fuse\n");
			goto fail;
		}
	}

	switch (fdev) {
	case TEGRABL_FUSE_BOOT_DEV_SDMMC:
		*device = TEGRABL_STORAGE_SDMMC_BOOT;
		*instance = 3;
		pr_info("Boot-device: eMMC\n");
		break;
	case TEGRABL_FUSE_BOOT_DEV_SPIFLASH:
		*device = TEGRABL_STORAGE_QSPI_FLASH;
		*instance = 0;
		pr_info("Boot-device: QSPI\n");
		break;
	case TEGRABL_FUSE_BOOT_DEV_SATA:
		*device = TEGRABL_STORAGE_SATA;
		*instance = 0;
		pr_info("Boot-device: SATA\n");
		break;
	case TEGRABL_FUSE_BOOT_DEV_UFS:
		*device = TEGRABL_STORAGE_UFS;
		*instance = 0;
		pr_info("Boot-device: UFS\n");
		break;
	default:
		pr_error("Unsupported boot-device strap-reg: 0x%08x\n", fdev);
		err = TEGRABL_ERROR(TEGRABL_ERR_NOT_SUPPORTED, 0);
		break;
	}

fail:
	return err;
}

void tegrabl_print_rst_status(void)
{
	uint32_t reg_val = 0;

	reg_val = PMC_READ(RST_STATUS);

	pr_info("rst_source : 0x%x\n",
			(uint16_t)NV_DRF_VAL(PMC_IMPL, RST_STATUS, RST_SOURCE, reg_val));

	pr_info("rst_level : 0x%x\n",
			(uint8_t)NV_DRF_VAL(PMC_IMPL, RST_STATUS, RST_LEVEL, reg_val));
}

tegrabl_error_t tegrabl_get_rst_status(tegrabl_rst_source_t *rst_source,
									   tegrabl_rst_level_t *rst_level)
{
	uint32_t reg_val = 0;

	if ((rst_source == NULL) && (rst_level == NULL)) {
		return TEGRABL_ERROR(TEGRABL_ERR_INVALID, 0);
	}

	reg_val = PMC_READ(RST_STATUS);

	if (rst_source != NULL) {
		*rst_source = NV_DRF_VAL(PMC_IMPL, RST_STATUS, RST_SOURCE, reg_val);
	}
	if (rst_level != NULL) {
		*rst_level = NV_DRF_VAL(PMC_IMPL, RST_STATUS, RST_LEVEL, reg_val);
	}
	return TEGRABL_NO_ERROR;
}

bool tegrabl_rst_is_sc8_exit(void)
{
	uint32_t rst_source;

	if (tegrabl_get_rst_status(&rst_source, NULL) != TEGRABL_NO_ERROR) {
		return false;
	}
	if (rst_source == RST_SOURCE_SC7) {
		return ((PMC_READ(SC7_CONFIG) & 0x1U) == 1U) ? true : false;
	}
	return false;
}

bool tegrabl_rst_is_l0_l1a(void)
{
	bool ret = true;
	tegrabl_rst_source_t rst_source;
	tegrabl_rst_level_t rst_level;

	if (tegrabl_get_rst_status(&rst_source, &rst_level) == TEGRABL_NO_ERROR) {
		switch (rst_level) {
		case 0:
			ret = true;
			break;
		case 1:
			switch (rst_source) {
			case RST_SOURCE_AOWDT:
			case RST_SOURCE_BCCPLEXWDT:
			case RST_SOURCE_BPMPWDT:
			case RST_SOURCE_SCEWDT:
			case RST_SOURCE_SPEWDT:
			case RST_SOURCE_APEWDT:
			case RST_SOURCE_LCCPLEXWDT:
			case RST_SOURCE_MAINSWRST:
			case RST_SOURCE_RCEEWDT:
			case RST_SOURCE_PVA0WDT:
			case RST_SOURCE_PVA1WDT:
				ret = false;
				break;
			default:
				ret = true;
				break;
			}
			break;
		default:
			ret = false;
			break;
		}
	}

	return ret;
}

tegrabl_error_t tegrabl_enable_soc_therm(void)
{
	tegrabl_error_t err = TEGRABL_NO_ERROR;

	err = tegrabl_car_clk_enable(TEGRABL_MODULE_SOC_THERM, 0, NULL);
	if (err != TEGRABL_NO_ERROR) {
		return err;
	}
	err = tegrabl_car_rst_clear(TEGRABL_MODULE_SOC_THERM, 0);
	if (err != TEGRABL_NO_ERROR) {
		return err;
	}
	return err;
}

void tegrabl_boot_recovery_mode(void)
{
	pr_debug("Setting recovery flag\n");
	(void)tegrabl_set_pmc_scratch0_flag(TEGRABL_PMC_SCRATCH0_FLAG_FORCED_RECOVERY, true);

	pr_debug("Resetting\n");
	tegrabl_pmc_reset();
}

tegrabl_error_t tegrabl_uphy_suspend(void)
{
	uint32_t base_address = 0, reg = 0;
	uint32_t idx = 0;
	bool uphy_global_rst, pll_rst, lane_rst;
	tegrabl_error_t err = TEGRABL_NO_ERROR;

	err = tegrabl_car_clk_get_reset_state(TEGRABL_MODULE_PEX_USB_UPHY, TEGRABL_CLK_PEX_USB_UPHY_RST,
					     &uphy_global_rst);
	if (err != TEGRABL_NO_ERROR) {
		return err;
	}
	if (uphy_global_rst == true) {
		pr_debug("UPHY is in reset, skip uphy_suspend\n");
		TEGRABL_UNUSED(uphy_lanes);
		TEGRABL_UNUSED(uphy_pll);
		return err;
	}

	/* Set RX_SLEEP, RX_IDDQ, TX_SLEEP, TX_IDDQ bits for all lanes */
	for (idx = 0; idx < ARRAY_SIZE(uphy_lanes); idx++) {
		base_address = uphy_lanes[idx].base;
		err = tegrabl_car_clk_get_reset_state(TEGRABL_MODULE_PEX_USB_UPHY, (uint8_t)uphy_lanes[idx].module_id,
						     &lane_rst);
		if (err != TEGRABL_NO_ERROR) {
			pr_debug("Failed to get reset status for lane%d\n", idx);
			return err;
		}
		if (lane_rst == true) {
			pr_trace("UPHY: lane %d is in reset\n", idx);
			continue;
		}
		reg = NV_FLD_SET_DRF_NUM(UPHY_LANE, MISC_CTL_1, RX_SLEEP, 1U, reg);
		reg = NV_FLD_SET_DRF_NUM(UPHY_LANE, MISC_CTL_1, RX_IDDQ, 1U, reg);
		reg = NV_FLD_SET_DRF_NUM(UPHY_LANE, MISC_CTL_1, TX_SLEEP, 1U, reg);
		reg = NV_FLD_SET_DRF_NUM(UPHY_LANE, MISC_CTL_1, TX_IDDQ, 1U, reg);
		NV_WRITE32(base_address + (uint32_t) UPHY_LANE_MISC_CTL_1_0, reg);
	}

	/* Clear ENABLE bit, Set SLEEP, IDDQ bits for all PLL's */
	for (idx = 0; idx < ARRAY_SIZE(uphy_pll); idx++) {
		base_address = uphy_pll[idx].base;
		err = tegrabl_car_clk_get_reset_state(TEGRABL_MODULE_PEX_USB_UPHY, (uint8_t)uphy_pll[idx].module_id,
						     &pll_rst);
		if (err != TEGRABL_NO_ERROR) {
			pr_debug("Failed to get reset status for pll%d\n", idx);
			return err;
		}
		if (pll_rst == true) {
			pr_debug("PLL%d is in reset, skip suspend\n", idx);
			continue;
		}
		reg = NV_FLD_SET_DRF_NUM(UPHY_PLL, CTL_1, ENABLE, 0U, reg);
		NV_WRITE32(base_address + (uint32_t) UPHY_PLL_CTL_1_0, reg);

		reg = NV_FLD_SET_DRF_NUM(UPHY_PLL, CTL_1, SLEEP, 1U, reg);
		reg = NV_FLD_SET_DRF_NUM(UPHY_PLL, CTL_1, IDDQ, 1U, reg);
		NV_WRITE32(base_address + (uint32_t) UPHY_PLL_CTL_1_0, reg);
	}
	return err;
}

void tegrabl_pmc_reset(void)
{
	uint32_t reg = 0;

	tegrabl_reset_prepare();

	reg = PMC_READ(CNTRL);
	reg = NV_FLD_SET_DRF_DEF(PMC_IMPL, CNTRL, MAIN_RST, ENABLE, reg);
	PMC_WRITE(CNTRL, reg);
}

void tegrabl_clear_pmc_rsvd()
{
	/* return in case of SC8 exit */
	if (tegrabl_rst_is_sc8_exit()) {
		return;
	}

	/* Sane initialisation of all reset IP */
	SCRATCH_WRITE(SECURE_RSV2_SCRATCH_0, 0);
	SCRATCH_WRITE(SECURE_RSV2_SCRATCH_1, 0);

	SCRATCH_WRITE(SECURE_RSV3_SCRATCH_0, 0);
	SCRATCH_WRITE(SECURE_RSV3_SCRATCH_1, 0);

	SCRATCH_WRITE(SECURE_RSV4_SCRATCH_0, 0);
	SCRATCH_WRITE(SECURE_RSV4_SCRATCH_1, 0);

	SCRATCH_WRITE(SECURE_RSV5_SCRATCH_0, 0);
	SCRATCH_WRITE(SECURE_RSV5_SCRATCH_1, 0);

	SCRATCH_WRITE(SECURE_RSV6_SCRATCH_0, 0);
	SCRATCH_WRITE(SECURE_RSV6_SCRATCH_1, 0);

	SCRATCH_WRITE(SECURE_RSV7_SCRATCH_0, 0);
	SCRATCH_WRITE(SECURE_RSV7_SCRATCH_1, 0);
}

tegrabl_error_t tegrabl_set_pmc_scratch0_flag(
		tegrabl_scratch0_flag_t flag, bool set)
{
	uint32_t reg;
	tegrabl_error_t ret = TEGRABL_NO_ERROR;

	reg = SCRATCH_READ(SCRATCH0_0);
	switch (flag) {
	case TEGRABL_PMC_SCRATCH0_FLAG_FORCED_RECOVERY:
	case TEGRABL_PMC_SCRATCH0_FLAG_BOOT_RECOVERY_KERNEL:
	case TEGRABL_PMC_SCRATCH0_FLAG_FASTBOOT:
		if (set) {
			reg |= (1UL << flag);
		} else {
			reg &= ~(1UL << flag);
		}

		SCRATCH_WRITE(SCRATCH0_0, reg);
		break;
	default:
		pr_error("Flag %u not handled\n", flag);
		ret = TEGRABL_ERROR(TEGRABL_ERR_NOT_SUPPORTED, 0);
		break;
	}

	return ret;
}

tegrabl_error_t tegrabl_get_pmc_scratch0_flag(
	tegrabl_scratch0_flag_t flag, bool *is_set)
{
	uint32_t reg;
	if (is_set == NULL) {
		return TEGRABL_ERROR(TEGRABL_ERR_INVALID, 0);
	}
	reg = SCRATCH_READ(SCRATCH0_0);
	switch (flag) {
	case TEGRABL_PMC_SCRATCH0_FLAG_FORCED_RECOVERY:
	case TEGRABL_PMC_SCRATCH0_FLAG_BOOT_RECOVERY_KERNEL:
	case TEGRABL_PMC_SCRATCH0_FLAG_FASTBOOT:
		*is_set = ((reg & (1UL << flag)) != 0UL) ? true : false;
		break;
	default:
		pr_critical("Flag %u not handled\n", flag);
		break;
	}
	return TEGRABL_NO_ERROR;
}

tegrabl_error_t tegrabl_set_soc_core_voltage(uint32_t soc_mv)
{
	TEGRABL_UNUSED(soc_mv);
	/* Dummy function */
	return TEGRABL_NO_ERROR;
}

tegrabl_boot_chain_type_t tegrabl_get_boot_chain_type(void)
{
	uint32_t reg = 0;

	reg = SCRATCH_READ(SCRATCH_99);

	if ((reg & RECOVERY_BOOT_CHAIN_MASK) == RECOVERY_BOOT_CHAIN_VALUE) {
		return TEGRABL_BOOT_CHAIN_RECOVERY;
	}
	return TEGRABL_BOOT_CHAIN_PRIMARY;
}

void tegrabl_set_boot_chain_type(tegrabl_boot_chain_type_t boot_chain)
{
	uint32_t reg = 0;

	reg = SCRATCH_READ(SCRATCH_99);
	reg = reg & ~RECOVERY_BOOT_CHAIN_MASK;

	if (boot_chain == TEGRABL_BOOT_CHAIN_RECOVERY) {
		reg = reg | RECOVERY_BOOT_CHAIN_VALUE;
	}

	SCRATCH_WRITE(SCRATCH_99, reg);
}

void tegrabl_reset_fallback_scratch(void)
{
	SCRATCH_WRITE(SCRATCH_99, 0);
}

void tegrabl_trigger_fallback_boot_chain(const uint32_t bootchain_max_retries)
{
	uint32_t reg = 0;
	uint32_t boot_chain_retry;

	reg = SCRATCH_READ(SCRATCH_99);

	boot_chain_retry = (reg & BOOT_CHAIN_RETRY_MASK) >> BOOT_CHAIN_RETRY_SHIFT;

	/* Retry the same bootchain if MAX_RETRIES is not elapsed*/
	if (boot_chain_retry < bootchain_max_retries) {
		/* increment value of retry */
		reg += (1UL << BOOT_CHAIN_RETRY_SHIFT);
		SCRATCH_WRITE(SCRATCH_99, reg);
		pr_critical("Retry_%u : Resetting device for the same boot chain\n", boot_chain_retry);
		tegrabl_pmc_reset();
		pr_critical("Should not be reaching here.\n");
		while (true) {
			;
		}
	}

	/* check if other chain is corrupted or not by checking bit[1]*/
	if ((reg  & BOOT_CHAIN_INVALID_MASK) == BOOT_CHAIN_INVALID_MASK) {
		pr_critical("The backup boot chain is corrupted.\n");
		pr_critical("Clearing Scratch_99 and entering RCM.\n");
		tegrabl_reset_fallback_scratch();
		/*enter recovery*/
		tegrabl_boot_recovery_mode();
		pr_critical("Should not be reaching here.\n");
		while (true) {
			;
		}
	}

	if ((reg & RECOVERY_BOOT_CHAIN_MASK) != RECOVERY_BOOT_CHAIN_VALUE) {
		pr_critical("Resetting device for recovery boot chain\n");
	} else {
		pr_critical("Resetting device for primary boot chain\n");
	}

	/*mark the current chain as corrupted*/
	reg |= BOOT_CHAIN_INVALID_MASK;
	/*toggle bit[0] to boot from other chain*/
	reg ^= (0x1U);
	/* clear the boot_chain_retry count, as there should be retry for other chain */
	reg = reg & (~BOOT_CHAIN_RETRY_MASK);

	SCRATCH_WRITE(SCRATCH_99, reg);

	tegrabl_pmc_reset();

	pr_critical("Should not be reaching here.\n");
	while (true) {
		;
	}
}

bool tegrabl_is_scratch_dram_dbe_flag_set(void)
{
	uint32_t reg;

	reg = SCRATCH_READ(SECURE_RSV74_SCRATCH_0);

	return (reg == HSM_RESET_UNCORRECTABLE_ERROR_MAGIC);
}

uint32_t tegrabl_get_bad_dram_page_number(void)
{
	return SCRATCH_READ(SECURE_RSV74_SCRATCH_1);
}

bool tegrabl_is_fpga(void)
{
	uint32_t reg;

	reg = NV_READ32((uint32_t)NV_ADDRESS_MAP_MISC_BASE + (uint32_t)MISCREG_HIDREV_0);

	return (NV_DRF_VAL(MISCREG, HIDREV, PRE_SI_PLATFORM, reg) ==
			MISCREG_HIDREV_0_PRE_SI_PLATFORM_SYSTEM_FPGA);
}

bool tegrabl_is_vdk(void)
{
	uint32_t reg;

	reg = NV_READ32(NV_ADDRESS_MAP_MISC_BASE + MISCREG_HIDREV_0);

	return (NV_DRF_VAL(MISCREG, HIDREV, PRE_SI_PLATFORM, reg) ==
			MISCREG_HIDREV_0_PRE_SI_PLATFORM_VDK);
}

void tegrabl_get_chip_info(struct tegrabl_chip_info *info)
{
	uint32_t reg;

	if (info != NULL) {
		reg = REG_READ(MISC, MISCREG_HIDREV);
		info->chip_id = NV_DRF_VAL(MISCREG, HIDREV, CHIPID, reg);
		info->major = NV_DRF_VAL(MISCREG, HIDREV, MAJORREV, reg);
		info->minor = NV_DRF_VAL(MISCREG, HIDREV, MINORREV, reg);
		info->pre_si_platform = NV_DRF_VAL(MISCREG, HIDREV, PRE_SI_PLATFORM, reg);

		info->revision = NV_READ32((uint32_t)NV_ADDRESS_MAP_FUSE_BASE + (uint32_t)FUSE_OPT_SUBREVISION_0);

		reg = REG_READ(MISC, MISCREG_EMU_REVID);
		info->emulation_rev = NV_DRF_VAL(MISCREG, EMU_REVID, BOARD, reg);
	}
}

tegrabl_error_t tegrabl_dbell_trigger(uint32_t source, uint32_t target)
{
	uint32_t offset = 0;
	uint32_t mask = 0;

	if ((target >= TEGRABL_DBELL_TARGET_MAX) || (source >= TEGRABL_DBELL_CLIENT_MAX)) {
		pr_error("Doorbell target/source not supported\n");
		return TEGRABL_ERROR(TEGRABL_ERR_NOT_SUPPORTED, 2);
	}

	offset = (((uint32_t)HSP_DBELL_1_TRIGGER_0 -
								(uint32_t)HSP_DBELL_0_TRIGGER_0) * target);

	/* The source client has the following format :
	Bit 0 : Reserved
	Bit 1 :CCPLEX
	Bit 2 : DPMU
	Bit 3 : BPMP
	Bit 4 : SPE
	Bit 5 : SCE
	Bit 6 : DMA
	Bit 7 : TSECA
	Bit 8 : TSECB
	Bit 9 : JTAGM
	Bit 10 : CSITE
	Bit 11 : APE
	Bit 12 : PEATRANS
	Bit 13 - Bit 15 : Reserved
	Bit 16 - Bit 31 : Same strucutre as the 16 LSB
	*/
	mask = mask | (1UL << source) | (1UL << (source + 16U));

	TOP0_DBELL_WRITE((uint32_t)HSP_DBELL_0_ENABLE_0 + offset, mask);
	TOP0_DBELL_WRITE((uint32_t)HSP_DBELL_0_TRIGGER_0 + offset, 0x1); /* any value is ok */
	(void)TOP0_DBELL_READ((uint32_t)HSP_DBELL_0_RAW_0 + offset);
	pr_trace("Triggered doorbell- source:%u, target: %u\n", source, target);

	return TEGRABL_NO_ERROR;
}

uint32_t tegrabl_dbell_ack(uint32_t recipient)
{
	uint32_t ack = 0;
	uint32_t mask = 0;
	uint32_t timeout = 0;

	mask = mask | (1UL << recipient) | (1UL << (recipient + 16U));

	/* poll till pending is cleared */
	timeout = DBELL_ACK_TIMEOUT_US;
	while (timeout != 0U) {
		ack = TOP0_DBELL_READ((uint32_t)HSP_DBELL_0_PENDING_0);
		if ((ack & mask) == 0U) {
			break;
		}
		tegrabl_udelay(DBELL_ACK_POLL_INTERVAL_US);
		timeout -= DBELL_ACK_POLL_INTERVAL_US;
	}

	return ack;
}

bool tegrabl_is_wdt_enable(void)
{
	uint32_t reg_val = 0;
	uint32_t halt_in_fiq = 0;

	reg_val = PMC_READ(RAMDUMP_CTL_STATUS);
	halt_in_fiq = NV_DRF_VAL(PMC_IMPL, RAMDUMP_CTL_STATUS, HALT_IN_FIQ, reg_val);
	return (halt_in_fiq == 0UL);
}

tegrabl_binary_type_t tegrabl_get_kernel_type(void)
{
	bool boot_recovery_kernel = false;
	tegrabl_binary_type_t bin_type;
	tegrabl_error_t error = TEGRABL_NO_ERROR;

	error = tegrabl_get_pmc_scratch0_flag(TEGRABL_PMC_SCRATCH0_FLAG_BOOT_RECOVERY_KERNEL,
										  &boot_recovery_kernel);
	TEGRABL_ASSERT(error == TEGRABL_NO_ERROR);
	TEGRABL_UNUSED(error);

	if (boot_recovery_kernel == false) {
		bin_type = TEGRABL_BINARY_KERNEL;
		pr_info("Kernel type = Normal\n");
	} else {
		bin_type = TEGRABL_BINARY_RECOVERY_KERNEL;
		pr_info("Kernel type = Recovery\n");
	}

	return bin_type;
}

void tegrabl_set_boot_slot_reg(uint32_t slot_info)
{
	SCRATCH_WRITE(SCRATCH_15, slot_info);
}

uint32_t tegrabl_get_boot_slot_reg(void)
{
	return SCRATCH_READ(SCRATCH_15);
}

#define FUSE_ECID_MAX_SIZE 4UL /* in Bytes */
tegrabl_error_t tegrabl_get_ecid_str(char *ecid_str, uint32_t size)
{
	tegrabl_error_t err;
	uint32_t ecid[FUSE_ECID_MAX_SIZE];
	uint32_t ecid_size;
	uint32_t *ptr;

	if (ecid_str == NULL) {
		pr_error("Invalid ECID addr\n");
		err = TEGRABL_ERROR(TEGRABL_ERR_INVALID, 0);
		goto done;
	}

	err = tegrabl_fuse_query_size(FUSE_UID, &ecid_size);
	if (err != TEGRABL_NO_ERROR) {
		pr_error("Failed to query size of ECID\n");
		TEGRABL_SET_HIGHEST_MODULE(err);
		goto done;
	}
	if ((ecid_size > (FUSE_ECID_MAX_SIZE * sizeof(uint32_t))) || (size < (ecid_size * 2UL))) {
		pr_error("Not enough buffer for ECID\n");
		err = TEGRABL_ERROR(TEGRABL_ERR_NO_MEMORY, 0);
		goto done;
	}

	err = tegrabl_fuse_read(FUSE_UID, ecid, ecid_size);
	if (err != TEGRABL_NO_ERROR) {
		pr_error("Failed to read ECID\n");
		TEGRABL_SET_HIGHEST_MODULE(err);
		goto done;
	}

	/* Transfer ECID to string */
	for (ptr = ecid + ecid_size / sizeof(uint32_t) - 1; ptr >= ecid; ptr--) {
		tegrabl_snprintf(ecid_str, size, "%s%08x", ecid_str, *ptr);
	}

done:
	return err;
}
