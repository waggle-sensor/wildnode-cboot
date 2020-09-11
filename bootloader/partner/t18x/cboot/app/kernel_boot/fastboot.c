/*
 * Copyright (c) 2016-2018, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited
 */

/*
 * Copyright (c) 2009, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#define MODULE TEGRABL_ERR_FASTBOOT

#include <tegrabl_debug.h>
#include <tegrabl_error.h>
#include <tegrabl_partition_manager.h>
#include <tegrabl_fastboot_protocol.h>
#include <tegrabl_fastboot_partinfo.h>
#include <tegrabl_fastboot_oem.h>
#include <tegrabl_fastboot_a_b.h>
#include <tegrabl_transport_usbf.h>
#include <tegrabl_odmdata_soc.h>
#include <tegrabl_soc_misc.h>
#include <tegrabl_keyboard.h>
#include <tegrabl_timer.h>
#include <tegrabl_frp.h>
#include <tegrabl_se.h>
#include <tegrabl_fuse.h>
#include <tegrabl_brbct.h>
#include <tegrabl_bootloader_update.h>
#include <tegrabl_a_b_boot_control.h>
#include <tegrabl_prevent_rollback.h>
#include <tegrabl_io.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <kernel/thread.h>
#include <boot.h>
#include <fastboot.h>
#include <fastboot_a_b.h>
#include <linux_load.h>
#include <arscratch.h>
#include <tegrabl_addressmap.h>
#include <fastboot_menu.h>
#include <nvboot_bct.h>

#define TEGRABL_RSA_LENGTH 2048
#define HASH_SZ 32
#define FASTBOOT_LONG_PRESS_TIME_MS 3000
#define FASTBOOT_GPIO_NUM_SAMPLES 10

#define SCRATCH_READ(reg)			\
	NV_READ32(NV_ADDRESS_MAP_SCRATCH_BASE + SCRATCH_##reg)

static fastboot_usb_state_t
	cur_usb_state = FASTBOOT_USB_STATE_NOT_CONFIGURED;

static thread_t *fastboot_thread;
static fastboot_thread_status_t thread_state = TERMINATED;

#if defined(IS_T186)
static bool is_in_ota_progress(void)
{
	uint32_t reg;

	reg = tegrabl_get_boot_slot_reg();
	return (BOOT_CHAIN_REG_UPDATE_FLAG_GET(reg) == BC_FLAG_OTA_ON) ?
		   true : false;
}
#endif

static tegrabl_error_t is_fastboot_gpio_long_pressed(bool *is_long_press)
{
	uint32_t sampling_delay;
	uint32_t i;
	key_code_t key_code;
	key_event_t key_event;
	tegrabl_error_t ret = TEGRABL_NO_ERROR;

	ret = tegrabl_keyboard_init();
	if (ret != TEGRABL_NO_ERROR) {
		TEGRABL_SET_HIGHEST_MODULE(ret);
		return ret;
	}

	sampling_delay = FASTBOOT_LONG_PRESS_TIME_MS / FASTBOOT_GPIO_NUM_SAMPLES;

	for (i = 0; i < FASTBOOT_GPIO_NUM_SAMPLES; i++) {
		ret = tegrabl_keyboard_get_key_data(&key_code, &key_event);
		if (ret != TEGRABL_NO_ERROR) {
			TEGRABL_SET_HIGHEST_MODULE(ret);
			return ret;
		}
		/* if POWER key is not pressed or released, return false */
		if (key_code != KEY_ENTER && key_code != KEY_HOLD) {
			*is_long_press = false;
			return TEGRABL_NO_ERROR;
		}
		if (key_event != KEY_PRESS_FLAG) {
			*is_long_press = false;
			return TEGRABL_NO_ERROR;
		}
		tegrabl_mdelay(sampling_delay);
	}

	pr_info("Power button long press detected\n");
	*is_long_press = true;

	return TEGRABL_NO_ERROR;
}

