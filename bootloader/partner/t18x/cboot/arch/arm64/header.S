/*
 * Copyright (c) 2014 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

/* Make sure that .cbootheader is in sync with nvtegra_bl_header.h */
.section .cbootheader
header_start:
reserved:
	.skip 4,0x00
magic:
	.asciz "EBT"
length_insecure:
	.skip 8,0x11
/* Load_address should be equal to CBOOT_LOAD_ADDRESS from
   $TEGRA_TOP/nvtboot/platform/t210/nvtboot_sdram_usage.h */
load_address_insecure:
	.dword cboot_start
key_modulus:
	.skip 256,0xaa
signature:
	.skip 272,0xee
padding:
	.skip 8,0x00
random_aes:
	.skip 16,0xcc
length_secure:
	.skip 8,0xee
/* Load_address should be equal to CBOOT_LOAD_ADDRESS from
   $TEGRA_TOP/nvtboot/platform/t210/nvtboot_sdram_usage.h */
load_address_secure:
	.dword cboot_start
/* Entry_point should be equal to load_address + sizeof this header (0x258) */
entry_point:
	.dword header_end - header_start + cboot_start
header_end:
