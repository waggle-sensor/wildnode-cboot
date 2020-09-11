/*
 * Copyright (c) 2015-2018, NVIDIA Corporation.	All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#define MODULE TEGRABL_ERR_VERIFIED_BOOT

#include <verified_boot.h>
#include <verified_boot_ui.h>
#include <err.h>
#include <debug.h>
#include <malloc.h>
#include <sys/types.h>
#include <tegrabl_error.h>
#include <arse0.h>
#include <string.h>
#include <sm_err.h>
#include <tegrabl_debug.h>
#include <tegrabl_odmdata_soc.h>
#include <tegrabl_pkc_ops.h>
#include <tegrabl_partition_manager.h>
#include <tegrabl_a_b_boot_control.h>
#include <tegrabl_a_b_partition_naming.h>
#include <tegrabl_fastboot_partinfo.h>
#include <tegrabl_exit.h>
#include <tegrabl_cache.h>
#include <tegrabl_linuxboot_helper.h>
#include <libfdt.h>
#include <libavb/libavb.h>

#if defined(IS_T186)
#include <tegrabl_se.h>
#else
#include <tegrabl_crypto_se.h>
#endif

static inline AvbIOResult is_device_unlocked(AvbOps *ops, bool *is_unlocked)
{
	if (tegrabl_odmdata_get() & (1 << TEGRA_BOOTLOADER_LOCK_BIT)) {
		*is_unlocked = false;
	} else {
		*is_unlocked = true;
	}
	return AVB_IO_RESULT_OK;
}

static void *boot_img_laddr;
static void *kernel_dtb_laddr;
static AvbIOResult read_from_partition(AvbOps *ops, const char *partition,
									   int64_t offset, size_t num_bytes,
									   void *buffer, size_t *out_num_read)
{
	tegrabl_error_t err = TEGRABL_NO_ERROR;
	size_t part_size = 0;
	struct tegrabl_partition part;
	const char *tegra_part_name = NULL;
	const struct tegrabl_fastboot_partition_info *part_info = NULL;
	const char *suffix = NULL;

	TEGRABL_UNUSED(ops);

	suffix = tegrabl_a_b_get_part_suffix(partition);
	part_info = tegrabl_fastboot_get_partinfo(partition);
	tegra_part_name = tegrabl_fastboot_get_tegra_part_name(suffix, part_info);

	/* boot.img and kernel-dtb is already in memory, direct copy to improve
	 * performance */
	if (!strcmp(part_info->fastboot_part_name, "boot") && (offset == 0)) {
		TEGRABL_ASSERT(boot_img_laddr);
		memcpy(buffer, boot_img_laddr + offset, num_bytes);
		goto done;
	}
	if (!strcmp(part_info->fastboot_part_name, "kernel-dtb") && (offset == 0)) {
		TEGRABL_ASSERT(kernel_dtb_laddr);
		memcpy(buffer, kernel_dtb_laddr + offset, num_bytes);
		goto done;
	}

	err = tegrabl_partition_open(tegra_part_name, &part);
	if (err != TEGRABL_NO_ERROR) {
		return AVB_IO_RESULT_ERROR_NO_SUCH_PARTITION;
	}

	/* If offset < 0, we are actually reading the footer */
	if (offset < 0) {
		part_size = tegrabl_partition_size(&part);
		offset = part_size - (-offset);
	}
	err = tegrabl_partition_seek(&part, offset, TEGRABL_PARTITION_SEEK_SET);
	if (err != TEGRABL_NO_ERROR) {
		return AVB_IO_RESULT_ERROR_RANGE_OUTSIDE_PARTITION;
	}
	err = tegrabl_partition_read(&part, buffer, num_bytes);
	if (err != TEGRABL_NO_ERROR) {
		return AVB_IO_RESULT_ERROR_IO;
	}

done:
	*out_num_read = num_bytes;
	return AVB_IO_RESULT_OK;
}