tegrabl_error_t check_enter_fastboot(bool *out)
{
	/* This function has to be implemented based on platform requirements */

	bool pmc_fastboot_flag;
	tegrabl_error_t ret = TEGRABL_NO_ERROR;

#if defined(IS_T186)
	if (is_in_ota_progress()) {
		*out = false;
		goto done;
	}
#endif

	*out = false;
	/* check SCRATCH0 fastboot bit */
	ret = tegrabl_get_pmc_scratch0_flag(TEGRABL_PMC_SCRATCH0_FLAG_FASTBOOT,
				&pmc_fastboot_flag);
	if (ret != TEGRABL_NO_ERROR) {
		pr_info("Failed to read fastboot flag from PMC!\n");
		goto done;
	}

	if (pmc_fastboot_flag) {
		/* clear the fastboot flag */
		tegrabl_set_pmc_scratch0_flag(TEGRABL_PMC_SCRATCH0_FLAG_FASTBOOT,
			false);
		*out = true;
		goto done;
	}

	/* check fastboot gpio long press */
	ret = is_fastboot_gpio_long_pressed(out);

done:
	return ret;
}

fastboot_thread_status_t is_fastboot_running(void)
{
	return thread_state;
}

tegrabl_error_t fastboot_server(void *arg)
{
	tegrabl_error_t err = TEGRABL_NO_ERROR;
	struct usbf_priv_info usb_info;
	(void)arg;

	pr_debug("starting fastboot mode\n");

	tegrabl_fastboot_cmd_init();

	while (true) {
		usb_info.reopen = false;
		usb_info.usb_class = TEGRABL_USB_CLASS_FASTBOOT;
		err = tegrabl_transport_usbf_open(0, &usb_info);
		if (err != TEGRABL_NO_ERROR) {
			pr_critical("%s USB enumeration failed\n", __func__);
			goto fail;
		}
		pr_info("%s FASTBOOT enumeration success\n", __func__);

		err = tegrabl_fastboot_command_handler();
		if (err != TEGRABL_NO_ERROR) {
			pr_critical("%s FASTBOOT command handler failed!\n", __func__);
			goto fail;
		}

		if (cur_usb_state == FASTBOOT_USB_STATE_CABLE_NOT_CONNECTED) {
			dprintf(INFO, "Stopping usb driver\n");
			tegrabl_transport_usbf_close(0);
			dprintf(INFO, "Trying for re-enumeration\n");
			continue;
		}

		break;
	}
fail:
	thread_state = TERMINATED;
	return err;
}

/**
 * @brief Verify bootloader update payload signature used in
 *        fastboot flash bootloader <payload>
 *
 * @param bh payload/blob handle
 *
 * @return TEGRABL_NO_ERROR if success else return appropriate error code
 */
static tegrabl_error_t verify_bl_payload(tegrabl_blob_handle bh)
{
	tegrabl_error_t ret = TEGRABL_NO_ERROR;
	uint8_t is_signed;
	uint32_t blb_length;
	uint8_t *blb_addr = NULL;
	uint8_t *bsign;
	uint32_t bsign_size = 0;

	is_signed = tegrabl_blob_is_signed(bh);
	if (!is_signed)
		goto end;

	ret = tegrabl_blob_get_details(bh, &blb_addr, &blb_length);
	if (ret != TEGRABL_NO_ERROR)
		goto end;

	ret = tegrabl_blob_get_signature(bh, &bsign_size, &bsign);
	if (ret != TEGRABL_NO_ERROR)
		goto end;

	/* Verify blob signature read from blob and verify */
	ret = tegrabl_se_hash_and_rsa_pss_verify(0, TEGRABL_RSA_LENGTH,
											 (uint32_t *)bsign,
											 SE_SHAMODE_SHA256, HASH_SZ,
											 (uint32_t *)blb_addr,
											 blb_length);
	if (ret != TEGRABL_NO_ERROR)
		goto end;

end:
	return ret;
}

