/*
 * Copyright (c) 2015-2019, NVIDIA Corporation. All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation. Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

/**
 * @brief Define error module
 */
#define MODULE TEGRABL_ERR_BRBCT
/**
 * @brief Define NVBOOT_TARGET_FPGA to false
 */
#define NVBOOT_TARGET_FPGA 0

#include "build_config.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <inttypes.h>
#include <tegrabl_utils.h>
#include <tegrabl_debug.h>
#include <tegrabl_brbit.h>
#include <tegrabl_brbct.h>
#include <tegrabl_malloc.h>
#include <tegrabl_addressmap.h>
#include <nvboot_bct.h>
#include <nvboot_config.h>
#include <tegrabl_brbct_err_aux.h>
#include <tegrabl_crypto_se.h>

#define SHA2_DIGEST_BYTES 32U

static uintptr_t brbct;

tegrabl_error_t tegrabl_brbct_relocate_to_sdram(uint64_t sdram_brbct_location)
{
	tegrabl_error_t err = TEGRABL_NO_ERROR;
	uint32_t br_bct_location = 0;
	uint32_t br_bct_size = 0;

	if (sdram_brbct_location == 0ULL) {
		err = TEGRABL_ERROR(TEGRABL_ERR_BAD_PARAMETER, TEGRABL_BRBCT_RELOCATE_TO_SDRAM_1);
		TEGRABL_SET_ERROR_STRING(err, "sdram_brbct_location = 0x%" PRIx64, sdram_brbct_location);
		goto fail;
	}

	br_bct_location = tegrabl_brbct_get();
	pr_trace("BRBCT @ 0x%x\n", br_bct_location);

	if (br_bct_location == 0U) {
		err = TEGRABL_ERROR(TEGRABL_ERR_NOT_INITIALIZED, TEGRABL_BRBCT_RELOCATE_TO_SDRAM_1);
		TEGRABL_SET_ERROR_STRING(err, "BR-BCT");
		goto fail;
	}

	br_bct_size = tegrabl_brbct_size();

	(void)memcpy((uint8_t *)(uintptr_t)sdram_brbct_location,
			(uint8_t *)(uintptr_t)br_bct_location, br_bct_size);

	brbct = (uintptr_t)sdram_brbct_location;

	pr_trace("%s relocated to 0x%08x\n", "BR-BCT", (uint32_t)sdram_brbct_location);

fail:
	return err;
}

tegrabl_error_t tegrabl_brbct_init(uintptr_t load_address)
{
	tegrabl_error_t err = TEGRABL_NO_ERROR;

	if (load_address == 0UL) {
		err = TEGRABL_ERROR(TEGRABL_ERR_BAD_PARAMETER, TEGRABL_BRBCT_INIT_1);
		TEGRABL_SET_ERROR_STRING(err, "load_address = 0x%" PRIx64, (uint64_t)load_address);
	} else {
		brbct = load_address;
	}

	return err;
}

uintptr_t tegrabl_brbct_get(void)
{
	tegrabl_error_t err = TEGRABL_NO_ERROR;
	uint32_t brbct_location = 0;
	void *ptr = &brbct_location;
	uint32_t size = sizeof(brbct_location);

	if (brbct == 0U) {
		err = tegrabl_brbit_get_data(TEGRABL_BRBIT_DATA_ACTIVE_BCT_PTR, 0,
									 (void **)&ptr, &size);
		if (err == TEGRABL_NO_ERROR) {
			brbct = brbct_location;
		}
	}

	pr_trace("br bct location = 0x%" PRIx64 "\n", (uint64_t)brbct);

	return brbct;
}

