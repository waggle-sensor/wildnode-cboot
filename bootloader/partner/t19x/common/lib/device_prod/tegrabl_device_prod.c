/*
 * Copyright (c) 2018-2019, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#define MODULE TEGRABL_ERR_DEVICE_PROD

#include "build_config.h"
#include <stddef.h>
#include <stdint.h>
#include <tegrabl_error.h>
#include <tegrabl_module.h>
#include <tegrabl_io.h>
#include <tegrabl_debug.h>
#include <tegrabl_device_prod.h>
#include <string.h>
#include <tegrabl_error.h>

#define AUX_INFO_DEVICE_PROD_PARSE			1U
#define AUX_INFO_DEVICE_PROD_REGISTER		2U
#define AUX_INFO_DEVICE_PROD_GET_INFO		3U

static uintptr_t device_prod_data;
static uint32_t device_prod_size;
static bool is_registered;

tegrabl_error_t tegrabl_device_prod_register(uintptr_t data, uint32_t size)
{
	tegrabl_error_t error = TEGRABL_NO_ERROR;

	if ((data == 0UL) || (size == 0UL)) {
		error = TEGRABL_ERROR(TEGRABL_ERR_INVALID, AUX_INFO_DEVICE_PROD_REGISTER);
		TEGRABL_SET_ERROR_STRING(error, "data = %lx, size = %lx", data, size);
		goto fail;
	}
	device_prod_data = data;
	device_prod_size = size;
	is_registered = true;

fail:
	return error;
}

static tegrabl_error_t device_prod_get_info(uintptr_t *data, uint32_t *size)
{
	tegrabl_error_t error = TEGRABL_NO_ERROR;

	if (!is_registered) {
		error = TEGRABL_ERROR(TEGRABL_ERR_INVALID, AUX_INFO_DEVICE_PROD_GET_INFO);
		goto fail;
	}

	*data = device_prod_data;
	*size = device_prod_size;

fail:
	return error;
}

tegrabl_error_t tegrabl_device_prod_configure(tegrabl_module_t module, uint8_t instance,
												struct addr_range *range, char *prod_name)
{
	tegrabl_error_t error;
	uint32_t *pbase_addr, *pbuf_prod_data, *pbuf_prod_val;
	char *pbuf_prod_names;
	uint32_t prod_id, offset_prod_names, len_prod_names;
	uint32_t data, local_module, local_instance, offset_prod_data, offset_prod_val, count_modules;
	uint32_t local_prod_id, num_of_prods, num_of_entries;
	uint32_t addr, mask, val;
	bool found;
	uint32_t module_idx, entry_idx, prod_idx;
	uint32_t *pdata;
	uintptr_t pbase;
	uint32_t len;
	struct addr_range *addr_map;
	uint32_t names_list_idx = 0UL;
	uint32_t name_idx = 0UL;
	const char *prod_names_list;

	/* get device prod info base address */
	error = device_prod_get_info(&pbase, &len);
	if (error != TEGRABL_NO_ERROR) {
		goto fail;
	}

	pbase_addr = (uint32_t *)pbase;

	/* get prod names list offset and prod names count */
	data = *(pbase_addr + 1UL);
	offset_prod_names = data >> 16;
	len_prod_names = data & 0xFFFFUL;

	/* get prod names address */
	pbuf_prod_names = (char *)(pbase_addr + offset_prod_names);

	/* get module count */
	data = *pbase_addr;
	count_modules = (data  >> 16) & 0xFFFFUL;

	/* check if the given module and instance is present */
	offset_prod_data = 0UL;
	for (module_idx = 0UL; module_idx < count_modules; module_idx++) {
		data = *(pbase_addr + 2UL + module_idx);
		local_module = (data >> 24) & 0xFFUL;
		local_instance = (data >> 16) & 0xFFUL;
		if ((local_module == module) && (local_instance == instance)) {
			offset_prod_data = data & 0xFFFFUL;
			break;
		}
	}

	if (module_idx >= count_modules) {
		TEGRABL_PRINT_WARN_STRING(TEGRABL_ERR_NOT_FOUND, "module = %u, instance = %u", "device prod", module,
			instance);
		goto fail;
	}

	/* check given prod name is present */
	prod_id = 1UL;
	names_list_idx = 0UL;
	name_idx = 0UL;
	prod_names_list = pbuf_prod_names;
	found = false;

	for (names_list_idx = 0; names_list_idx < len_prod_names; names_list_idx++) {
		/* Reached the end of current prod in prod_names_list */
		if (prod_names_list[names_list_idx] == '$') {
			if (prod_name[name_idx] == '\0') {
				/* it's a complete match and reached the end of prod_name as well */
				found = true;
				break;
			} else {
				/* mismatch, so start search/compare from next prod name in the list */
				name_idx = 0UL;
				prod_id++;
			}
		} else if (prod_names_list[names_list_idx] != prod_name[name_idx]) {
			/* mismatch in between, so skip over till the end of current prod in prod_names_list */
			while ((prod_names_list[names_list_idx + 1UL] != '$') && (names_list_idx < len_prod_names)) {
				names_list_idx++;
			}
			names_list_idx++;
			name_idx = 0UL;
			prod_id++;
		}
		else {
			name_idx++;
		}
	}

	if (!found) {
		TEGRABL_PRINT_WARN_STRING(TEGRABL_ERR_NOT_FOUND, "prod name = %s", "device prod", prod_name);
		goto fail;
	}

	pbuf_prod_data = pbase_addr + offset_prod_data;
	num_of_prods = *pbuf_prod_data++;

	/* check module and instance with prod name is present */
	local_prod_id = 0UL;
	found = false;
	for (prod_idx = 0UL; prod_idx < num_of_prods; prod_idx++) {
		data = *pbuf_prod_data;
		local_prod_id =  data >> 16;
		if (prod_id == local_prod_id) {
			found = true;
			break;
		}
		pbuf_prod_data += 2UL;
	}

	if (!found) {
		TEGRABL_PRINT_WARN_STRING(TEGRABL_ERR_NOT_FOUND, "module %u:%u:%s", module, instance, prod_name,
									"device prod");
		goto fail;
	}

	data = *pbuf_prod_data;
	num_of_entries = data & 0xFFFFUL;
	offset_prod_val = *(pbuf_prod_data + 1UL);

	/* get prod address and check if it is withing allowed range */
	pbuf_prod_val = pbase_addr + offset_prod_val;
	for (entry_idx = 0UL; entry_idx < num_of_entries; entry_idx++) {
		addr = *pbuf_prod_val;
		addr_map = range;
		while ((addr_map->higher != 0UL) && (addr_map->lower != 0UL)) {
			if ((addr >= addr_map->higher) || (addr < addr_map->lower)) {
				error = TEGRABL_ERROR(TEGRABL_ERR_INVALID, AUX_INFO_DEVICE_PROD_PARSE);
				TEGRABL_SET_ERROR_STRING(error, "upper = %x, lower = %x, addr = %x", addr_map->higher,
						addr_map->lower);
				goto fail;
			}
			addr_map++;
		}
		pbuf_prod_val += 3UL;
	}

	/* get prod address, mask and value. then prepare and set the value */
	pbuf_prod_val = pbase_addr + offset_prod_val;
	for (entry_idx = 0UL; entry_idx < num_of_entries; entry_idx++) {
		addr = *pbuf_prod_val++;
		mask = *pbuf_prod_val++;
		val = *pbuf_prod_val++;

		pdata = (uint32_t *)(uintptr_t)addr;
		*pdata = (*pdata & ~mask) | (mask & val);
		pr_trace("%u.%s.0x%08x.0x%08x = 0x%08x\n", instance, prod_name, addr, mask, val);
	}

fail:
	return error;
}
