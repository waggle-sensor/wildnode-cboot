/*
 * Copyright (c) 2017, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#define MODULE TEGRABL_ERR_XUSBF

#include <tegrabl_io.h>
#include <tegrabl_xusbf_soc.h>


void tegrabl_xusbf_soc_fpga_config(void)
{
	NV_WRITE32(0x3550100, 0x9C);
	NV_WRITE32(0x3550104, 0x1ADD);
	NV_WRITE32(0x3550108, 0x1871);
	NV_WRITE32(0x355010C, 0x1E848);
	NV_WRITE32(0x3550110, 0x9C4);
	NV_WRITE32(0x3550114, 0xEA6);
	NV_WRITE32(0x3550118, 0x2DCB7);
	NV_WRITE32(0x355011C, 0x74);
	NV_WRITE32(0x3550120, 0x5B);
	NV_WRITE32(0x3550124, 0x98968);
	NV_WRITE32(0x3550128, 0x1E87);
	NV_WRITE32(0x355012C, 0xF444);
	NV_WRITE32(0x3550130, 0x1FE);
	NV_WRITE32(0x3550134, 0xC35);
	NV_WRITE32(0x355018C, 0x21);
	NV_WRITE32(0x3550190, 0x5B);
	NV_WRITE32(0x355019C, 0x0);
}

