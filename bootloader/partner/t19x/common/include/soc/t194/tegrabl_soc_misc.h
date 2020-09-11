/*
 * Copyright (c) 2015-2018, NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#ifndef INCLUDED_TEGRABL_SOC_MISC_H
#define INCLUDED_TEGRABL_SOC_MISC_H

/* source/recipient of doorbell ring */
#define	TEGRABL_DBELL_CLIENT_CCPLEX 0x1U
#define	TEGRABL_DBELL_CLIENT_DPMU 0x2U
#define	TEGRABL_DBELL_CLIENT_BPMP 0x3U
#define	TEGRABL_DBELL_CLIENT_SPE 0x4U
#define TEGRABL_DBELL_CLIENT_SCE 0x5U
#define TEGRABL_DBELL_CLIENT_DMA 0x6U
#define	TEGRABL_DBELL_CLIENT_TSECA 0x7U
#define	TEGRABL_DBELL_CLIENT_TSECB 0x8U
#define	TEGRABL_DBELL_CLIENT_JTAGM 0x9U
#define	TEGRABL_DBELL_CLIENT_CSITE 0xAU
#define	TEGRABL_DBELL_CLIENT_APE 0xBU
#define	TEGRABL_DBELL_CLIENT_MAX 0xCU

/* target for doorbell ring */
#define	TEGRABL_DBELL_TARGET_DPMU 0x0U
#define	TEGRABL_DBELL_TARGET_CCPLEX_TZ_NS 0x1U
#define	TEGRABL_DBELL_TARGET_CCPLEX_TZ_S 0x2U
#define	TEGRABL_DBELL_TARGET_BPMP 0x3U
#define	TEGRABL_DBELL_TARGET_SPE 0x4U
#define	TEGRABL_DBELL_TARGET_SCE 0x5U
#define	TEGRABL_DBELL_TARGET_APE 0x6U
#define	TEGRABL_DBELL_TARGET_MAX 0x7U


#include "build_config.h"
#include <stdint.h>
#include <stdbool.h>
#include <tegrabl_error.h>
#include <tegrabl_blockdev.h>
#include <tegrabl_binary_types.h>

/*
 * @brief specifies the reset source
 */
/* macro tegrabl rst source */
typedef uint32_t tegrabl_rst_source_t;
#define RST_SOURCE_SYS_RESET_N 0U
#define RST_SOURCE_AOWDT 1U
#define RST_SOURCE_BCCPLEXWDT 2U
#define RST_SOURCE_BPMPWDT 3U
#define RST_SOURCE_SCEWDT 4U
#define RST_SOURCE_SPEWDT 5U
#define RST_SOURCE_APEWDT 6U
#define RST_SOURCE_LCCPLEXWDT 7U
#define RST_SOURCE_SENSOR 8U
#define RST_SOURCE_AOTAG 9U
#define RST_SOURCE_VFSENSOR 10U
#define RST_SOURCE_MAINSWRST 11U
#define RST_SOURCE_SC7 12U
#define RST_SOURCE_HSM 13U
#define RST_SOURCE_CSITE 14U
#define RST_SOURCE_RCEEWDT 15U
#define RST_SOURCE_PVA0WDT 16U
#define RST_SOURCE_PVA1WDT 17U
#define RST_SOURCE_L1A_ASYNC 18U
#define RST_SOURCE_BPMPBOOT 19U
#define RST_SOURCE_FUSECRC 20U

/**
 * @brief specifies boot type either normal cold boot or recovery cold boot.
 */
/* macro tegrabl boot chain type */
typedef uint32_t tegrabl_boot_chain_type_t;
#define TEGRABL_BOOT_CHAIN_PRIMARY 0U
#define TEGRABL_BOOT_CHAIN_RECOVERY 1U
#define	TEGRABL_BOOT_CHAIN_MAX 2U

/*
 * @brief specifies the fields in the miscreg strap register
 */
/* macro tegrabl strap field */
typedef uint32_t tegrabl_strap_field_t;
#define BOOT_SELECT_FIELD 0U
#define RAM_CODE_FIELD 1U

/*
 * @brief specifies the reset level
 */
/* macro tegrabl rst level */
typedef uint32_t tegrabl_rst_level_t;
#define RST_LEVEL_L0 0U
#define RST_LEVEL_L1 1U
#define RST_LEVEL_L2 2U
#define RST_LEVEL_WARM 3U

