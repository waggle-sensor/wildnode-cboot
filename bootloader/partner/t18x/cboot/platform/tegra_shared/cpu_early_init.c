/*
 * Copyright (c) 2014, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited
 */

#include <boot_arg.h>

extern boot_arg *g_boot_params;

void __cpu_early_init(void);

void __cpu_early_init()
{
#if 0
	__asm__ volatile(
			"mrs x0, mpidr_el1\n"
			"tst x0, #0xf     \n"
			"bne .            \n"
			"mov %0, x5       \n"
//			: "=r" (g_boot_params)
	);
#endif
}
