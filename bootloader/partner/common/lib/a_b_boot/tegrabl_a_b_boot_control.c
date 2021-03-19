/*
 * Copyright (c) 2016-2019, NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#define MODULE TEGRABL_ERR_AB_BOOTCTRL

#include <string.h>
#include <tegrabl_error.h>
#include <tegrabl_debug.h>
#include <tegrabl_utils.h>
#include <stdbool.h>
#include <inttypes.h>
#include <tegrabl_a_b_boot_control.h>
#include <tegrabl_soc_misc.h>
#include <tegrabl_malloc.h>
#include <tegrabl_partition_manager.h>

typedef uint32_t smd_bin_copy_t;
#define SMD_COPY_PRIMARY 0U
#define SMD_COPY_SECONDARY 1U
#define MAX_SMD_COPY 2U

static void *smd_loadaddress;

#if defined(CONFIG_ENABLE_DEBUG)
static void boot_chain_dump_slot_info(void *load_address)
{
	uint32_t i = 0;
	struct slot_meta_data *smd_info = (struct slot_meta_data *)load_address;

	pr_error("magic:0x%x, version: %d, num_slots: %d\n",
			smd_info->magic,
			smd_info->version,
			smd_info->num_slots);

	for (i = 0; i < MAX_SLOTS; i++) {
		pr_trace("slot: %d, pri: %d, suffix: %c%c, retry: %d, boot_succ: %d\n",
				 i, smd_info->slot_info[i].priority,
				 smd_info->slot_info[i].suffix[0],
				 smd_info->slot_info[i].suffix[1],
				 smd_info->slot_info[i].retry_count,
				 smd_info->slot_info[i].boot_successful);
	}
}
#endif

uint16_t tegrabl_a_b_get_version(void *smd)
{
	struct slot_meta_data *smd_info = (struct slot_meta_data *)smd;
	pr_error("%s: start [smd: %p]\n", __func__, smd);

	TEGRABL_ASSERT(smd != NULL);

	pr_error("%s: end [smd_info->version: %d]\n", __func__, smd_info->version);
	return smd_info->version;
}

void tegrabl_a_b_init(void *smd)
{
	struct slot_meta_data *smd_info = (struct slot_meta_data *)smd;
	pr_error("%s: start [smd: %p]\n", __func__, smd);

	TEGRABL_ASSERT(smd != NULL);

	/* Set default: A: bootable, B: unbootable */
	smd_info->slot_info[0].priority = 15;
	smd_info->slot_info[0].retry_count = 7;
	smd_info->slot_info[0].boot_successful = 1;
	(void) memcpy(smd_info->slot_info[0].suffix, BOOT_CHAIN_SUFFIX_A,
			BOOT_CHAIN_SUFFIX_LEN);

	smd_info->slot_info[1].priority = 0;
	smd_info->slot_info[1].retry_count = 0;
	smd_info->slot_info[1].boot_successful = 0;
	(void) memcpy(smd_info->slot_info[1].suffix, BOOT_CHAIN_SUFFIX_B,
			BOOT_CHAIN_SUFFIX_LEN);

	smd_info->magic = BOOT_CHAIN_MAGIC;
	smd_info->version = BOOT_CHAIN_VERSION;

	/*
	 * Simulate SMD but set max_slots to 1 so that device is handled as
	 * non-A/B system.
	 */
	smd_info->num_slots = 1;
	pr_error("%s: end\n", __func__);
}

static inline uint16_t tegrabl_a_b_get_max_num_slots(void *smd)
{
	struct slot_meta_data *smd_info = (struct slot_meta_data *)smd;
	pr_error("%s: start [smd: %p]\n", __func__, smd);

	TEGRABL_ASSERT(smd != NULL);

	pr_error("%s: end [smd_info->num_slots: %u]\n", __func__, smd_info->num_slots);
	return smd_info->num_slots;
}

tegrabl_error_t tegrabl_a_b_init_boot_slot_reg(void *smd)
{
	tegrabl_error_t err = TEGRABL_NO_ERROR;
	uint32_t reg;
	uint32_t slot;
	pr_error("%s: start [smd: %p]\n", __func__, smd);

	reg = tegrabl_get_boot_slot_reg();

	/* In case not power on reset, scratch register may have been already
	 * initialized in previous boot, hence skip */
	if (BOOT_CHAIN_REG_MAGIC_GET(reg) == BOOT_CHAIN_REG_MAGIC) {
		pr_error("%s: boot chain match\n", __func__);
		goto done;
	}

	/* If reg magic is invalid, init reg with SMD and retain update flag */
	reg = 0;
	/* Set reg magic */
	reg = BOOT_CHAIN_REG_MAGIC_SET(reg);
	tegrabl_set_boot_slot_reg(reg);

	err = tegrabl_a_b_get_active_slot(smd, &slot);
	if (err != TEGRABL_NO_ERROR) {
		goto done;
	}

	tegrabl_a_b_save_boot_slot_reg(smd, slot);

done:
	pr_error("%s: end [err]\n", __func__);
	tegrabl_error_print_error(err);
	return err;
}

static bool tegrabl_a_b_is_valid(void *smd)
{
	uint32_t reg;
	struct slot_meta_data *smd_info;
	pr_error("%s: start [smd: %p]\n", __func__, smd);

	if (smd == NULL) {
		/* check info stored in scratch register */
		reg = tegrabl_get_boot_slot_reg();

		pr_error("%s: end 1 [bool: %u]\n", __func__, (bool)
				((BOOT_CHAIN_REG_MAGIC_GET(reg) == BOOT_CHAIN_REG_MAGIC) &&
				 (BOOT_CHAIN_REG_MAX_SLOTS_GET(reg) > 1U)));
		return (bool)
				((BOOT_CHAIN_REG_MAGIC_GET(reg) == BOOT_CHAIN_REG_MAGIC) &&
				 (BOOT_CHAIN_REG_MAX_SLOTS_GET(reg) > 1U));
	}

	/* check info from smd buffer */
	smd_info = (struct slot_meta_data *)smd;
	pr_error("%s: end 2 [bool: %u]\n", __func__, (bool)((smd_info->magic == BOOT_CHAIN_MAGIC) && (smd_info->num_slots > 1U)));
	return (bool)((smd_info->magic == BOOT_CHAIN_MAGIC) &&
				  (smd_info->num_slots > 1U));
}

static tegrabl_error_t boot_chain_get_bootslot_from_reg(uint32_t *active_slot)
{
	tegrabl_error_t err;
	uint32_t reg;
	pr_error("%s: start [active_slot: %u]\n", __func__, *active_slot);

	reg = tegrabl_get_boot_slot_reg();

	if (!tegrabl_a_b_is_valid(NULL)) {
		pr_error("No valid slot number is found in scratch register\n");
		pr_error("Return default slot: %s\n", BOOT_CHAIN_SUFFIX_A);
		*active_slot = BOOT_SLOT_A;
		err = TEGRABL_ERROR(TEGRABL_ERR_NOT_FOUND, 0);
		goto done;
	}

	*active_slot = BOOT_CHAIN_REG_SLOT_NUM_GET(reg);
	err = TEGRABL_NO_ERROR;

done:
	pr_error("%s: end [err | active_slot: %u]\n", __func__, *active_slot);
	tegrabl_error_print_error(err);
	return err;
}

static tegrabl_error_t boot_chain_get_bootslot_from_smd(void *smd,
							uint32_t *active_slot)
{
	tegrabl_error_t err = TEGRABL_NO_ERROR;
	struct slot_meta_data *bootctrl = (struct slot_meta_data *)smd;
	uint8_t max_priority;
	uint32_t i, slot;
	pr_error("%s: start [smd: %p | active_slot: %u]\n", __func__, smd, *active_slot);

	/* Check if the data is correct */
	if (bootctrl->magic != BOOT_CHAIN_MAGIC) {
		pr_error("SMD is corrupted!\n");
		err = TEGRABL_ERROR(TEGRABL_ERR_INVALID, 0);
		goto done;
	}

	/* Init priority as unbootable */
	max_priority = 0;

	/* Find slot with retry_count > 0 and highest priority */
	for (i = 0; i < bootctrl->num_slots; i++) {
		if ((bootctrl->slot_info[i].retry_count != 0U) &&
			(bootctrl->slot_info[i].priority != 0U)) {
			if (max_priority < bootctrl->slot_info[i].priority) {
				max_priority = bootctrl->slot_info[i].priority;
				slot = i;
			}
		}
	}

	/* Found a bootable slot? */
	if (max_priority == 0U) {
		pr_error("No bootable slot found\n");
		err = TEGRABL_ERROR(TEGRABL_ERR_INVALID, 1);
		goto done;
	}

	*active_slot = (uint32_t)slot;
	pr_trace("Active boot chain: %u\n", *active_slot);

	err = TEGRABL_NO_ERROR;

done:
	pr_error("%s: end [err | active_slot: %u]\n", __func__, *active_slot);
	tegrabl_error_print_error(err);
	return err;
}

tegrabl_error_t tegrabl_a_b_get_active_slot(void *smd, uint32_t *active_slot)
{
	tegrabl_error_t err;
	pr_error("%s: start [smd: %p | active_slot: %u]\n", __func__, smd, *active_slot);

	/* Use slot number previous saved in scratch register */
	if (smd == NULL) {
		pr_error("%s: use slot number in sratch register\n", __func__);
		err = boot_chain_get_bootslot_from_reg(active_slot);
		goto done;
	}

	/* Run a/b slot selection logic to find out active slot */
	err = boot_chain_get_bootslot_from_smd(smd, active_slot);

done:
	pr_error("%s: end [err | active_slot: %u]\n", __func__, *active_slot);
	tegrabl_error_print_error(err);
	return err;
}

void tegrabl_a_b_set_retry_count_reg(uint32_t slot, uint8_t retry_count)
{
	uint32_t reg;
	pr_error("%s: start [slot: %u | retry_count: %u]\n", __func__, slot, retry_count);

	reg = tegrabl_get_boot_slot_reg();
	switch (slot) {
	case BOOT_SLOT_A:
		pr_error("%s: boot slot A\n", __func__);
		reg = BOOT_CHAIN_REG_A_RETRY_COUNT_SET(retry_count, reg);
		break;
	case BOOT_SLOT_B:
		pr_error("%s: boot slot B\n", __func__);
		reg = BOOT_CHAIN_REG_B_RETRY_COUNT_SET(retry_count, reg);
		break;
	default:
		break;
	}
	tegrabl_set_boot_slot_reg(reg);
	pr_error("%s: end\n", __func__);
}

static uint8_t tegrabl_a_b_get_retry_count_reg(uint32_t slot, uint32_t reg)
{
	uint8_t retry_count = 0;
	pr_error("%s: start [slot: %u | reg: %u]\n", __func__, slot, reg);

	switch (slot) {
	case BOOT_SLOT_A:
		pr_error("%s: boot slot A\n", __func__);
		retry_count = (uint8_t)BOOT_CHAIN_REG_A_RETRY_COUNT_GET(reg);
		break;
	case BOOT_SLOT_B:
		pr_error("%s: boot slot B\n", __func__);
		retry_count = (uint8_t)BOOT_CHAIN_REG_B_RETRY_COUNT_GET(reg);
		break;
	default:
		TEGRABL_ASSERT(0);
		break;
	}
	pr_error("%s: end [retry_count: %u]\n", __func__, retry_count);
	return retry_count;
}