static AvbIOResult get_unique_guid_for_partition(AvbOps *ops,
												 const char *part_name,
												 char *guid_buf,
												 size_t guid_buf_size)
{
	tegrabl_error_t err = TEGRABL_NO_ERROR;
	struct tegrabl_partition part;
	const struct tegrabl_fastboot_partition_info *part_info = NULL;
	const char *tegra_part_name = NULL;
	const char *suffix = NULL;

	TEGRABL_ASSERT(part_name);
	TEGRABL_ASSERT(guid_buf);
	TEGRABL_ASSERT(guid_buf_size);

	suffix = tegrabl_a_b_get_part_suffix(part_name);
	part_info = tegrabl_fastboot_get_partinfo(part_name);
	tegra_part_name = tegrabl_fastboot_get_tegra_part_name(suffix, part_info);

	err = tegrabl_partition_open(tegra_part_name, &part);
	if (err != TEGRABL_NO_ERROR) {
		return AVB_IO_RESULT_ERROR_NO_SUCH_PARTITION;
	}
	memcpy(guid_buf, part.partition_info->guid, guid_buf_size);

	return AVB_IO_RESULT_OK;
}

static AvbIOResult hash_salt_image(AvbOps *ops, const uint8_t *payload,
								   size_t size, uint8_t *digest,
								   const char *algorithm)
{
#if defined(IS_T186)
	struct se_sha_input_params sha_input;
	struct se_sha_context sha_context;
#endif
	tegrabl_error_t ret = TEGRABL_NO_ERROR;

	TEGRABL_UNUSED(ops);
	TEGRABL_ASSERT(payload);
	TEGRABL_ASSERT(digest);

#if defined(IS_T186)
	/* Vbmeta hash algorithm: SHA256, SHA512 */
	if (!strcmp(algorithm, "sha512")) {
		sha_context.hash_algorithm = SE_SHAMODE_SHA512;
	} else if (!strcmp(algorithm, "sha256")) {
		sha_context.hash_algorithm = SE_SHAMODE_SHA256;
	} else {
		pr_error("Hash algorithm not supported: %s\n", algorithm);
		return AVB_IO_RESULT_ERROR_IO;
	}

	sha_context.input_size = (uint32_t)size;
	sha_input.block_addr = (uintptr_t)payload;
	sha_input.block_size = (uint32_t)size;
	sha_input.size_left = (uint32_t)size;
	sha_input.hash_addr = (uintptr_t)digest;

	ret = tegrabl_se_sha_process_payload(&sha_input, &sha_context);
#else
	TEGRABL_UNUSED(algorithm);
	ret = tegrabl_crypto_compute_sha2((uint8_t *)payload, size, digest);
#endif

	if (ret != TEGRABL_NO_ERROR) {
		return AVB_IO_RESULT_ERROR_IO;
	}

	return AVB_IO_RESULT_OK;
}

/* Compare the keys k1 and k2. They are both expected to be in little endian
 * format
 */
static inline bool are_keys_identical(const uint8_t *k1, const uint8_t *k2)
{
	return !memcmp(k1, k2, VERITY_KEY_SIZE);
}

static AvbIOResult validate_vbmeta_public_key(AvbOps *ops,
											  const uint8_t *pub_key,
											  size_t pub_key_len,
											  const uint8_t *pub_key_metadata,
											  size_t pub_key_metadata_len,
											  bool *out_is_trusted)
{
	tegrabl_error_t err = TEGRABL_NO_ERROR;
	uint8_t bct_key_mod[VERITY_KEY_SIZE];

	TEGRABL_UNUSED(pub_key_metadata);
	TEGRABL_UNUSED(pub_key_metadata_len);
	TEGRABL_ASSERT(pub_key);
	TEGRABL_ASSERT(pub_key_len);

	/* Get public key from BCT */
	err = tegrabl_pkc_modulus_get(bct_key_mod);
	if (err != TEGRABL_NO_ERROR) {
		pr_error("Failed to get oem key from BCT\n");
		return AVB_IO_RESULT_ERROR_IO;
	}

	if ((pub_key_len != VERITY_KEY_SIZE) ||
		!are_keys_identical(bct_key_mod, pub_key)) {
		*out_is_trusted = false;
		return AVB_IO_RESULT_OK;
	}

	*out_is_trusted = true;
	return AVB_IO_RESULT_OK;
}

