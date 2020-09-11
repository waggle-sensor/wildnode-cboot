/*
 * Copyright (c) 2015-2018, NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#define MODULE TEGRABL_ERR_BRBIT
#define NVBOOT_TARGET_FPGA 0

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <tegrabl_utils.h>
#include <tegrabl_debug.h>
#include <tegrabl_brbit.h>
#include <tegrabl_brbit_core.h>
#include <tegrabl_addressmap.h>
#include <nvboot_bit.h>
#include <nvboot_config.h>
#include <nvboot_version_defs.h>
#include <tegrabl_brbit_err_aux.h>

#define TEGRABL_CHIP 0x19UL

#define sizeoff(st, m) ((int)(sizeof((st *)0)->m));

void *tegrabl_brbit_location(void)
{
	uint64_t address = 0;

	address = NVBOOT_BIT_START;

	return (void *)(intptr_t) address;
}

bool tegrabl_brbit_verify(void *buffer)
{
	NvBootInfoTable *boot_info = (NvBootInfoTable *)buffer;

	if (((boot_info->BootRomVersion == NVBOOT_VERSION(TEGRABL_CHIP, 0x01U)) ||
		 (boot_info->BootRomVersion == NVBOOT_VERSION(TEGRABL_CHIP, 0x02U))) &&
		 (boot_info->DataVersion    == NVBOOT_VERSION(TEGRABL_CHIP, 0x01U)) &&
		 (boot_info->RcmVersion     == NVBOOT_VERSION(TEGRABL_CHIP, 0x01U)) &&
		 (boot_info->PrimaryDevice  == NvBootDevType_Irom)) {
		return true;
	}

	return false;
}

tegrabl_error_t tegrabl_brbit_get_offset_size(tegrabl_brbit_data_type_t type,
		uint32_t instance, uint32_t *offset, uint32_t *size)
{
	tegrabl_error_t error = TEGRABL_NO_ERROR;

	TEGRABL_UNUSED(instance);

	if ((offset == NULL) || (size == NULL)) {
		error = TEGRABL_ERROR(TEGRABL_ERR_INVALID, TEGRABL_BRBIT_GET_OFFSET_SIZE_1);
		TEGRABL_SET_ERROR_STRING(error, "offset = %p, size = %p\n", offset, size);
		goto fail;
	}

	switch (type) {
	case TEGRABL_BRBIT_DATA_BRBIT:
		*offset = 0;
		*size = sizeof(NvBootInfoTable);
		break;
	case TEGRABL_BRBIT_DATA_BOOT_TYPE:
		*offset = offsetof(NvBootInfoTable, BootType);
		*size = sizeoff(NvBootInfoTable, BootType);
		break;
	case TEGRABL_BRBIT_DATA_SAFE_START_ADDRESS:
		*offset = offsetof(NvBootInfoTable, SafeStartAddr);
		*size = sizeoff(NvBootInfoTable, SafeStartAddr);
		break;
	case TEGRABL_BRBIT_DATA_ACTIVE_BCT_PTR:
		*offset = offsetof(NvBootInfoTable, BctPtr);
		*size = sizeoff(NvBootInfoTable, BctPtr);
		break;
	case TEGRABL_BRBIT_DATA_BCT_SIZE:
		*offset = offsetof(NvBootInfoTable, BctSize);
		*size = sizeoff(NvBootInfoTable, BctSize);
		break;
	default:
		error = TEGRABL_ERROR(TEGRABL_ERR_NOT_SUPPORTED, TEGRABL_BRBIT_GET_OFFSET_SIZE_1);
		TEGRABL_SET_ERROR_STRING(error, "Type %d", type);
		break;
	}

fail:
	return error;
}

