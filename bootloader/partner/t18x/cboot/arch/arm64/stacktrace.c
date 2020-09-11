/*
 * Copyright (c) 2014-2015, NVIDIA CORPORATION. All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include <debug.h>
#include <arch/arm64.h>
#include <reg.h>

/*
 * This is how the stack should look like as per AArch64 PCS:-
 *
 *   stack_top =>  ___________________________
 *                |                           |
 *                |          ......           |
 *                |          ......           |
 *                |          ......           |
 *                |          ......           |
 * previous-SP => |___________________________|
 *                |                           |
 *                |   local variables, etc    |
 *                |___________________________|
 *                |                           |
 *                |   return address (LR)     | <= 8 bytes
 *                |___________________________|
 *                |                           |
 *                |        previous FP        | <= 8 bytes
 *  SP = FP =>    |___________________________|
 *                |                           |
 *                |          ......           |
 *                |          ......           |
 *                |          ......           |
 *                |          ......           |
 * stack_bottom =>|___________________________|
 *
 *
 *  frame-pointer (FP) is stored in x29,
 *  link-register (LR) is stored in x30
 */

extern void initial_thread_func(void);

void print_stacktrace(struct arm64_iframe_long *iframe)
{
	unsigned long fp, sp, pc;
	unsigned long stack_size;

	fp = iframe->r[29];
	sp = iframe->r[31];
	pc = iframe->elr;
	stack_size = sp;

	while (1) {
		dprintf(CRITICAL, "=> pc:0x%08lX, sp:0x%08lX\n", pc, sp);

		if (!fp || fp < sp || fp & 0xf)
			break;

		sp = fp;
		pc = readl(fp+8); /* LR = PC for the first function in thread */
		if (pc != (unsigned long)initial_thread_func)
			pc = readl(fp+8) - 4; /* LR = PC at function-call + 4 */
		fp = readl(fp);
	}

	stack_size = sp - stack_size;
	if (stack_size >= ARCH_DEFAULT_STACK_SIZE)
		dprintf(CRITICAL, "\n Using %lu bytes - STACK OVERFLOW !!!\n",
				stack_size);
}