void tegrabl_a_b_copy_retry_count(void *smd, uint32_t *reg, uint32_t direct)
{
	struct slot_meta_data *bootctrl = (struct slot_meta_data *)smd;
	uint8_t retry_count;
	pr_error("%s: start [smd: %p | reg: %u | direct: %u]\n", __func__, smd, *reg, direct);

	TEGRABL_ASSERT(smd != NULL);

	switch (direct) {
	case FROM_REG_TO_SMD:
		pr_error("%s: REG -> SMD\n", __func__);
		retry_count = (uint8_t)BOOT_CHAIN_REG_A_RETRY_COUNT_GET(*reg);
		bootctrl->slot_info[BOOT_SLOT_A].retry_count = retry_count;
		retry_count = (uint8_t)BOOT_CHAIN_REG_B_RETRY_COUNT_GET(*reg);
		bootctrl->slot_info[BOOT_SLOT_B].retry_count = retry_count;
		break;
	case FROM_SMD_TO_REG:
		pr_error("%s: SMD -> REG\n", __func__);
		retry_count = bootctrl->slot_info[BOOT_SLOT_A].retry_count;
		*reg = BOOT_CHAIN_REG_A_RETRY_COUNT_SET(retry_count, *reg);
		retry_count = bootctrl->slot_info[BOOT_SLOT_B].retry_count;
		*reg = BOOT_CHAIN_REG_B_RETRY_COUNT_SET(retry_count, *reg);
		break;
	default:
		break;
	}
	pr_error("%s: end\n", __func__);
}

void tegrabl_a_b_save_boot_slot_reg(void *smd, uint32_t slot)
{
	uint32_t reg;
	struct slot_meta_data *bootctrl = (struct slot_meta_data *)smd;
	pr_error("%s: start [smd: %p | slot: %u]\n", __func__, smd, slot);

	TEGRABL_ASSERT(smd != NULL);

	reg = tegrabl_get_boot_slot_reg();

	/* Set slot number */
	reg = BOOT_CHAIN_REG_SLOT_NUM_SET(slot, reg);

	/* Set max slots */
	reg = BOOT_CHAIN_REG_MAX_SLOTS_SET(tegrabl_a_b_get_max_num_slots(smd), reg);

	/* Set retry counts */
	tegrabl_a_b_copy_retry_count(smd, &reg, FROM_SMD_TO_REG);

	/* Set update flag if current boot slot's boot_succ flag is 0 */
	if (bootctrl->slot_info[slot].boot_successful == 0U) {
		pr_error("%s: 1\n", __func__);
		reg = BOOT_CHAIN_REG_UPDATE_FLAG_SET(BC_FLAG_OTA_ON, reg);
	} else {
		pr_error("%s: 2\n", __func__);
		/* check SMD version and set REDUNDANCY flag if it is supported */
		if (BOOTCTRL_SUPPORT_REDUNDANCY(bootctrl->version) != 0U) {
			reg = BOOT_CHAIN_REG_UPDATE_FLAG_SET(BC_FLAG_REDUNDANCY_BOOT, reg);
		}
	}

	tegrabl_set_boot_slot_reg(reg);
	pr_error("%s: end\n", __func__);
}

tegrabl_error_t tegrabl_a_b_check_and_update_retry_count(void *smd,
														 uint32_t slot)
{
	struct slot_meta_data *bootctrl = (struct slot_meta_data *)smd;
	pr_error("%s: start [smd: %p | slot: %u]\n", __func__, smd, slot);

	TEGRABL_ASSERT(smd != NULL);

	/*
	 * Decrement retry count if
	 * a. REDUNDNACY is supported, or
	 * b. Current slot state is unsuccessful
	 */

	pr_error("%s: BOOTCTRL_SUPPORT_REDUNDANCY: [%lu]\n", __func__, (BOOTCTRL_SUPPORT_REDUNDANCY(bootctrl->version)));

	if ((BOOTCTRL_SUPPORT_REDUNDANCY(bootctrl->version) != 0U) ||
		(bootctrl->slot_info[slot].boot_successful == 0U)) {
		pr_error("%s: decrement retry count - before [%u]\n", __func__, bootctrl->slot_info[slot].retry_count);
		TEGRABL_ASSERT(bootctrl->slot_info[slot].retry_count != 0U);
		bootctrl->slot_info[slot].retry_count--;
		pr_error("%s: decrement retry count - after [%u]\n", __func__, bootctrl->slot_info[slot].retry_count);
	}

#if defined(CONFIG_ENABLE_DEBUG)
	boot_chain_dump_slot_info(bootctrl);
#endif

	pr_error("%s: end [TEGRABL_NO_ERROR]\n", __func__);
	return TEGRABL_NO_ERROR;
}

tegrabl_error_t tegrabl_a_b_get_bootslot_suffix(char *suffix, bool full_suffix)
{
	tegrabl_error_t err;
	uint32_t slot;
	pr_error("%s: start [suffix: %s | full_suffix: %u]\n", __func__, suffix, full_suffix);

	err = tegrabl_a_b_get_active_slot(NULL, &slot);
	if (err != TEGRABL_NO_ERROR) {
		if (TEGRABL_ERROR_REASON(err) == TEGRABL_ERR_NOT_FOUND) {
			err = TEGRABL_NO_ERROR;
		} else {
			err = TEGRABL_ERROR(TEGRABL_ERR_INVALID, 0);
			goto fail;
		}
	}

	if ((full_suffix == false) &&  (slot == BOOT_SLOT_A)) {
		*suffix = '\0';
		goto done;
	}

	if (slot == BOOT_SLOT_A) {
		strncpy(suffix, BOOT_CHAIN_SUFFIX_A, BOOT_CHAIN_SUFFIX_LEN);
	} else {
		strncpy(suffix, BOOT_CHAIN_SUFFIX_B, BOOT_CHAIN_SUFFIX_LEN);
	}
	*(suffix + BOOT_CHAIN_SUFFIX_LEN) = '\0';

done:
	pr_error("Active slot suffix: %s\n", suffix);

fail:
	pr_error("%s: end [err]\n", __func__);
	tegrabl_error_print_error(err);
	return err;
}

tegrabl_error_t tegrabl_a_b_set_bootslot_suffix(uint32_t slot, char *partition,
												bool full_suffix)
{
	const char *suffix;
	pr_error("%s: start [slot: %u | partition: %s | full_suffix %u]\n", __func__, slot, partition, full_suffix);

	/*
	 * To be compatible with legacy partition names, set slot A's suffix to
	 * none, ie, no suffix for slot A, if full_suffix is not needed
	 */
	if ((full_suffix == false) && (slot == BOOT_SLOT_A)) {
		goto done;
	}

	if (slot == BOOT_SLOT_A) {
		suffix = BOOT_CHAIN_SUFFIX_A;
	} else {
		suffix = BOOT_CHAIN_SUFFIX_B;
	}
	strcat(partition, suffix);

done:
	pr_debug("Select partition: %s\n", partition);
	pr_error("%s: end [TEGRABL_NO_ERROR]\n", __func__);
	return TEGRABL_NO_ERROR;
}

