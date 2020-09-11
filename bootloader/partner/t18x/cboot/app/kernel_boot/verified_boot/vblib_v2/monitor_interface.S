/*
 * Copyright (c) 2016-2018, NVIDIA Corporation. All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#include <asm.h>

/* Issue an SMC instruction to trap into the monitor.
 * The arguments are passed via registers x0-x2.
 * The first parameter refers to the appropriate SMC handler to invoke.
 * The following parameters follow the format expected by this SMC handler.
 *
 * uint32_t smc_call(uint32_t arg0, uintptr_t arg1, uintptr_t arg2)
 */
FUNCTION(smc_call)
	smc	#0
	ret
