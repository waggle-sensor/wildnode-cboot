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
#include <nvboot_bct.h>
#include <nvboot_version_defs.h>
#include <tegrabl_brbct.h>

#define CUSTOMERDATA_NVPT_OFFSET		16U

uint32_t tegrabl_brbct_size(void)
{
	return sizeof(NvBootConfigTable);
}

uint32_t tegrabl_brbct_nvpt_offset(void)
{
	return offsetof(NvBootConfigTable, CustomerData) + CUSTOMERDATA_NVPT_OFFSET;
}

uint32_t tegrabl_brbct_customerdata_offset(void)
{
	return offsetof(NvBootConfigTable, CustomerData);
}

uint32_t tegrabl_brbct_customerdata_size(void)
{
	return NVBOOT_BCT_CUSTOMER_DATA_SIZE;
}