tegrabl_error_t tegrabl_a_b_get_slot_via_suffix(const char *suffix,
												uint32_t *boot_slot_id)
{
	tegrabl_error_t error = TEGRABL_NO_ERROR;
	pr_error("%s: start [suffix: %s | boot_slot_id: %u]\n", __func__, suffix, *boot_slot_id);

	if ((*suffix == '\0') || (strcmp(suffix, BOOT_CHAIN_SUFFIX_A) == 0)) {
		*boot_slot_id = (uint32_t)BOOT_SLOT_A;
	} else if (strcmp(suffix, BOOT_CHAIN_SUFFIX_B) == 0) {
		*boot_slot_id = (uint32_t)BOOT_SLOT_B;
	} else {
		error = TEGRABL_ERROR(TEGRABL_ERR_INVALID, 0);
	}

	pr_error("%s: end [error | boot_slot_id: %u]\n", __func__, *boot_slot_id);
	tegrabl_error_print_error(error);
	return error;
}

tegrabl_error_t tegrabl_a_b_get_slot_num(void *smd_addr, uint8_t *num_slots)
{
	struct slot_meta_data *smd;
	pr_error("%s: start [smd: %p | num_slots: %u]\n", __func__, smd_addr, *num_slots);

	if ((smd_addr == NULL) || (num_slots == NULL)) {
		pr_error("%s: end 1 [TEGRABL_ERR_INVALID]\n", __func__);
		return TEGRABL_ERROR(TEGRABL_ERR_INVALID, 0);
	}

	smd = (struct slot_meta_data *)smd_addr;
	*num_slots = (uint8_t)smd->num_slots;

	pr_error("%s: end 2 [TEGRABL_NO_ERROR | num_slots: %u]\n", __func__, *num_slots);
	return TEGRABL_NO_ERROR;
}

tegrabl_error_t tegrabl_a_b_get_successful(void *smd_addr, uint32_t slot,
										   uint8_t *successful)
{
	struct slot_meta_data *smd;
	pr_error("%s: start [smd: %p | slot: %u | successful: %u]\n", __func__, smd_addr, slot, *successful);

	if ((smd_addr == NULL) || (slot >= (uint32_t)MAX_SLOTS) || (successful == NULL)) {
		pr_error("%s: end 1 [TEGRABL_ERR_INVALID]\n", __func__);
		return TEGRABL_ERROR(TEGRABL_ERR_INVALID, 0);
	}

	smd = (struct slot_meta_data *)smd_addr;
	*successful = smd->slot_info[slot].boot_successful;

	pr_error("%s: end 2 [TEGRABL_NO_ERROR | successful: %u]\n", __func__, *successful);
	return TEGRABL_NO_ERROR;
}

tegrabl_error_t tegrabl_a_b_get_retry_count(void *smd_addr, uint32_t slot,
											uint8_t *retry_count)
{
	struct slot_meta_data *smd;
	pr_error("%s: start [smd: %p | slot: %u | retry_count: %u]\n", __func__, smd_addr, slot, *retry_count);

	if ((smd_addr == NULL) || (slot >= (uint32_t)MAX_SLOTS) || (retry_count == NULL)) {
		pr_error("%s: end 1 [TEGRABL_ERR_INVALID]\n", __func__);
		return TEGRABL_ERROR(TEGRABL_ERR_INVALID, 0);
	}

	smd = (struct slot_meta_data *)smd_addr;
	*retry_count = smd->slot_info[slot].retry_count;

	pr_error("%s: end 2 [TEGRABL_NO_ERROR | retry_count: %u]\n", __func__, *retry_count);
	return TEGRABL_NO_ERROR;
}

tegrabl_error_t tegrabl_a_b_get_priority(void *smd_addr, uint32_t slot,
										 uint8_t *priority)
{
	struct slot_meta_data *smd;
	pr_error("%s: start [smd: %p | slot: %u | priority: %u]\n", __func__, smd_addr, slot, *priority);

	if ((smd_addr == NULL) || (slot >= MAX_SLOTS) || (priority == NULL)) {
		return TEGRABL_ERROR(TEGRABL_ERR_INVALID, 0);
	}

	smd = (struct slot_meta_data *)smd_addr;
	*priority = smd->slot_info[slot].priority;

	pr_error("%s: end [TEGRABL_NO_ERROR | priority: %u]\n", __func__, *priority);
	return TEGRABL_NO_ERROR;
}

tegrabl_error_t tegrabl_a_b_is_unbootable(void *smd_addr, uint32_t slot,
										  bool *is_unbootable)
{
	tegrabl_error_t error = TEGRABL_NO_ERROR;
	struct slot_meta_data *smd;
	uint8_t priority, retry_count;
	pr_error("%s: start [smd: %p | slot: %u | is_unbootable: %u]\n", __func__, smd_addr, slot, *is_unbootable);

	if ((smd_addr == NULL) || (slot >= MAX_SLOTS) || (is_unbootable == NULL)) {
		error = TEGRABL_ERROR(TEGRABL_ERR_INVALID, 0);
		goto done;
	}

	smd = (struct slot_meta_data *)smd_addr;

	error = tegrabl_a_b_get_priority((void *)smd, slot, &priority);
	if (error != TEGRABL_NO_ERROR) {
		pr_error("%s: tegrabl_a_b_get_priority error\n", __func__);
		TEGRABL_SET_HIGHEST_MODULE(error);
		goto done;
	}

	error = tegrabl_a_b_get_retry_count((void *)smd, slot, &retry_count);
	if (error != TEGRABL_NO_ERROR) {
		pr_error("%s: tegrabl_a_b_get_retry_count error\n", __func__);
		TEGRABL_SET_HIGHEST_MODULE(error);
		goto done;
	}

	*is_unbootable = ((priority == 0U) || (retry_count == 0U)) ? true : false;

done:
	pr_error("%s: end [error | is_unbootable: %u]\n", __func__, *is_unbootable);
	tegrabl_error_print_error(error);
	return error;
}