bool is_public_key_mismatch(AvbSlotVerifyData *slot_data)
{
	uint8_t i;

	/* Any public key set in AvbSlotVerifyData struct is mismatching the signing
	 * key */
	for (i = 0; i < slot_data->num_vbmeta_images; i++) {
		if (slot_data->vbmeta_images[i].pub_key != NULL) {
			return true;
		}
	}
	return false;
}

/* Possible values:
 * One of VERIFIED_BOOT_YELLOW_STATE/VERIFIED_BOOT_GREEN_STATE/
 *        VERIFIED_BOOT_ORANGE_STATE once verified,
 * initialised as VERIFIED_BOOT_UNKNOWN_STATE */
static boot_state_t s_boot_state;

status_t verified_boot_get_boot_state(boot_state_t *bs,
									  AvbSlotVerifyData **slot_data)
{
	tegrabl_error_t err = TEGRABL_NO_ERROR;
	AvbSlotVerifyResult avbres = AVB_SLOT_VERIFY_RESULT_OK;
	AvbOps ops;
	const char *requested_partitions[] = {NULL};
	bool unlocked;
	char ab_suffix[BOOT_CHAIN_SUFFIX_LEN + 1];

	/* Return early if already verified */
	if (s_boot_state != VERIFIED_BOOT_UNKNOWN_STATE) {
		*bs = s_boot_state;
		return NO_ERROR;
	}

#ifndef IS_T186
	tegrabl_crypto_early_init();
#endif

	/* Use libavb API to verify the boot */
	ops.read_from_partition = read_from_partition;
	ops.read_is_device_unlocked = is_device_unlocked;
	ops.hash_salt_image = hash_salt_image;
	ops.validate_vbmeta_public_key = validate_vbmeta_public_key;
	ops.get_unique_guid_for_partition = get_unique_guid_for_partition;

	if (is_device_unlocked(&ops, &unlocked) != AVB_IO_RESULT_OK) {
		pr_error("Failed to determine whether device is unlocked.\n");
		goto exit;
	}

	err = tegrabl_a_b_get_bootslot_suffix(ab_suffix, true);
	if (err != TEGRABL_NO_ERROR) {
		goto exit;
	}

	avbres = avb_slot_verify(&ops,
							 requested_partitions,
							 ab_suffix,
							 unlocked,  /* allow_verification_error */
							 AVB_HASHTREE_ERROR_MODE_RESTART_AND_INVALIDATE,
							 slot_data);

	/**
	 * Orange state:
	 * Device is unlocked
	 * Red state:
	 * Any fatal failure during verification
	 * Yellow state:
	 * Verification passed, but public key in vbmeta does not match the keys
	 * indicated in vbmeta.img
	 * Green state:
	 * Verification passed with the key in vbmeta.img
	 **/
	if (unlocked) {
		*bs = VERIFIED_BOOT_ORANGE_STATE;
	} else if (avbres != AVB_SLOT_VERIFY_RESULT_OK) {
		*bs = VERIFIED_BOOT_RED_STATE;
	} else if (is_public_key_mismatch(*slot_data)) {
		*bs = VERIFIED_BOOT_YELLOW_STATE;
	} else {
		*bs = VERIFIED_BOOT_GREEN_STATE;
	}

exit:
	return NO_ERROR;
}

status_t verified_boot_ui(boot_state_t bs, AvbSlotVerifyData *slot_data)
{
	switch (bs) {
	case VERIFIED_BOOT_RED_STATE:
		return verified_boot_red_state_ui();
	case VERIFIED_BOOT_YELLOW_STATE:
		return verified_boot_yellow_state_ui(slot_data);
	case VERIFIED_BOOT_ORANGE_STATE:
		return verified_boot_orange_state_ui();
	default:
		return ERR_INVALID_ARGS;
	}
}