/* pmc scratch0 bit positions */
/* macro tegrabl scratch0 flag */
typedef uint32_t tegrabl_scratch0_flag_t;
#define TEGRABL_PMC_SCRATCH0_FLAG_FORCED_RECOVERY 1U
#define TEGRABL_PMC_SCRATCH0_FLAG_FASTBOOT 30U
#define TEGRABL_PMC_SCRATCH0_FLAG_BOOT_RECOVERY_KERNEL 31U

#define SYSTEM_FPGA_BASE	1U
#define SYSTEM_FPGA_MAX		2U
#define SYSTEM_FPGA_GPU		3U
/*
 * @brief struct to hold chip info read from register
 */
struct tegrabl_chip_info {
	uint32_t chip_id;
	uint32_t major;
	uint32_t minor;
	uint32_t revision;
	uint32_t pre_si_platform;
	uint32_t emulation_rev;
};

/**
 * @brief api to trigger the doorbell
 *
 * @param source specifies the source of the doorbell
 * @param target specifies the target of the doorbell
 *
 * @return TEGRABL_NO_ERROR in case of success,
 */
tegrabl_error_t tegrabl_dbell_trigger(uint32_t source, uint32_t target);

/**
 * @brief api to capture the ack the doorbell
 *
 * @param client specifies the recipient of the ack of the doorbell
 *
 * @return ack status of the doorbel register
 */
uint32_t tegrabl_dbell_ack(uint32_t recipient);

/**
 * @brief Check tegra-ap-wdt bit of odmdata 0:Tegra AP watchdog disable
 * and value of HALT_IN_FIQ to determine whether to configure watchdog
 * or not
 *
 * @return bool value specifying whether to do any wdt operation or not
.*
 */
bool tegrabl_is_wdt_enable(void);

/**
 * @brief Register prod settings to the respective module driver.
 *
 * @param prod_settings Buffer pointing to set of prod settings
 * @param num_settings Number of prod settings sets
 *
 * @return TEGRABL_NO_ERROR if successful else appropriate error.
 */
tegrabl_error_t tegrabl_register_prod_settings(uint32_t *prod_settings,
											   uint64_t num_settings);

/**
 * @brief read given strap field from miscreg strap register
 *
 * @param fld specifies field from miscreg strap register
 *
 * @return returns requested strap field value,
 */
uint32_t read_miscreg_strap(tegrabl_strap_field_t fld);

/**
 * @brief print rst_source and rst_level
 */
void tegrabl_print_rst_status(void);

/**
 * @brief Get either rst_source or rst_level of the device or both
 *
 * @param rst_source a pointer where enum tegrabl_rst_source will be stored
 * @param rst_level a pointer where enum tegrabl_rst_level will be stored
 *
 * @return TEGRABL_NO_ERROR in case of success,
 */
tegrabl_error_t tegrabl_get_rst_status(tegrabl_rst_source_t *rst_source,
									   tegrabl_rst_level_t *rst_level);

/**
 * @brief Determine if the device is waking up from sc8 or not
 *
 * @return true if sc8 else false,
 */
bool tegrabl_rst_is_sc8_exit(void);

/**
 * @brief Determine if it was L0 or L1A reset
 *
 * @return true in case of L0/L1A boot
 */
bool tegrabl_rst_is_l0_l1a(void);

/**
 * @brief Get the secondary boot-device type and instance for the SOC
 *
 * @param device Output parameter to hold boot-device type
 * @param instance Output parameer to hold boot-device instance
 *
 * @return TEGRABL_NO_ERROR in case of success,
 * TEGRABL_ERR_INVALID in case either device/instance are NULL,
 * TEGRABL_ERR_NOT_SUPPORTED if the boot-device determined from straps
 * or fuses is not supported.
 */
tegrabl_error_t tegrabl_soc_get_bootdev(
		tegrabl_storage_type_t *device, uint32_t *instance);

#if defined(CONFIG_ENABLE_APB2JTAG_WAR)
/**
 * @brief api to invoke the apb2jtag war routine
 *
 * @param length specifies the lsb length of the jtag register
 * @param instr_id specifies the instruction id for the jtag register
 * @param cluster specifies the cluster id for the jtag register
 */
void apb2jtag_mb1(uint32_t length, uint32_t instr_id, uint32_t cluster);
#endif

/** @brief Enable soc therm
 *
 *  @return TEGRABL_NO_ERROR if successful, error-code otherwise.
 */
tegrabl_error_t tegrabl_enable_soc_therm(void);

/**
* @brief reset and boot the board in recovery mode
*/
void tegrabl_boot_recovery_mode(void);

