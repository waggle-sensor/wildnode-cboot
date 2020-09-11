/*
 * Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited
 */

#define MODULE TEGRABL_ERR_EXIT

#include <tegrabl_error.h>
#include <tegrabl_exit.h>
#include <tegrabl_soc_misc.h>
#include <tegrabl_psci.h>

#if defined(CONFIG_ENABLE_DISPLAY)
#include <tegrabl_display.h>
#endif

static tegrabl_error_t power_off(void *param)
{
	TEGRABL_UNUSED(param);
#if defined(CONFIG_ENABLE_DISPLAY)
	tegrabl_display_shutdown();
#endif
	tegrabl_psci_sys_off();
	/* Should not arrive here */
	return TEGRABL_ERROR(TEGRABL_ERR_COMMAND_FAILED, 0);
}

static tegrabl_error_t reset(void *param)
{
	TEGRABL_UNUSED(param);
#if defined(CONFIG_ENABLE_DISPLAY)
	tegrabl_display_shutdown();
#endif
	tegrabl_psci_sys_reset();
	/* Should not arrive here */
	return TEGRABL_ERROR(TEGRABL_ERR_COMMAND_FAILED, 0);
}

static tegrabl_error_t reboot_fastboot(void *param)
{
	/* set fastboot bit */
	tegrabl_set_pmc_scratch0_flag(TEGRABL_PMC_SCRATCH0_FLAG_FASTBOOT, true);
	reset(NULL);
	/* Should not arrive here */
	return TEGRABL_ERROR(TEGRABL_ERR_COMMAND_FAILED, 0);
}

static tegrabl_error_t reboot_forced_recovery(void *param)
{
	/* set recovery bit */
	tegrabl_set_pmc_scratch0_flag(TEGRABL_PMC_SCRATCH0_FLAG_FORCED_RECOVERY,
								  true);
	reset(NULL);
	/* Should not arrive here */
	return TEGRABL_ERROR(TEGRABL_ERR_COMMAND_FAILED, 0);
}

tegrabl_error_t tegrabl_exit_register(void)
{
	struct tegrabl_exit_ops *ops;

	ops = tegrabl_exit_get_ops();
	if (!ops)
		return TEGRABL_ERROR(TEGRABL_ERR_INVALID, 0);

	/* Register cboot exit APIs to the exit framework in common repo */
	ops->sys_power_off = power_off;
	ops->sys_reset = reset;
	ops->sys_reboot_fastboot = reboot_fastboot;
	ops->sys_reboot_forced_recovery = reboot_forced_recovery;

	return TEGRABL_NO_ERROR;
}

