/*
 * Copyright (c) 2017-2018, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited
 */

#ifndef __FASTBOOT_A_B_H_
#define __FASTBOOT_A_B_H_

#include <tegrabl_error.h>

/**
 * @brief Get the active boot slot and return the suffix
 *
 * @param slot_suffix slot_suffix string within double quotations (output param)
 *
 * @return TEGRABL_NO_ERROR on success. Otherwise, return appropriate error code
 */
tegrabl_error_t fastboot_a_b_get_current_slot(char *slot_suffix);

/**
 * @brief Get the active boot slot index
 *
 * @param slot_id active slot index (output param)
 *
 * @return TEGRABL_NO_ERROR on success. Otherwise, return appropriate error code
 */
tegrabl_error_t fastboot_a_b_get_current_slot_id(uint32_t *slot_id);

/**
 * @brief Get the slot number
 *
 * @param slot_number number of slots (output param)
 *
 * @return TEGRABL_NO_ERROR on success. Otherwise, return appropriate error code
 */
tegrabl_error_t fastboot_a_b_get_slot_num(uint8_t *slot_num);

/**
 * @brief Get slot suffixes base on slot index
 *
 * @param slot_id slot index
 * @param slot_suffix slot suffix string (output param)
 *
 * @return TEGRABL_NO_ERROR on success. Otherwise, return appropriate error code
 */
tegrabl_error_t fastboot_a_b_get_slot_suffix(uint8_t slot_id,
											 char *slot_suffix);

/**
 * @brief Check if given slot is verified to boot successful
 *
 * @param slot given slot suffix
 * @param state slot boot successful flag (output param)
 *
 * @return TEGRABL_NO_ERROR on success. Otherwise, return appropriate error code
 */
tegrabl_error_t fastboot_a_b_is_slot_successful(const char *slot, bool *state);

/**
 * @brief Check if given slot is unable to boot
 *
 * @param slot given slot suffix
 * @param state if slot is unbootable (output param)
 *
 * @return TEGRABL_NO_ERROR on success. Otherwise, return appropriate error code
 */
tegrabl_error_t fastboot_a_b_is_slot_unbootable(const char *slot, bool *state);

/**
 * @brief Get retry count of given slot
 *
 * @param slot given slot suffix
 * @param count retry count remained (output param)
 *
 * @return TEGRABL_NO_ERROR on success. Otherwise, return appropriate error code
 */
tegrabl_error_t fastboot_a_b_get_slot_retry_count(const char *slot,
												  uint8_t *count);

/**
 * @brief Set retry count of given slot
 *
 * @param slot_id given slot index
 * @param retry_count retry count
 *
 * @return TEGRABL_NO_ERROR on success. Otherwise, return appropriate error code
 */
tegrabl_error_t fastboot_a_b_set_slot_retry_count(uint32_t slot_id,
												  uint8_t retry_count);

/**
 * @brief Mark given slot active
 *
 * @param slot given slot suffix
 *
 * @return TEGRABL_NO_ERROR on success. Otherwise, return appropriate error code
 */
tegrabl_error_t fastboot_a_b_slot_set_active(const char *slot);

/**
 * @brief Check if given partition always need to update all slots during OTA
 *
 * @param part_name partition name
 *
 * @return true if given partition need to update all slots during update
 */
bool fastboot_a_b_is_always_ab_partition(const char *part_name);

#endif /* TEGRABL_FASTBOOT_A_B_H */
