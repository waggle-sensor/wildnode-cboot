/*
 * Copyright (c) 2015-2019, NVIDIA CORPORATION. All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation. Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#ifndef TEGRABL_BRBCT_H
#define TEGRABL_BRBCT_H

#include <stddef.h>
#include <stdint.h>
#include <tegrabl_error.h>
#include <tegrabl_partition_manager.h>

/**
 * @brief Define the maximum copy of BR-BCT
 */
#define BR_BCT_MAX_COPIES 64U
/**
 * @brief Define the BR-BCT block size
 */
#define BR_BLOCK_SIZE (16 * 1024)

/* TODO: Converge mb1-bct and br-bct libraries into a single library */

/**
 * @brief Copy BR-BCT from sysram/iram to sdram.
 * Use the CARVEOUT_CPUBL_PARAMS in SDRAM.
 *
 * @param[in] sdram_brbct_location the SDRAM location
 * to which BR-BCT will be copied
 *
 * @return TEGRABL_NO_ERROR successful, otherwise appropriate error.
 */
tegrabl_error_t tegrabl_brbct_relocate_to_sdram(uint64_t sdram_brbct_location);


/**
 * @brief Get the size of BR-BCT.
 *
 * @return Non-zero value.
 */
uint32_t tegrabl_brbct_size(void);

/**
 * @brief Initialize BR-BCT library
 *
 * @param[in] load_address Address where BR-BCT is loaded.
 *
 * @return TEGRABL_NO_ERROR successful, otherwise TEGRABL_ERR_INVALID
 */
tegrabl_error_t tegrabl_brbct_init(uintptr_t load_address);

/**
 * @brief Get the BR-BCT address
 *
 * @return Address of BR-BCT, 0 in case library not initialized
 */
uintptr_t tegrabl_brbct_get(void);

/**
 * @brief Write multiple copies of BR-BCT to storage
 *
 * @param[in] buffer Input buffer.
 * @param[in] partition handle to BR-BCT partition.
 * @param[in] part_size Size of the BR-BCT partition
 * @param[in] bct_size Size of the BR-BCT buffer
 * @param[in] chunk_size Maximum transfer chunk size
 *
 * @return TEGRABL_NO_ERROR successful, otherwise appropriate error code
 */
tegrabl_error_t tegrabl_brbct_write_multiple(
	const void *buffer, struct tegrabl_partition *partition, uint64_t part_size,
	uint64_t bct_size, uint32_t chunk_size);

/**
 * @brief Get the partition table address value
 *
 * @return partition table address value in BR-BCT
 */
uintptr_t tegrabl_get_nvpt_offset(void);

/**
 * @brief Calculate partition table offset in BR-BCT
 *
 * @return partition table offset in BR-BCT
 */
uint32_t tegrabl_brbct_nvpt_offset(void);

/**
 * @brief Get customer data offset in BR-BCT
 *
 * @return offset of customer data
 */
uint32_t tegrabl_brbct_customerdata_offset(void);

/**
 * @brief Return customer data size in BR-BCT
 *
 * @return size of customer data
 */
uint32_t tegrabl_brbct_customerdata_size(void);

/**
 * @brief Copy current BR-BCT's customer data to new BR-BCT
 *
 * @param[in] new_bct start address of the new BR-BCT
 * @param[in] size size of new BR-BCT
 *
 * @return TEGRABL_NO_ERROR successful, otherwise appropriate error code
 */
tegrabl_error_t tegrabl_brbct_update_customer_data(uintptr_t new_bct, uint32_t size);

/**
 * @brief Verify the customer data field of given BR-BCT
 *
 * @param[in] brbct_addr Start address of BR-BCT
 *
 * @return TEGRABL_NO_ERROR customerdata is verified, otherwise appropriate error code
 */
tegrabl_error_t tegrabl_brbct_verify_customerdata(uintptr_t brbct_addr);

#endif /* TEGRABL_BRBCT_H */

