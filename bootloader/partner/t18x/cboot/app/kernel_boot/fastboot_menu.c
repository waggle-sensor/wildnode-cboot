/*
 * Copyright (c) 2016-2018, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited
 */

#define MODULE TEGRABL_ERR_MENU

#include <tegrabl_error.h>
#include <tegrabl_soc_misc.h>
#include <tegrabl_exit.h>
#include <menu.h>
#include <linux_load.h>
#include <boot.h>
#include <fastboot.h>

static tegrabl_error_t fastboot_menu_continue(void *arg)
{
	tegrabl_error_t ret = TEGRABL_NO_ERROR;
	(void)arg;
	struct tegrabl_kernel_bin kernel;
	kernel.bin_type = tegrabl_get_kernel_type();

	ret = load_and_boot_kernel(&kernel);
	if (ret != TEGRABL_NO_ERROR)
		TEGRABL_SET_HIGHEST_MODULE(ret);
	/* Control should not reach here */
	return ret;
}

static tegrabl_error_t fastboot_menu_recovery(void *arg)
{
	tegrabl_error_t ret = TEGRABL_NO_ERROR;
	(void)arg;
	struct tegrabl_kernel_bin kernel;
	kernel.bin_type = TEGRABL_BINARY_RECOVERY_KERNEL;

	ret = load_and_boot_kernel(&kernel);
	if (ret != TEGRABL_NO_ERROR)
		TEGRABL_SET_HIGHEST_MODULE(ret);
	/* Control should not reach here */
	return ret;
}

static tegrabl_error_t fastboot_menu_reset(void *arg)
{
	tegrabl_error_t ret = TEGRABL_NO_ERROR;
	(void)arg;
	ret = tegrabl_reset();
	/* Control should not reach here */
	return ret;
}

static tegrabl_error_t fastboot_menu_poweroff(void *arg)
{
	tegrabl_error_t ret = TEGRABL_NO_ERROR;
	(void)arg;
	ret = tegrabl_poweroff();
	/* Control should not reach here */
	return ret;
}

static tegrabl_error_t fastboot_menu_forced_recovery(void *arg)
{
	tegrabl_error_t ret = TEGRABL_NO_ERROR;
	(void)arg;
	ret = tegrabl_reboot_forced_recovery();
	/* Control should not reach here */
	return ret;
}

static struct menu_entry fastboot_menu_entries[] = {
	{ /* Entry 1 */
		.ms_entry = {
#if defined(IS_T186)
			.color = WHITE,
#endif
			.data = "Continue\n"
		},
		.ms_on_select = {
#if defined(IS_T186)
			.color = GREEN,
#endif
			.data = "Continue booting ...\n"
		},
		.on_select_callback = fastboot_menu_continue,
		.arg = NULL
	},
	{ /* Entry 2 */
		.ms_entry = {
#if defined(IS_T186)
			.color = WHITE,
#endif
			.data = "Boot recovery kernel\n"
		},
		.ms_on_select = {
#if defined(IS_T186)
			.color = RED,
#endif
			.data = "Booting recovery kernel ...\n\n"
		},
		.on_select_callback = fastboot_menu_recovery,
		.arg = NULL
	},
	{ /* Entry 3 */
		.ms_entry = {
#if defined(IS_T186)
			.color = WHITE,
#endif
			.data = "Reboot\n"
		},
		.ms_on_select = {
#if defined(IS_T186)
			.color = GREEN,
#endif
			.data = "Rebooting ...!!!\n\n"
		},
		.on_select_callback = fastboot_menu_reset,
		.arg = NULL
	},
	{ /* Entry 4 */
		.ms_entry = {
#if defined(IS_T186)
			.color = WHITE,
#endif
			.data = "Poweroff\n"
		},
		.ms_on_select = {
#if defined(IS_T186)
			.color = GREEN,
#endif
			.data = "Powering off ...!!!\n\n"
		},
		.on_select_callback = fastboot_menu_poweroff,
		.arg = NULL
	},
	{ /* Entry 5 */
		.ms_entry = {
#if defined(IS_T186)
			.color = WHITE,
#endif
			.data = "Forced Recovery\n"
		},
		.ms_on_select = {
#if defined(IS_T186)
			.color = GREEN,
#endif
			.data = "Booting into usb recovery mode ...\n\n"
		},
		.on_select_callback = fastboot_menu_forced_recovery,
		.arg = NULL
	},
};