/**
* @brief suspend uphy
*/
tegrabl_error_t tegrabl_uphy_suspend(void);

/**
* @brief reset the board
*/
void tegrabl_pmc_reset(void);

/** @brief set pmc scratch0 for fastboot, recovery and forced recovery usecases
 *
 *  @param flag specifies type of scratch0_flag
 *  @set flag will be set if true else reseted to zero
 *
 *  @return TEGRABL_NO_ERROR if successful.
 */
tegrabl_error_t tegrabl_set_pmc_scratch0_flag(
		tegrabl_scratch0_flag_t flag, bool set);

/**
 * @brief Clear scratch registers SECURE_RSV2_0 - SECURE_RSV7_1
 */
void tegrabl_clear_pmc_rsvd(void);

/** @brief get pmc scratch0 for fastboot, recovery and forced recovery usecases
 *
 *  @param flag specifies type of scratch0_flag
 *  @is_set get whether flag is set or not
 *
 *  @return TEGRABL_NO_ERROR if successful.
 */
tegrabl_error_t tegrabl_get_pmc_scratch0_flag(
		tegrabl_scratch0_flag_t flag, bool *is_set);

/** @brief set soc core voltage to given milli volts
 *
 * @param soc_mv soc core voltage to be set in milli volts
 *
 * @return TEGRABL_NO_ERROR if successful, specific error code
 *         in case of failure
 */
tegrabl_error_t tegrabl_set_soc_core_voltage(uint32_t soc_mv);

/**
 * @brief Retrieve the boot type based on contents of scratch
 * regiter.
 *
 * @return appropriate boot chain type based on scratch register value.
 */
tegrabl_boot_chain_type_t tegrabl_get_boot_chain_type(void);

/**
 * @brief Updates scratch register as per the boot chain type input.
 *
 * @param boot_chain Which chain of binaries to boot.
 */
void tegrabl_set_boot_chain_type(tegrabl_boot_chain_type_t boot_chain);

/**
 * @brief Resets the scratch register used for fallback mechanism.
 */
void tegrabl_reset_fallback_scratch(void);

/**
 * @brief Triggers recovery boot chain if current boot chain is primary or
 * vice-versa.
 *
 * @param bootchain_max_retries specifies the max count to retry a particular
 *        bootchain, before switching.
 */
void tegrabl_trigger_fallback_boot_chain(const uint32_t bootchain_max_retries);

/**
 * @brief get soc chip info from register
 *
 * @param info callee filled, chip info read
 */
void tegrabl_get_chip_info(struct tegrabl_chip_info *info);

/**
 * @brief Select kernel to be loaded based upon value in scratch register
 *
 * @return Kernel type to be loaded
 */
tegrabl_binary_type_t  tegrabl_get_kernel_type(void);

/**
 * @brief Check the uncorrectable error signature in the secure scratch register
 *
 * @return true if signature is DRAM ECC Uncorrectable error, false otherwise.
 */
bool tegrabl_is_scratch_dram_dbe_flag_set(void);

/**
 * @brief Get bad page number from SCRATCH_7 register
 *
 * @return bad page number
 */
uint32_t tegrabl_get_bad_dram_page_number(void);

/**
 * @brief Detect whether running on System-FPGA
 *
 * @return true on system-FPGA and false on other platforms
 */
bool tegrabl_is_fpga(void);

/**
 * @brief Detect whether running on VDK
 *
 * @return true on VDK and false on other platforms
 */
bool tegrabl_is_vdk(void);

/**
 * @brief Set A/B boot slot register
 *
 * @param slot_info to be set
 */
void tegrabl_set_boot_slot_reg(uint32_t slot_info);

/**
 * @brief get A/B boot slot register
 *
 * @return A/B slot register value
 */
uint32_t tegrabl_get_boot_slot_reg(void);

/**
 * @brief get chip ecid string
 *
 * @param ecid_str - pointer to buffer to save ecid string
 *        size - buffer size
 *
 * @return TEGRABL_NO_ERROR if success; error code otherwise
 */
tegrabl_error_t tegrabl_get_ecid_str(char *ecid_str, uint32_t size);

/**
 * @brief checks if the given slot has non zero key
 *
 * @param keyslot to be checked
 *
 * @return true if the key is non zero
*/
bool tegrabl_keyslot_check_if_key_is_nonzero(uint8_t keyslot);

#endif /* INCLUDED_TEGRABL_SOC_MISC_H */