tegrabl_error_t verify_boot(union tegrabl_bootimg_header *hdr,
							void *kernel_dtb, void *kernel_dtbo)
{
	status_t ret = NO_ERROR;
	tegrabl_error_t err = TEGRABL_NO_ERROR;
	boot_state_t bs;
	struct root_of_trust r_o_t_params = {0};
	uint8_t i;

	/* DTBO is not enabled in verified boot 1.0 in Android N */
	TEGRABL_UNUSED(kernel_dtbo);

	AvbSlotVerifyData *slot_data;
	const char *bs_str = NULL;

	/* Cache the load address of boot.img and kernel-dtb */
	boot_img_laddr = (void *)hdr;
	kernel_dtb_laddr = kernel_dtb;

	ret = verified_boot_get_boot_state(&bs, &slot_data);
	if (ret != NO_ERROR) {
		panic("An error occured in verified boot module.\n");
	}

	r_o_t_params.magic_header = MAGIC_HEADER;
	r_o_t_params.version = VERSION;

	if (slot_data != NULL) {
		for (i = 0; i < slot_data->num_vbmeta_images; i++) {
			if ((0 == strcmp(slot_data->vbmeta_images[i].partition_name, "boot")) &&
				(slot_data->vbmeta_images[i].pub_key_size == VERITY_KEY_SIZE / 8)) {
				memcpy(r_o_t_params.boot_pub_key, slot_data->vbmeta_images[i].pub_key,
					   VERITY_KEY_SIZE);
			} else if ((0 == strcmp(slot_data->vbmeta_images[i].partition_name, "kernel-dtb")) &&
					   (slot_data->vbmeta_images[i].pub_key_size == VERITY_KEY_SIZE / 8)) {
				memcpy(r_o_t_params.dtb_pub_key, slot_data->vbmeta_images[i].pub_key,
					   VERITY_KEY_SIZE);
			}
		}
	}

	r_o_t_params.verified_boot_state = bs;

	/* Flush these bytes so that the monitor can access them */
	tegrabl_arch_clean_dcache_range((addr_t)&r_o_t_params,
									sizeof(struct root_of_trust));

	/* Passing the address directly works because VMEM=PMEM in Cboot.
	 * If this assumption changes, we need to explicitly convert the
	 * virtual adddress to a physical address prior to the smc_call.
	 * Re-attempt the smc if it receives SM_ERR_INTERRUPTED.
	 */
	ret = smc_call(SMC_SET_ROOT_OF_TRUST, (uintptr_t)&r_o_t_params,
				   sizeof(struct root_of_trust));
	while (ret == SM_ERR_INTERRUPTED) {
		pr_info("SMC Call Interrupted, retry with smc # %x\n",
				SMC_TOS_RESTART_LAST);
		ret = smc_call(SMC_TOS_RESTART_LAST, 0, 0);
	}

	if (ret != NO_ERROR) {
		pr_error("Failed to pass verified boot params: %x\n", ret);
		err = TEGRABL_ERROR(TEGRABL_ERR_COMMAND_FAILED, 0);
		goto fail;
	}

	bs_str = (bs == VERIFIED_BOOT_RED_STATE) ? "red" :
			 (bs == VERIFIED_BOOT_YELLOW_STATE) ? "yellow" :
			 (bs == VERIFIED_BOOT_GREEN_STATE) ? "green" :
			 (bs == VERIFIED_BOOT_ORANGE_STATE) ? "orange" : "unknown";

	pr_info("Verified boot state = %s\n", bs_str);

	if ((slot_data != NULL) && (slot_data->cmdline != NULL)) {
		err = tegrabl_linuxboot_set_vbmeta_info(slot_data->cmdline);
		if (err != TEGRABL_NO_ERROR) {
			goto fail;
		}
	}
	if (bs_str != NULL) {
	err = tegrabl_linuxboot_set_vbstate(bs_str);
		if (err != TEGRABL_NO_ERROR) {
			goto fail;
		}
	}

	if (bs != VERIFIED_BOOT_GREEN_STATE) {
		verified_boot_ui(bs, slot_data);
	}

	/* We should not allow booting if red state, trigger a reboot to hit a_b
	 * boot logic in next boot cycle */
	if (bs == VERIFIED_BOOT_RED_STATE) {
		err = tegrabl_reset();
	}

fail:
	return err;
}
