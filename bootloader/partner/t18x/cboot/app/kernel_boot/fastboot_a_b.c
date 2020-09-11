/*
 * Copyright (c) 2017-2018, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited
 */

#define MODULE TEGRABL_ERR_FASTBOOT

#include <tegrabl_debug.h>
#include <tegrabl_error.h>
#include <tegrabl_malloc.h>
#include <tegrabl_a_b_boot_control.h>
#include <tegrabl_partition_manager.h>
#include <string.h>

/* Define the partitions that must be updated all slots during OTA */
const char *always_ab_partition_list[] = {
	"mb1",
};

bool fastboot_a_b_is_always_ab_partition(const char *part)
{
	uint8_t i;
	for (i = 0; i < ARRAY_SIZE(always_ab_partition_list); i++) {
		if (!strcmp(part, always_ab_partition_list[i])) {
			return true;
		}
	}
	return false;
}

tegrabl_error_t fastboot_a_b_get_current_slot(char *current_slot)
{
	tegrabl_error_t err = TEGRABL_NO_ERROR;

	if (current_slot == NULL) {
		err = TEGRABL_ERROR(TEGRABL_ERR_INVALID, 0);
		goto done;
	}

	err = tegrabl_a_b_get_bootslot_suffix(current_slot + strlen(current_slot),
										  true);
	if (err != TEGRABL_NO_ERROR) {
		TEGRABL_SET_HIGHEST_MODULE(err);
		pr_info("get current slot failed\n");
		goto done;
	}

done:
	return err;
}

tegrabl_error_t fastboot_a_b_get_current_slot_id(uint32_t *slot_id)
{
	tegrabl_error_t err = TEGRABL_NO_ERROR;
	char suffix[BOOT_CHAIN_SUFFIX_LEN] = {0};

	TEGRABL_ASSERT(slot_id);

	err = fastboot_a_b_get_current_slot(suffix);
	if (err != TEGRABL_NO_ERROR) {
		goto done;
	}

	err = tegrabl_a_b_get_slot_via_suffix(suffix, slot_id);
	if (err != TEGRABL_NO_ERROR) {
		goto done;
	}

done:
	return err;
}

tegrabl_error_t fastboot_a_b_get_slot_suffix(uint8_t slot, char *slot_suffix)
{
	tegrabl_error_t error = TEGRABL_NO_ERROR;

	if (slot_suffix == NULL) {
		error = TEGRABL_ERROR(TEGRABL_ERR_INVALID, 0);
		goto done;
	}

	error = tegrabl_a_b_set_bootslot_suffix(slot, slot_suffix, true);
	if (error != TEGRABL_NO_ERROR) {
		TEGRABL_SET_HIGHEST_MODULE(error);
		goto done;
	}

done:
	return error;
}

tegrabl_error_t fastboot_a_b_get_slot_num(uint8_t *num)
{
	tegrabl_error_t error = TEGRABL_NO_ERROR;
	struct slot_meta_data *smd = NULL;

	TEGRABL_ASSERT(num);

	error = tegrabl_a_b_get_smd((void **)&smd);
	if (error != TEGRABL_NO_ERROR) {
		TEGRABL_SET_HIGHEST_MODULE(error);
		goto done;
	}

	error = tegrabl_a_b_get_slot_num((void *)smd, num);
	if (error != TEGRABL_NO_ERROR) {
		TEGRABL_SET_HIGHEST_MODULE(error);
		goto done;
	}
done:
	return TEGRABL_NO_ERROR;
}

tegrabl_error_t fastboot_a_b_is_slot_successful(const char *slot,
												bool *slot_successful)
{
	tegrabl_error_t error = TEGRABL_NO_ERROR;
	uint32_t slot_id;
	struct slot_meta_data *smd = NULL;
	uint8_t boot_successful;

	TEGRABL_ASSERT(slot);
	TEGRABL_ASSERT(slot_successful);

	error = tegrabl_a_b_get_slot_via_suffix(slot, &slot_id);
	if (error != TEGRABL_NO_ERROR) {
		TEGRABL_SET_HIGHEST_MODULE(error);
		goto done;
	}

	error = tegrabl_a_b_get_smd((void **)&smd);
	if (error != TEGRABL_NO_ERROR) {
		TEGRABL_SET_HIGHEST_MODULE(error);
		goto done;
	}

	error = tegrabl_a_b_get_successful((void *)smd, slot_id, &boot_successful);
	if (error != TEGRABL_NO_ERROR) {
		TEGRABL_SET_HIGHEST_MODULE(error);
		goto done;
	}

	*slot_successful = (boot_successful != 0) ? true : false;

done:
	return error;
}