#if defined(CONFIG_ENABLE_A_B_SLOT)
static tegrabl_error_t is_ratchet_update_required(void *blob, bool *is_required)
{
	tegrabl_error_t err = TEGRABL_NO_ERROR;
	struct ratchet_info *ratchet = NULL;
	struct blob_header *bh = NULL;
	uint32_t rb_ratchet = 0;
	uint32_t mb1_ratchet = 0;
	uint32_t mts_ratchet = 0;
	struct tegrabl_rollback *rb = NULL;

	TEGRABL_ASSERT(blob);
	TEGRABL_ASSERT(is_required);

	bh = (struct blob_header *)blob;
	ratchet = &(bh->accessory.ratchet_info);

	rb = tegrabl_get_rollback_data();
	if (rb != NULL) {
		err = tegrabl_fuse_read(FUSE_RESERVED_ODM0 + rb->fuse_idx, &rb_ratchet,
								sizeof(uint32_t));
		if (err != TEGRABL_NO_ERROR) {
			return err;
		}
		rb_ratchet = tegrabl_rollback_fusevalue_to_level(rb_ratchet);
	}

	/* mb1 ratchet level is stored in SECURE_RSV54_SCRATCH_0 */
	mb1_ratchet = SCRATCH_READ(SECURE_RSV54_SCRATCH_0);

	/* Get mts ratchet from register SECURE_RSV52_SCRATCH_1 */
	mts_ratchet = SCRATCH_READ(SECURE_RSV52_SCRATCH_1);

	*is_required = ((ratchet->rollback_ratchet_level > rb_ratchet) ||
					(ratchet->mb1_ratchet_level > mb1_ratchet) ||
					(ratchet->mts_ratchet_level > mts_ratchet)) ? true : false;

	return TEGRABL_NO_ERROR;
}
#endif

static tegrabl_error_t flush_brbct_to_storage(uintptr_t new_bct, uint32_t size)
{
	tegrabl_error_t err = TEGRABL_NO_ERROR;
	struct tegrabl_partition part;
	uint64_t part_size = 0;
	uint64_t bct_size = 0;

	err = tegrabl_brbct_update_customer_data(new_bct, size);
	if (err != TEGRABL_NO_ERROR) {
		goto fail;
	}

	err = tegrabl_partition_open("BCT", &part);
	if (err != TEGRABL_NO_ERROR) {
		goto fail;
	}

	bct_size = sizeof(NvBootConfigTable);
	err = tegrabl_brbct_write_multiple((void *)new_bct, (void *)&part,
									   part_size, bct_size, bct_size);
	if (err != TEGRABL_NO_ERROR) {
		goto fail;
	}

	pr_info("BRBCT write successfully to storage\n");

fail:
	tegrabl_partition_close(&part);
	return err;
}

tegrabl_error_t fastboot_init(void)
{
	tegrabl_error_t err = TEGRABL_NO_ERROR;

#if defined(IS_T186)
	struct tegrabl_bl_update_callbacks cbs;
#endif

	(void)confirm_lock_menu;
	(void)confirm_unlock_menu;

	/* Initialize the fastboot framework */
	err = tegrabl_fastboot_set_callbacks();
	if (err != TEGRABL_NO_ERROR)
		return err;

#if defined(IS_T186)
	cbs.update_bct = flush_brbct_to_storage;
	cbs.verify_payload = verify_bl_payload;
	cbs.get_slot_num = fastboot_a_b_get_slot_num;
	cbs.get_slot_via_suffix = tegrabl_a_b_get_slot_via_suffix;
	cbs.get_slot_suffix = tegrabl_a_b_set_bootslot_suffix;
	cbs.is_ratchet_update_required = is_ratchet_update_required;
	cbs.is_always_ab_partition = fastboot_a_b_is_always_ab_partition;
	tegrabl_bl_update_set_callbacks(&cbs);
#endif

	fastboot_thread = thread_create("fastboot-server-thread",
									(thread_start_routine)fastboot_server,
									NULL, DEFAULT_PRIORITY,
									DEFAULT_STACK_SIZE);

	if (fastboot_thread == NULL) {
		pr_error("FAILED fastboot initialization\n");
		err = TEGRABL_ERROR(TEGRABL_ERR_INIT_FAILED, 0);
		return err;
	}

	thread_state = RUNNING;
	thread_resume(fastboot_thread);

	/* TODO
	 * Retrieve fastboot header text from DTB */
	menu_init();
	err = menu_draw(&fastboot_menu);

	/* in case fastbooot is already terminated by command line */
	if (is_fastboot_running() == TERMINATED) {
		goto exit_menu;
	}

	/* Display not init error is not critical to fastboot, need to quit
	 * fastboot menu but keep the fastboot cmd thread */
	if (err != TEGRABL_NO_ERROR &&
		is_fastboot_running() != TERMINATED) {
		pr_info("Fastboot menu unexpected failure, exit fastboot menu..\n");
		goto exit_menu;
	}

	err = menu_handle_on_select(&fastboot_menu);
	if (err != TEGRABL_NO_ERROR) {
		TEGRABL_SET_HIGHEST_MODULE(err);
		return err;
	}

exit_menu:
	thread_join(fastboot_thread, (int *)&err, INFINITE_TIME);

	thread_state = TERMINATED;

	if (err != TEGRABL_NO_ERROR)
		pr_critical("Error %d in fastboot thread....exiting fastboot mode\n",
					err);

	return err;
}

static tegrabl_error_t change_device_state(fastboot_lockcmd_t cmd)
{
	tegrabl_error_t err = TEGRABL_NO_ERROR;
	struct menu *confirm_menu;
	bool frp_enabled;

	if (cmd == FASTBOOT_LOCK) {
		if (tegrabl_odmdata_is_device_unlocked()) {
			confirm_menu = &confirm_lock_menu;
		} else {
			fastboot_ack("INFO", "Bootloader is already locked.");
			return TEGRABL_NO_ERROR;
		}
	} else { /* cmd == FASTBOOT_UNLOCK */
		if (!tegrabl_odmdata_is_device_unlocked()) {
			err = tegrabl_is_frp_enabled("fac_rst_protection", &frp_enabled);
			if (err != TEGRABL_NO_ERROR) {
				pr_error("Failed to get factory reset protection state\n");
				fastboot_fail("Device cannot be unlocked");
				return err;
			}
			if (frp_enabled) {
				pr_warn("FRP is enabled, unlock device is not allowed\n");
				fastboot_fail("Device cannot be unlocked");
#if defined(IS_T186)
				tegrabl_display_text_set_cursor(CURSOR_END);
				tegrabl_display_printf(RED, "Device cannot be unlocked");
#endif
				return TEGRABL_ERROR(TEGRABL_ERR_INVALID_STATE, 0);
			}
			confirm_menu = &confirm_unlock_menu;
		} else {
			fastboot_ack("INFO", "Bootloader is already unlocked.");
			return TEGRABL_NO_ERROR;
		}
	}

	thread_state = PAUSED;

	menu_unlock();
	err = menu_draw(confirm_menu);
	menu_lock();

	if (err != TEGRABL_NO_ERROR) {
		if (cmd == FASTBOOT_LOCK)
			fastboot_fail("Could not confirm lock from user");
		else
			fastboot_fail("Could not confirm unlock from user");
		thread_state = RUNNING;
		return err;
	}

	err = menu_handle_on_select(confirm_menu);
	if (err != TEGRABL_NO_ERROR) {
		if (cmd == FASTBOOT_LOCK)
			fastboot_fail("Failed to lock bootloader");
		else
			fastboot_fail("Failed to unlock bootloader");
		thread_state = RUNNING;
		return err;
	}

	thread_state = RUNNING;
	return TEGRABL_NO_ERROR;
}

static tegrabl_error_t fastboot_erase_partition(const char *partition_name,
												bool secure)
{
	const struct tegrabl_fastboot_partition_info *partinfo = NULL;
	struct tegrabl_partition partition;
	tegrabl_error_t error = TEGRABL_NO_ERROR;
	char ack_info[64];

	partinfo = tegrabl_fastboot_get_partinfo(partition_name);
	if (!partinfo)
		return TEGRABL_ERROR(TEGRABL_ERR_INVALID, 0);

	error = tegrabl_partition_open(partinfo->tegra_part_name, &partition);
	if (error) {
		/* if partition is not present, skip and return no error */
		if (TEGRABL_ERROR_REASON(error) == TEGRABL_ERR_NOT_FOUND)
			return TEGRABL_NO_ERROR;
		return error;
	}

	snprintf(ack_info, sizeof(ack_info), "erasing %s...", partition_name);
	fastboot_ack("INFO", ack_info);
	pr_info("%s\n", ack_info);

	error = tegrabl_partition_erase(&partition, secure);
	if (error) {
		snprintf(ack_info, sizeof(ack_info), "Partition %s erase failed!",
				partition_name);
		fastboot_ack("INFO", ack_info);
		pr_info("%s\n", ack_info);
		return error;
	}

	snprintf(ack_info, sizeof(ack_info), "erasing %s done", partition_name);
	fastboot_ack("INFO", ack_info);
	pr_info("%s\n", ack_info);

	return TEGRABL_NO_ERROR;
}

static tegrabl_error_t fastboot_erase_data(char *response)
{
	tegrabl_error_t error = TEGRABL_NO_ERROR;

	error = fastboot_erase_partition("userdata", true);
	if (error != TEGRABL_NO_ERROR)
		return error;

	/* cache partition is not present from Android N and later */
	error = fastboot_erase_partition("cache", true);
	if (error != TEGRABL_NO_ERROR)
		return error;

	return TEGRABL_NO_ERROR;
}

tegrabl_error_t fastboot_unlock_bootloader(void)
{
	tegrabl_error_t error;
	uint32_t odm_data = tegrabl_odmdata_get();
	char response[MAX_RESPONSE_SIZE];

	error = fastboot_erase_data(response);
	if (error)
		return error;

	fastboot_ack("INFO", "Unlocking bootloader...");
	tegrabl_odmdata_set(odm_data & (~(1 << TEGRA_BOOTLOADER_LOCK_BIT)), true);
	fastboot_ack("INFO", "Bootloader is now unlocked.");

	return TEGRABL_NO_ERROR;
}

tegrabl_error_t fastboot_lock_bootloader(void)
{
	tegrabl_error_t error;
	uint32_t odm_data = tegrabl_odmdata_get();
	char response[MAX_RESPONSE_SIZE];

	error = fastboot_erase_data(response);
	if (error)
		return error;

	fastboot_ack("INFO", "Locking bootloader...");
	tegrabl_odmdata_set(odm_data | (1 << TEGRA_BOOTLOADER_LOCK_BIT), true);
	fastboot_ack("INFO", "Bootloader is now locked.");

	return TEGRABL_NO_ERROR;
}

tegrabl_error_t tegrabl_fastboot_set_callbacks(void)
{
	struct tegrabl_fastboot_oem_ops *oem_ops;
	struct tegrabl_fastboot_a_b_ops *a_b_ops;

	oem_ops = tegrabl_fastboot_get_oem_ops();
	if (oem_ops == NULL) {
		return TEGRABL_ERROR(TEGRABL_ERR_NOT_INITIALIZED, 0);
	}

	a_b_ops = tegrabl_fastboot_get_a_b_ops();
	if (a_b_ops == NULL) {
		return TEGRABL_ERROR(TEGRABL_ERR_NOT_INITIALIZED, 0);
	}

	/* Register cboot fastboot APIs to the fastboot framework in common repo */
	oem_ops->change_device_state = change_device_state;
	oem_ops->is_device_unlocked = tegrabl_odmdata_is_device_unlocked;
	oem_ops->get_fuse_ecid = tegrabl_get_ecid_str;

#if defined(IS_T186)
	a_b_ops->get_current_slot = fastboot_a_b_get_current_slot;
	a_b_ops->get_slot_num = fastboot_a_b_get_slot_num;
	a_b_ops->get_slot_suffix = fastboot_a_b_get_slot_suffix;
	a_b_ops->is_slot_successful = fastboot_a_b_is_slot_successful;
	a_b_ops->is_slot_unbootable = fastboot_a_b_is_slot_unbootable;
	a_b_ops->get_slot_retry_count = fastboot_a_b_get_slot_retry_count;
	a_b_ops->slot_set_active = fastboot_a_b_slot_set_active;
#endif

	return TEGRABL_NO_ERROR;
}