tegrabl_error_t tegrabl_brbct_write_multiple(
	const void *buffer, struct tegrabl_partition *partition, uint64_t part_size,
	uint64_t bct_size, uint32_t chunk_size)
{
	tegrabl_error_t err = TEGRABL_NO_ERROR;
	uint32_t i = 0;
	uint64_t num_blocks = 0;
	uint64_t bootrom_block_size = 0;
	uint32_t bootrom_page_size = 0;
	uint64_t pages_in_bct = 0;
	uint64_t slot_size = 0;
	uint32_t br_bct_count = 0;
	uint32_t block_size = 0;
	uint64_t rounded_br_block_size = 0;
	const NvBootConfigTable *br_bct_ptr = NULL;
	uint64_t temp1;
	uint64_t temp;

	/* Block 0 : slot 0 , slot 1  & Block 1 <-> n-1 : slot 0 */
	br_bct_ptr = (const NvBootConfigTable *)tegrabl_brbct_get();
	bootrom_block_size = 1ULL << br_bct_ptr->BlockSizeLog2;
	bootrom_page_size = 1UL << br_bct_ptr->PageSizeLog2;
	num_blocks = part_size / bootrom_block_size;
	pages_in_bct = DIV_CEIL(bct_size, bootrom_page_size);

	/*
	The term "slot" refers to a potential location
	of a BCT in a block. A slot is the smallest integral
	number of pages that can hold a BCT.Thus, every
	BCT begins at the start of a page and may span
	multiple pages. A block is a space in memory that
	can hold multiple slots of BCTs.
	*/
	slot_size = pages_in_bct * bootrom_page_size;

	/*
	The BCT search sequence followed by BootROM is:
	Block 0, Slot 0
	Block 0, Slot 1
	Block 1, Slot 0
	Block 1, Slot 1
	Block 1, Slot 2
	.....
	Block 1, Slot N
	Block 2, Slot 0
	.....
	Block 2, Slot N
	.....
	Block 63, Slot N
	Based on the search sequence, we write the
	block 0, slot 1 BCT first, followed by one BCT
	in slot 0 of subsequent blocks and lastly one BCT
	in block0, slot 0.
	*/
	/* Special case block 0 : slot 0 & 1 BCT's */
	/* Block 0 Slot 0 */
	err = tegrabl_partition_write(partition, buffer, chunk_size);
	if (err != TEGRABL_NO_ERROR) {
		TEGRABL_SET_HIGHEST_MODULE(err);
		TEGRABL_PRINT_ERROR_STRING(TEGRABL_ERR_WRITE_FAILED, "BCT in block %d slot %d", 0, 0);
		goto fail;
	}
	br_bct_count++;

	/* Block 0 Slot 1 */
	/* Get next sector byte , basically slot size aligned to block size */
	block_size = TEGRABL_BLOCKDEV_BLOCK_SIZE(partition->block_device);
	temp = (ROUND_UP((uint64_t)slot_size, block_size));
	err = tegrabl_partition_seek(partition, (int64_t)temp,
			TEGRABL_PARTITION_SEEK_SET);
	if (err != TEGRABL_NO_ERROR) {
		TEGRABL_SET_HIGHEST_MODULE(err);
		pr_error("Failed to seek partition\n");
		goto fail;
	}

	err = tegrabl_partition_write(partition, buffer, chunk_size);
	if (err != TEGRABL_NO_ERROR) {
		TEGRABL_SET_HIGHEST_MODULE(err);
		TEGRABL_PRINT_ERROR_STRING(TEGRABL_ERR_WRITE_FAILED, "BCT in block %d slot %d", 0, 1);
		goto fail;
	}
	br_bct_count++;

	/* Fill Slot 0 for all other blocks */
	rounded_br_block_size = ROUND_UP(bootrom_block_size, block_size);
	for (i = 1UL; i < num_blocks; i++) {
		if (br_bct_count >= BR_BCT_MAX_COPIES) {
			break;
		}
		temp1 = (uint64_t)i * rounded_br_block_size;
		err = tegrabl_partition_seek(partition, (int64_t)temp1,
				TEGRABL_PARTITION_SEEK_SET);
		if (err != TEGRABL_NO_ERROR) {
			TEGRABL_SET_HIGHEST_MODULE(err);
			pr_error("Failed to seek partition\n");
			goto fail;
		}

		err = tegrabl_partition_write(partition, buffer, chunk_size);
		if (err != TEGRABL_NO_ERROR) {
			TEGRABL_SET_HIGHEST_MODULE(err);
			TEGRABL_PRINT_ERROR_STRING(TEGRABL_ERR_WRITE_FAILED, "BCT in block %d slot %d", i, 0);
			goto fail;
		}
		br_bct_count++;
	}

fail:
	return err;
}

