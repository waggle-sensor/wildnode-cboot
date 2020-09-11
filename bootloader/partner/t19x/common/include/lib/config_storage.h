/*
 * Copyright (c) 2018, NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */
#ifndef INCLUDED_CONFIG_STORAGE_H
#define INCLUDED_CONFIG_STORAGE_H

#include <tegrabl_error.h>
#include <tegrabl_compiler.h>
#include <tegrabl_storage_device_params.h>
#include <tegrabl_blockdev.h>

/**
 * @brief to get active boot device
 *
 * @return active boot device.
 */
tegrabl_storage_type_t mb2_get_boot_device(void);

/**
 * @brief Configure storage based on data in mb1 bct
 *
 * @param dev_param mb1 bct params for extracting conifg info
 * @param devices list of devices to initialize
 *
 * @return TEGRABL_NO_ERROR on success
 */
tegrabl_error_t config_storage(
		struct tegrabl_device_config_params *device_config,
		struct tegrabl_device *devices);

/**
 * @brief Implement storage_deinit to issue hibernate entry.
 *
 * @return TEGRABL_NO_ERROR on success
 */
tegrabl_error_t config_storage_deinit(void);

/**
 * @brief Initializes the specified device based on the device params and the instance provided.
 *
 * @param device_config specifies the device params
 * @param device_type specifies the storage type
 * @param instance specifies the instance of the device
 *
 * @return TEGRABL_NO_ERROR on success
 */
tegrabl_error_t init_storage_device(struct tegrabl_device_config_params *device_config,
									tegrabl_storage_type_t device_type,
									uint8_t instance);

#endif /*INCLUDED_CONFIG_STORAGE_H */