tegrabl_error_t fastboot_a_b_is_slot_unbootable(const char *slot,
												bool *slot_unbootable)
{
	tegrabl_error_t error = TEGRABL_NO_ERROR;
	struct slot_meta_data *smd = NULL;
	uint32_t slot_id;

	TEGRABL_ASSERT(slot);
	TEGRABL_ASSERT(slot_successful);

	error = tegrabl_a_b_get_slot_via_suffix(slot, &slot_id);
	if (error != TEGRABL_NO_ERROR) {
		TEGRABL_SET_HIGHEST_MODULE(error);
		goto done;
	}

	error = tegrabl_a_b_get_smd((void **)&smd);
	if (error != TEGRABL_NO_ERROR) {
		TEGRABL_SET_HIGHEST_MODULE(error);
		goto done;
	}

	error = tegrabl_a_b_is_unbootable((void *)smd, slot_id, slot_unbootable);
	if (error != TEGRABL_NO_ERROR) {
		TEGRABL_SET_HIGHEST_MODULE(error);
		goto done;
	}

done:
	return error;
}

tegrabl_error_t fastboot_a_b_get_slot_retry_count(const char *slot,
												  uint8_t *count)
{
	tegrabl_error_t error = TEGRABL_NO_ERROR;
	struct slot_meta_data *smd = NULL;
	uint32_t slot_id;
	uint8_t retry_count;

	TEGRABL_ASSERT(slot);
	TEGRABL_ASSERT(slot_successful);

	error = tegrabl_a_b_get_slot_via_suffix(slot, &slot_id);
	if (error != TEGRABL_NO_ERROR) {
		TEGRABL_SET_HIGHEST_MODULE(error);
		goto done;
	}

	error = tegrabl_a_b_get_smd((void **)&smd);
	if (error != TEGRABL_NO_ERROR) {
		TEGRABL_SET_HIGHEST_MODULE(error);
		goto done;
	}

	error = tegrabl_a_b_get_retry_count((void *)smd, slot_id, &retry_count);
	if (error != TEGRABL_NO_ERROR) {
		TEGRABL_SET_HIGHEST_MODULE(error);
		goto done;
	}

	*count = retry_count;

done:
	return error;
}

tegrabl_error_t fastboot_a_b_set_slot_retry_count(uint32_t slot_id,
												  uint8_t retry_count)
{
	tegrabl_error_t error = TEGRABL_NO_ERROR;
	struct slot_meta_data *smd = NULL;

	error = tegrabl_a_b_get_smd((void **)&smd);
	if (error != TEGRABL_NO_ERROR) {
		TEGRABL_SET_HIGHEST_MODULE(error);
		goto done;
	}

	error = tegrabl_a_b_set_retry_count((void *)smd, slot_id, retry_count);
	if (error != TEGRABL_NO_ERROR) {
		TEGRABL_SET_HIGHEST_MODULE(error);
		goto done;
	}

	error = tegrabl_a_b_flush_smd((void *)smd);
	if (error != TEGRABL_NO_ERROR) {
		TEGRABL_SET_HIGHEST_MODULE(error);
		goto done;
	}

done:
	return error;
}

tegrabl_error_t fastboot_a_b_slot_set_active(const char *slot)
{
	tegrabl_error_t error = TEGRABL_NO_ERROR;
	struct slot_meta_data *smd = NULL;
	uint32_t slot_id;

	TEGRABL_ASSERT(slot);

	error = tegrabl_a_b_get_slot_via_suffix(slot, &slot_id);
	if (error != TEGRABL_NO_ERROR) {
		TEGRABL_SET_HIGHEST_MODULE(error);
		goto done;
	}

	error = tegrabl_a_b_get_smd((void **)&smd);
	if (error != TEGRABL_NO_ERROR) {
		TEGRABL_SET_HIGHEST_MODULE(error);
		goto done;
	}

	error = tegrabl_a_b_set_active_slot((void *)smd, slot_id);
	if (error != TEGRABL_NO_ERROR) {
		TEGRABL_SET_HIGHEST_MODULE(error);
		goto done;
	}

	error = tegrabl_a_b_flush_smd((void *)smd);
	if (error != TEGRABL_NO_ERROR) {
		TEGRABL_SET_HIGHEST_MODULE(error);
		goto done;
	}

	/* Set active slot in BOOT_CHAIN Scratch register */
	tegrabl_a_b_save_boot_slot_reg(smd, slot_id);

done:
	return error;
}