uintptr_t tegrabl_get_nvpt_offset(void)
{
	uintptr_t bctptr;
	bctptr = tegrabl_brbct_get();
	if (bctptr != 0U) {
		return bctptr + tegrabl_brbct_nvpt_offset();
	} else {
		return bctptr;
	}
}

tegrabl_error_t tegrabl_brbct_verify_customerdata(uintptr_t brbct_addr)
{
	tegrabl_error_t err = TEGRABL_NO_ERROR;
	uint8_t *customerdataptr;
	uint8_t *hash_data_buf = NULL;
	uint8_t *computed_hash = NULL;
	uint32_t hash_data_length = 0UL;
	uint32_t signed_custdata_length = 0;
	int are_digest_same;

	if (brbct_addr == 0UL) {
		err = TEGRABL_ERROR(TEGRABL_ERR_INVALID, 1);
		goto fail;
	}

	/* nvimagegen has a bug that it aligns the signed-part of customer-data before computing the
	 * SHA256. This ends up making the signed section overlap with the 32B SHA256 hash.
	 * In order to retain compatibility with older releases, same computation is being followed
	 * on device-side as well */
	customerdataptr = (uint8_t *)brbct_addr + tegrabl_brbct_customerdata_offset();
	signed_custdata_length = tegrabl_brbct_customerdata_size() - SHA2_DIGEST_BYTES;
	hash_data_length = ROUND_UP(signed_custdata_length, 16UL);

	pr_trace("CustomerData offset: %u\n", tegrabl_brbct_customerdata_offset());
	pr_trace("Signed CustData length: %u\n", signed_custdata_length);
	pr_trace("Hash Data length: %u\n", hash_data_length);

	hash_data_buf = (uint8_t *)tegrabl_alloc(TEGRABL_HEAP_DMA, hash_data_length);
	if (hash_data_buf == NULL) {
		err = TEGRABL_ERROR(TEGRABL_ERR_NO_MEMORY, 0UL);
		goto fail;
	}

	(void)memcpy(hash_data_buf, customerdataptr, signed_custdata_length);
	(void)memset(hash_data_buf + signed_custdata_length, 0U, hash_data_length - signed_custdata_length);

	computed_hash = (uint8_t *)tegrabl_alloc(TEGRABL_HEAP_DMA, SHA2_DIGEST_BYTES);
	if (computed_hash == NULL) {
		err = TEGRABL_ERROR(TEGRABL_ERR_NO_MEMORY, 1UL);
		goto fail;
	}

	(void)memset(computed_hash, 0, SHA2_DIGEST_BYTES);

	err = tegrabl_crypto_compute_sha2(hash_data_buf, hash_data_length, computed_hash);
	if (err != TEGRABL_NO_ERROR) {
		goto fail;
	}

	are_digest_same = memcmp(computed_hash, customerdataptr + signed_custdata_length, SHA2_DIGEST_BYTES);
	if (are_digest_same != 0) {
		err = TEGRABL_ERROR(TEGRABL_ERR_VERIFY_FAILED, 0);
	}

fail:
	if (hash_data_buf != NULL) {
		tegrabl_dealloc(TEGRABL_HEAP_DMA, hash_data_buf);
	}
	if (computed_hash != NULL) {
		tegrabl_dealloc(TEGRABL_HEAP_DMA, computed_hash);
	}

	return err;
}

tegrabl_error_t tegrabl_brbct_update_customer_data(uintptr_t new_bct,
												   uint32_t size)
{
	tegrabl_error_t status = TEGRABL_NO_ERROR;
	uintptr_t cur_bct;
	uint32_t bct_size;
	uint32_t custdata_offset;
	uint32_t custdata_size;

	/* Initialize brbct from device */
	bct_size = tegrabl_brbct_size();

	if (size != bct_size) {
		pr_error("Brbct size is invalid.\n");
		status = TEGRABL_ERROR(TEGRABL_ERR_INVALID, 0);
		goto exit;
	}

	cur_bct = tegrabl_brbct_get();
	/* copy costomer data from original brbct to new brbct */
	custdata_offset = tegrabl_brbct_customerdata_offset();
	custdata_size = tegrabl_brbct_customerdata_size();
	(void)memcpy((uint8_t *)(new_bct + custdata_offset),
				 (uint8_t *)(cur_bct + custdata_offset), custdata_size);

exit:
	return status;
}
