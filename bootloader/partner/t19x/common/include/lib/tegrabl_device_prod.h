/*
 * Copyright (c) 2018, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#ifndef INCLUDE_TEGRABL_DEVICE_PROD
#define INCLUDE_TEGRABL_DEVICE_PROD

#include <stdint.h>
#include <tegrabl_error.h>
#include <tegrabl_module.h>

struct addr_range {
	uintptr_t lower;
	uintptr_t higher;
};

/**
* @brief registers device prod settings
*
* @param data Pointer to the device prod data
* @param size size of the device prod data in bytes
*
* @return returns TEGRABL_NO_ERROR if success, error code if fails.
*/
tegrabl_error_t tegrabl_device_prod_register(uintptr_t data, uint32_t size);

/**
* @brief configures the device prod of the given module and instance.
*
* @param module module id
* @param instance instance of the module
* @param range Pointer to the array of structures, which gives the address range list.
				Each structure points to one address range with lower address and higher address
*				Set NULL, NULL in the last structure.
*				If the address range is Lower1-Upper1, Lower2-Upper2, Lower3-Upper3
*				{lower1, upper1}, {lower2, upper2} {lower3, upper3}, {NULL, NULL}
* @param prod_name Pointer to the string, which specifies prod name
*				Ex: hs400, default, ddr52 for sdmmc
*					std, hs for i2c
*
* @return returns TEGRABL_NO_ERROR if success, error code if fails.
*/
tegrabl_error_t tegrabl_device_prod_configure(tegrabl_module_t module, uint8_t instance,
												struct addr_range *range, char *prod_name);

#endif /* INCLUDE_TEGRABL_DEVICE_PROD */