tegrabl_error_t tegrabl_a_b_set_successful(void *smd_addr, uint32_t slot,
										   uint8_t successful)
{
	struct slot_meta_data *smd;
	pr_error("%s: start [smd: %p | slot: %u | successful: %u]\n", __func__, smd_addr, slot, successful);

	if ((smd_addr == NULL) || (slot >= MAX_SLOTS)) {
		pr_error("%s: end 1 [TEGRABL_ERR_INVALID]\n", __func__);
		return TEGRABL_ERROR(TEGRABL_ERR_INVALID, 0);
	}

	smd = (struct slot_meta_data *)smd_addr;
	smd->slot_info[slot].boot_successful = successful;

	pr_error("%s: end 2 [TEGRABL_NO_ERROR | smd->slot_info[slot].boot_successful: %u]\n", __func__, smd->slot_info[slot].boot_successful);
	return TEGRABL_NO_ERROR;
}

tegrabl_error_t tegrabl_a_b_set_retry_count(void *smd_addr, uint32_t slot,
											uint8_t retry_count)
{
	struct slot_meta_data *smd;
	pr_error("%s: start [smd: %p | slot: %u | retry_count: %u]\n", __func__, smd_addr, slot, retry_count);

	if ((smd_addr == NULL) || (slot >= MAX_SLOTS)) {
		pr_error("%s: end 1 [TEGRABL_ERR_INVALID]\n", __func__);
		return TEGRABL_ERROR(TEGRABL_ERR_INVALID, 0);
	}

	smd = (struct slot_meta_data *)smd_addr;
	smd->slot_info[slot].retry_count = retry_count;

	pr_error("%s: end 2 [TEGRABL_NO_ERROR | smd->slot_info[slot].retry_count: %u]\n", __func__, smd->slot_info[slot].retry_count);
	return TEGRABL_NO_ERROR;
}

tegrabl_error_t tegrabl_a_b_set_priority(void *smd_addr, uint32_t slot,
										 uint8_t priority)
{
	struct slot_meta_data *smd;
	pr_error("%s: start [smd: %p | slot: %u | priority: %u]\n", __func__, smd_addr, slot, priority);

	if ((smd_addr == NULL) || (slot >= (uint32_t)MAX_SLOTS)) {
		pr_error("%s: end 1 [TEGRABL_ERR_INVALID]\n", __func__);
		return TEGRABL_ERROR(TEGRABL_ERR_INVALID, 0);
	}

	smd = (struct slot_meta_data *)smd_addr;
	smd->slot_info[slot].priority = priority;

	pr_error("%s: end 2 [TEGRABL_NO_ERROR | smd->slot_info[slot].priority: %u]\n", __func__, smd->slot_info[slot].priority);
	return TEGRABL_NO_ERROR;
}

tegrabl_error_t tegrabl_a_b_set_active_slot(void *smd_addr, uint32_t slot_id)
{
	tegrabl_error_t error = TEGRABL_NO_ERROR;
	struct slot_meta_data *smd;
	uint32_t i;
	pr_error("%s: start [smd: %p | slot_id: %u]\n", __func__, smd_addr, slot_id);

	if ((smd_addr == NULL) || (slot_id >= MAX_SLOTS)) {
		error = TEGRABL_ERROR(TEGRABL_ERR_INVALID, 0);
		goto done;
	}

	smd = (struct slot_meta_data *)smd_addr;

	/* Decrease all slot's priority by 1 */
	for (i = 0; i < MAX_SLOTS; i++) {
		if (smd->slot_info[i].priority > 1U) {
			smd->slot_info[i].priority -= 1U;
			pr_error("%s: slot [%u] new priority: [%u]\n", __func__, i, smd->slot_info[i].priority);
		}
	}

	/* Reset slot info of given slot to default */
	error = tegrabl_a_b_set_priority((void *)smd, slot_id,
									 SLOT_PRIORITY_DEFAULT);
	if (error != TEGRABL_NO_ERROR) {
		pr_error("%s: tegrabl_a_b_set_priority error\n", __func__);
		TEGRABL_SET_HIGHEST_MODULE(error);
		goto done;
	}
	error = tegrabl_a_b_set_retry_count((void *)smd, slot_id,
										SLOT_RETRY_COUNT_DEFAULT);
	if (error != TEGRABL_NO_ERROR) {
		pr_error("%s: tegrabl_a_b_set_retry_count error\n", __func__);
		TEGRABL_SET_HIGHEST_MODULE(error);
		goto done;
	}

	error = tegrabl_a_b_set_successful((void *)smd, slot_id, 0);
	if (error != TEGRABL_NO_ERROR) {
		pr_error("%s: tegrabl_a_b_set_successful error\n", __func__);
		TEGRABL_SET_HIGHEST_MODULE(error);
		goto done;
	}
done:
	pr_error("%s: end [error]\n", __func__);
	tegrabl_error_print_error(error);
	return error;
}

static tegrabl_error_t load_smd_bin_copy(smd_bin_copy_t bin_copy)
{
	tegrabl_error_t error = TEGRABL_NO_ERROR;
	struct slot_meta_data *smd = NULL;
	struct tegrabl_partition part;
	char *smd_part;
	uint32_t crc32;
	uint32_t smd_len = sizeof(struct slot_meta_data);
	pr_error("%s: start [bin_copy: %s]\n", __func__, (bin_copy == SMD_COPY_PRIMARY) ? "SMD" : "SMD_b");

	smd_part = (bin_copy == SMD_COPY_PRIMARY) ? "SMD" : "SMD_b";
	error = tegrabl_partition_open(smd_part, &part);
	if (error != TEGRABL_NO_ERROR) {
		pr_error("%s: tegrabl_partition_open error\n", __func__);
		TEGRABL_SET_HIGHEST_MODULE(error);
		goto done;
	}

	/* Always move to first byte */
	error = tegrabl_partition_seek(&part, 0, TEGRABL_PARTITION_SEEK_SET);
	if (error != TEGRABL_NO_ERROR) {
		pr_error("%s: tegrabl_partition_seek error\n", __func__);
		TEGRABL_SET_HIGHEST_MODULE(error);
		goto done;
	}

	error = tegrabl_partition_read(&part, smd_loadaddress, smd_len);
	if (error != TEGRABL_NO_ERROR) {
		pr_error("%s: tegrabl_partition_read error\n", __func__);
		TEGRABL_SET_HIGHEST_MODULE(error);
		goto done;
	}
	tegrabl_partition_close(&part);

	smd = (struct slot_meta_data *)smd_loadaddress;

	if ((tegrabl_a_b_get_version(smd) & BOOT_CHAIN_VERSION_MASK) >=
				BOOT_CHAIN_VERSION_CRC32) {
		pr_error("%s: 1\n", __func__);
		/* Check crc for SMD sanity */
		crc32 = tegrabl_utils_crc32(0, smd_loadaddress,
									smd_len - sizeof(crc32));
		if (crc32 != smd->crc32) {
			error = TEGRABL_ERROR(TEGRABL_ERR_VERIFY_FAILED, 0);
			pr_error("%s corrupt with incorrect crc\n", smd_part);
			goto done;
		}
	}

done:
	pr_error("%s: end [error | smd: %p]\n", __func__, smd);
	tegrabl_error_print_error(error);
	return error;
}

tegrabl_error_t tegrabl_a_b_get_smd(void **smd)
{
	tegrabl_error_t error = TEGRABL_NO_ERROR;
	uint32_t smd_len;
	pr_error("%s: start [smd: %p]\n", __func__, *smd);

	TEGRABL_ASSERT(smd != NULL);

	/* Return smd address directly if it has already been loaded */
	if (smd_loadaddress != NULL) {
		pr_error("%s: smd_loadaddress already loaded [%p]\n", __func__, smd_loadaddress);
		*smd = smd_loadaddress;
		goto done;
	}

	smd_len = sizeof(struct slot_meta_data);
	smd_loadaddress = tegrabl_malloc(smd_len);
	if (smd_loadaddress == NULL) {
		error = TEGRABL_ERROR(TEGRABL_ERR_NO_MEMORY, 0);
		goto done;
	}

	/* Load and verify SMD primary copy */
	error = load_smd_bin_copy(SMD_COPY_PRIMARY);
	if (error == TEGRABL_NO_ERROR) {
		pr_error("%s: load primary (1) SMD copy [%p]\n", __func__, smd_loadaddress);
		*smd = smd_loadaddress;
		goto done;
	}

	/* If SMD primary copy has corrupted, fallback to SMD secondary copy */
	error = load_smd_bin_copy(SMD_COPY_SECONDARY);
	if (error == TEGRABL_NO_ERROR) {
		pr_error("%s: load secondary (2) SMD copy [%p]\n", __func__, smd_loadaddress);
		*smd = smd_loadaddress;
		goto done;
	}

	tegrabl_free(smd_loadaddress);
	smd_loadaddress = NULL;

done:
	pr_error("%s: end [error | smd: %p]\n", __func__, *smd);
	tegrabl_error_print_error(error);
	return error;
}

static tegrabl_error_t flush_smd_bin_copy(void *smd, smd_bin_copy_t bin_copy)
{
	tegrabl_error_t error = TEGRABL_NO_ERROR;
	struct tegrabl_partition part;
	char *smd_part;
	pr_error("%s: start [smd: %p | bin_copy: %s]\n", __func__, smd, (bin_copy == SMD_COPY_PRIMARY) ? "SMD" : "SMD_b");

	smd_part = (bin_copy == SMD_COPY_PRIMARY) ? "SMD" : "SMD_b";

	/* Write SMD back to storage */
	error = tegrabl_partition_open(smd_part, &part);
	if (error != TEGRABL_NO_ERROR) {
		pr_error("%s: tegrabl_partition_open fail\n", __func__);
		TEGRABL_SET_HIGHEST_MODULE(error);
		goto done;
	}

#if defined(CONFIG_ENABLE_QSPI)
	uint32_t storage_type;
	pr_error("%s: CONFIG_ENABLE_QSPI defined\n", __func__);
	/* Erase SMD since QSPI storage needs to be erased before writing */
	storage_type = tegrabl_blockdev_get_storage_type(part.block_device);
	if (storage_type == TEGRABL_STORAGE_QSPI_FLASH) {
		error = tegrabl_partition_erase(&part, false);
		if (error != TEGRABL_NO_ERROR) {
			TEGRABL_SET_HIGHEST_MODULE(error);
			goto done;
		}
	}
#endif

	/* Always move to first byte */
	error = tegrabl_partition_seek(&part, 0, TEGRABL_PARTITION_SEEK_SET);
	if (error != TEGRABL_NO_ERROR) {
		pr_error("%s: tegrabl_partition_seek fail\n", __func__);
		TEGRABL_SET_HIGHEST_MODULE(error);
		goto done;
	}

	error = tegrabl_partition_write(&part, smd, sizeof(struct slot_meta_data));
	if (error != TEGRABL_NO_ERROR) {
		pr_error("%s: tegrabl_partition_write fail\n", __func__);
		TEGRABL_SET_HIGHEST_MODULE(error);
		goto done;
	}

	tegrabl_partition_close(&part);
done:
	pr_error("%s: end [error]\n", __func__);
	tegrabl_error_print_error(error);
	return error;
}