static tegrabl_error_t confirm_menu_lock_bootloader(void *arg)
{
	tegrabl_error_t error = TEGRABL_NO_ERROR;
	(void)arg;

	error = fastboot_lock_bootloader();
	return error;
}

static tegrabl_error_t confirm_menu_unlock_bootloader(void *arg)
{
	tegrabl_error_t error = TEGRABL_NO_ERROR;
	(void)arg;

	error = fastboot_unlock_bootloader();
	return error;
}

static struct menu_entry confirm_lock_menu_entries[] = {
	{ /* Entry 1 */
		.ms_entry = {
#if defined(IS_T186)
			.color = WHITE,
#endif
			.data = "Confirm\n"
		},
		.ms_on_select = {
#if defined(IS_T186)
			.color = GREEN,
#endif
			.data = "Confirmed. Locking the bootloader\n\n"
		},
		.on_select_callback = confirm_menu_lock_bootloader,
		.arg = NULL
	},
	{ /* Entry 2 */
		.ms_entry = {
#if defined(IS_T186)
			.color = WHITE,
#endif
			.data = "Back to menu\n",
		},
		.ms_on_select = {
#if defined(IS_T186)
			.color = RED,
#endif
			.data = "Okay, going back to menu ...\n\n"
		},
		.on_select_callback = NULL,
		.arg = NULL
	},
};

static struct menu_entry confirm_unlock_menu_entries[] = {
	{ /* Entry 1 */
		.ms_entry = {
#if defined(IS_T186)
			.color = WHITE,
#endif
			.data = "Confirm\n"
		},
		.ms_on_select = {
#if defined(IS_T186)
			.color = GREEN,
#endif
			.data = "Confirmed. Unlocking the bootloader\n\n"
		},
		.on_select_callback = confirm_menu_unlock_bootloader,
		.arg = NULL
	},
	{ /* Entry 2 */
		.ms_entry = {
#if defined(IS_T186)
			.color = WHITE,
#endif
			.data = "Back to menu\n",
		},
		.ms_on_select = {
#if defined(IS_T186)
			.color = RED,
#endif
			.data = "Okay, going back to menu ...\n\n"
		},
		.on_select_callback = NULL,
		.arg = NULL
	},
};

struct menu fastboot_menu = {
	.menu_type = FASTBOOT_MENU,
	.name = "fastboot-menu",
	.menu_entries = fastboot_menu_entries,
	.num_menu_entries = ARRAY_SIZE(fastboot_menu_entries),
#if defined(IS_T186)
	.menu_header = {
		.valid = true,
		.ms = {
			.color = RED,
			.data = "Fastboot menu:\n"
		}
	},
#endif
	.timeout = 0,
	.current_entry = 0,
};

struct menu confirm_lock_menu = {
	.menu_type = OTHER_MENU,
	.name = "confirm-lock-menu",
	.menu_entries = confirm_lock_menu_entries,
	.num_menu_entries = ARRAY_SIZE(confirm_lock_menu_entries),
#if defined(IS_T186)
	.menu_header = {
		.valid = true,
		.ms = {
			.color = RED,
			.data = " !!! READ THE FOLLOWING !!!\n"
					"If you lock the bootloader, all personal\n"
					"data from your device will be automatically\n"
					"deleted (a \"factory data reset\") to\n"
					"prevent unauthorized access to your\n"
					"personal data.\n\n\n"
		}
	},
#endif
	.timeout = 0,
	.current_entry = 0,
};

struct menu confirm_unlock_menu = {
	.menu_type = OTHER_MENU,
	.name = "confirm-unlock-menu",
	.menu_entries = confirm_unlock_menu_entries,
	.num_menu_entries = ARRAY_SIZE(confirm_unlock_menu_entries),
#if defined(IS_T186)
	.menu_header = {
		.valid = true,
		.ms = {
			.color = RED,
			.data = " !!! READ THE FOLLOWING !!!\n"
					"If you unlock the bootloader, you will\n"
					"be able to install custom operating system\n"
					"software on this device (but may void\n"
					"your warranty).\n"
					"A custom OS is not subject to the same\n"
					"testing as the original OS, and can cause\n"
					"your device and installed applications to\n"
					"stop working properly.\n"
					"Note,if you unlock the bootloader, all\n"
					"personal data from your device will be\n"
					"automatically deleted (a \"factory data\n"
					"reset\") to prevent unauthorized access to\n"
					"your personal data.\n\n\n"
		}
	},
#endif
	.timeout = 0,
	.current_entry = 0,
};