tegrabl_error_t tegrabl_a_b_flush_smd(void *smd)
{
	tegrabl_error_t error = TEGRABL_NO_ERROR;
	struct slot_meta_data *bootctrl = (struct slot_meta_data *)smd;
	uint32_t smd_payload_len;
	pr_error("%s: start [smd: %p]\n", __func__, smd);

	if (smd == NULL) {
		error = TEGRABL_ERROR(TEGRABL_ERR_INVALID, 0);
		goto done;
	}

	/* Update crc field before writing */
	smd_payload_len = sizeof(struct slot_meta_data) - sizeof(uint32_t);
	bootctrl->crc32 = tegrabl_utils_crc32(0, smd, smd_payload_len);

	/* Always flush both primary SMD and secondary SMD for backup, otherwise
	 * secondary copy will be stale and may lead to unknown behavior */
	error = flush_smd_bin_copy(smd, SMD_COPY_PRIMARY);
	pr_error("%s: flush primary (1) error: [%u]\n", __func__, error);

	error = flush_smd_bin_copy(smd, SMD_COPY_SECONDARY);
	pr_error("%s: flush secondary (2) error: [%u]\n", __func__, error);

done:
	pr_error("%s: end [error]\n", __func__);
	tegrabl_error_print_error(error);
	return error;
}

/*
 * For REDUNDANCY BL only, there is no need to update SMD except
 *    when boot slot is switched on this boot. For such case, we need
 *    save the latest retry count from SR to SMD.
 */
static tegrabl_error_t check_non_user_redundancy_status(uint32_t reg,
		struct slot_meta_data *smd, uint8_t *update_smd)
{
	tegrabl_error_t error;
	uint8_t retry_count;
	uint32_t smd_slot;
	uint32_t current_slot;
	pr_error("%s: start [reg: %u | smd: %p | update_smd: %u]\n", __func__, reg, smd, *update_smd);

	*update_smd = 0;

	/* Get initial retrying boot slot from SMD */
	error = tegrabl_a_b_get_active_slot(smd, &smd_slot);
	if (error != TEGRABL_NO_ERROR) {
		pr_error("%s: tegrabl_a_b_get_active_slot fail\n", __func__);
		goto done;
	}

	/* Get actual boot slot from SR */
	current_slot = BOOT_CHAIN_REG_SLOT_NUM_GET(reg);

	/* Skip updating SMD if there is no boot slot changing */
	if (current_slot == smd_slot) {
		pr_error("%s: 1 skip updating SMD\n", __func__);
		goto done;
	}

	/*
	 * Restore retry_count for REDUNDANCY BL only.
	 * For REDUNDANCY_USER, it is UE to restore retry count when control
	 * reaches kernel.
	 */
	retry_count = tegrabl_a_b_get_retry_count_reg(current_slot, reg);
	++retry_count;
	tegrabl_a_b_set_retry_count_reg(current_slot, retry_count);

	*update_smd = 1;
done:
	pr_error("%s: end [error | update_smd: %u]\n", __func__, *update_smd);
	tegrabl_error_print_error(error);
	return error;
}

static void rotate_slots_priority(struct slot_meta_data *smd,
				uint32_t current_slot, uint8_t slot1_priority,
				uint8_t slot2_priority)
{
	pr_error("%s: start [smd: %p | current_slot: %u | slot1_priority: %u | slot2_priority: %u]\n", __func__, smd, current_slot, slot1_priority, slot2_priority);
	if (current_slot == 0) {
		pr_error("%s: 1\n", __func__);
		if (slot1_priority > slot2_priority) {
			pr_error("%s: 1a\n", __func__);
			tegrabl_a_b_set_priority(smd, current_slot,
					slot1_priority - 1);
		} else {
			if (slot2_priority < SLOT_PRIORITY_DEFAULT) {
				pr_error("%s: 1b\n", __func__);
				tegrabl_a_b_set_priority(smd, !current_slot,
						slot2_priority + 1);
			} else {
				pr_error("%s: 1c\n", __func__);
				/* handle maximum priority */
				tegrabl_a_b_set_priority(smd, current_slot,
						slot2_priority - 1);
			}
		}
	} else {
		pr_error("%s: 2\n", __func__);
		/* current slot is 1 */
		if (slot1_priority == (slot2_priority + 1)) {
			/* set the gap by 2, ex curr:15, non-curr: 13 */
			if (slot2_priority > 1) {
				pr_error("%s: 2a\n", __func__);
				tegrabl_a_b_set_priority(smd, !current_slot,
							slot2_priority - 1);
			} else {
				pr_error("%s: 2b\n", __func__);
				/* handle minimum priority */
				tegrabl_a_b_set_priority(smd, current_slot,
							slot1_priority + 1);
			}
		} else {
			pr_error("%s: 2c\n", __func__);
			/* When the gap is already two, reverse priority */
			tegrabl_a_b_set_priority(smd, !current_slot,
						SLOT_PRIORITY_DEFAULT);
			tegrabl_a_b_set_priority(smd, current_slot,
						SLOT_PRIORITY_DEFAULT - 1);
		}
	}
	pr_error("%s: end\n", __func__);
}

static tegrabl_error_t check_user_redundancy_status(uint32_t reg,
		struct slot_meta_data *smd, uint8_t *update_smd)
{
	tegrabl_error_t error;
	uint8_t retry_count;
	uint8_t slot1_priority, slot2_priority;
	uint32_t current_slot;
	pr_error("%s: start [reg: %u | smd: %p | update_smd: %u]\n", __func__, reg, smd, *update_smd);

	*update_smd = 0;

	/*
	 * Restore retry_count
	 */
	current_slot = BOOT_CHAIN_REG_SLOT_NUM_GET(reg);
	retry_count = tegrabl_a_b_get_retry_count_reg(current_slot, reg);
	++retry_count;
	tegrabl_a_b_set_retry_count_reg(current_slot, retry_count);
	reg = tegrabl_get_boot_slot_reg();

	/*
	 * For any boot failure at this stage (u-boot or kernel), the policy
	 * is to try up to two times on a slot by changing slot's priority.
	 * When expired, switch slot and try again.
	 *
	 * Each try by either decreasing current slot's priority or increasing
	 * the other slot's priority.
	 *
	 * If control reaches kernel, UE restores slots priorities.
	 * If not, a/b logic at MB1 on next boot may switch boot slot based on
	 * slots priorities.
	 */
	error = tegrabl_a_b_get_priority(smd, current_slot,
				&slot1_priority);
	if (error != TEGRABL_NO_ERROR) {
		pr_error("%s: 1 tegrabl_a_b_get_priority error\n", __func__);
		goto done;
	}
	error = tegrabl_a_b_get_priority(smd, !current_slot,
				&slot2_priority);
	if (error != TEGRABL_NO_ERROR) {
		pr_error("%s: 2 tegrabl_a_b_get_priority error\n", __func__);
		goto done;
	}

	/* Change priority only when both slots are bootable */
	if (tegrabl_a_b_get_retry_count_reg(!current_slot, reg) &&
		slot1_priority && slot2_priority) {
		pr_error("%s: 1 slot1_priority: %u | slot1_priority: %u\n", __func__, slot1_priority, slot2_priority);
		/* current slot priority must be greater or equal than
		 * non-current slot */
		TEGRABL_ASSERT(slot1_priority >= slot2_priority);
		pr_error("%s: 2\n", __func__);
		/*
		 * Change slot priorities so that each boot-chain can be
		 * tried twice before switching boot-chain if boot failed
		 * after cboot
		 */
		rotate_slots_priority(smd, current_slot, slot1_priority,
				slot2_priority);
	}
	*update_smd = 1;
done:
	pr_error("%s: end [error | update_smd: %u]\n", __func__, *update_smd);
	tegrabl_error_print_error(error);
	return error;
}

static tegrabl_error_t check_redundancy_status(uint32_t reg,
		struct slot_meta_data *smd, uint8_t *update_smd)
{
	tegrabl_error_t error;
	pr_error("%s: start [reg: %u | smd: %p | update_smd: %u]\n", __func__, reg, smd, *update_smd);

	/*
	 * For REDUNDANCY BL only, there is no need to update SMD except
	 *    when boot slot is switched on this boot. For such case, we need
	 *    save the latest retry count from SR to SMD.
	 *
	 * For REDUNDANCY USER, ie, redundancy is supported for cboot's payload
	 *    such as u-boot and kernel. For such case, since BL is already
	 *    successfully booted to cboot, we should restore retry count but
	 *    update slot priority. If control can reach UE, UE is responsible
	 *    to restore slot priority. If control can not reach UE, boot
	 *    failure (including device reboot) happens between cboot and UE.
	 *    By changing slot priority values, A/B logic at mb1 can switch
	 *    boot slot when current boot slot's priority is lower than the
	 *    other slot.
	 */
	pr_error("%s: BOOTCTRL_SUPPORT_REDUNDANCY_USER: %lu\n", __func__, BOOTCTRL_SUPPORT_REDUNDANCY_USER(tegrabl_a_b_get_version(smd)));
	if (BOOTCTRL_SUPPORT_REDUNDANCY_USER(tegrabl_a_b_get_version(smd))
		== 0U) {
		pr_error("%s: 1 - check_non_user_redundancy_status\n", __func__);
		/* REDUNDANCY is supported at bootloader only */
		error = check_non_user_redundancy_status(reg, smd, update_smd);
	} else {
		pr_error("%s: 2 - check_user_redundancy_status\n", __func__);
		/* REDUNDANCY is supported at kernel (or u-boot) */
		error = check_user_redundancy_status(reg, smd, update_smd);
	}

	pr_error("%s: end [error | update_smd: %u]\n", __func__, *update_smd);
	tegrabl_error_print_error(error);
	return error;
}

tegrabl_error_t tegrabl_a_b_update_smd(void)
{
	tegrabl_error_t error = TEGRABL_NO_ERROR;
	struct slot_meta_data *smd = NULL;
	uint32_t reg;
	uint8_t bc_flag;
	uint8_t update_smd = 0;
	pr_error("%s: start\n", __func__);

	reg = tegrabl_get_boot_slot_reg();
	bc_flag = (uint8_t)BOOT_CHAIN_REG_UPDATE_FLAG_GET(reg);
	pr_error("%s: 1 - bc_flag [%u]\n", __func__, bc_flag);
	if ((BOOT_CHAIN_REG_MAGIC_GET(reg) == BOOT_CHAIN_REG_MAGIC) &&
		((bc_flag == BC_FLAG_OTA_ON) || (bc_flag == BC_FLAG_REDUNDANCY_BOOT))) {
		pr_error("%s: 2\n", __func__);
		error = tegrabl_a_b_get_smd((void **)&smd);
		if (error != TEGRABL_NO_ERROR) {
			pr_error("%s: tegrabl_a_b_get_smd error\n", __func__);
			TEGRABL_SET_HIGHEST_MODULE(error);
			goto done;
		}

		/*
		 * When control reaches here, BL can claim safe.
		 *
		 * If OTA in progress, save retry count.
		 * or
		 * If REDUNDANCY enabled, check redundancy status. save retry count
		 *    based on return flag.
		 */
		if ((bc_flag == BC_FLAG_REDUNDANCY_BOOT) &&
		    (BOOTCTRL_SUPPORT_REDUNDANCY(tegrabl_a_b_get_version(smd)) != 0U)) {
			pr_error("%s: 3\n", __func__);
			error = check_redundancy_status(reg, smd, &update_smd);
			if ((error != TEGRABL_NO_ERROR) || (update_smd == 0U)) {
				pr_error("%s: check_redundancy_status error [update_smd: %u]\n", __func__, update_smd);
				goto done;
			}
		}

		/* Update SMD based on SR and flush to storage */
		reg = tegrabl_get_boot_slot_reg();
		tegrabl_a_b_copy_retry_count(smd, &reg, FROM_REG_TO_SMD);
		error = tegrabl_a_b_flush_smd(smd);
		if (error != TEGRABL_NO_ERROR) {
			pr_error("%s: tegrabl_a_b_flush_smd error\n", __func__);
			TEGRABL_SET_HIGHEST_MODULE(error);
			goto done;
		}
	}

done:
	/* Clear SR before handing over to kernel */
	if (BOOT_CHAIN_REG_MAGIC_GET(reg) == BOOT_CHAIN_REG_MAGIC) {
		pr_error("%s: 4\n", __func__);
		reg = 0;
		tegrabl_set_boot_slot_reg(reg);
	}

	if (error != TEGRABL_NO_ERROR) {
		TEGRABL_SET_HIGHEST_MODULE(error);
	}

	pr_error("%s: end [error]\n", __func__);
	tegrabl_error_print_error(error);
	return error;
}
