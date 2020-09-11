/*
 * Copyright (c) 2014 Travis Geiselbrecht
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * Copyright (c) 2016, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited
 */

#include <stdio.h>
#include <debug.h>
#include <arch/arm64.h>

#define SHUTDOWN_ON_FATAL 1

void print_stacktrace(struct arm64_iframe_long *iframe);

static void dump_iframe(const struct arm64_iframe_long *iframe)
{
	printf("iframe %p:\n", iframe);
	printf("x0  0x%16" PRIx64" x1  0x%16" PRIx64" x2  0x%16" PRIx64" x3  0x%16" PRIx64"\n",
		   iframe->r[0], iframe->r[1], iframe->r[2], iframe->r[3]);
	printf("x4  0x%16" PRIx64" x5  0x%16" PRIx64" x6  0x%16" PRIx64" x7  0x%16" PRIx64"\n",
		   iframe->r[4], iframe->r[5], iframe->r[6], iframe->r[7]);
	printf("x8  0x%16" PRIx64" x9  0x%16" PRIx64" x10 0x%16" PRIx64" x11 0x%16" PRIx64"\n",
		   iframe->r[8], iframe->r[9], iframe->r[10], iframe->r[11]);
	printf("x12 0x%16" PRIx64" x13 0x%16" PRIx64" x14 0x%16" PRIx64" x15 0x%16" PRIx64"\n",
		   iframe->r[12], iframe->r[13], iframe->r[14], iframe->r[15]);
	printf("x16 0x%16" PRIx64" x17 0x%16" PRIx64" x18 0x%16" PRIx64" x19 0x%16" PRIx64"\n",
		   iframe->r[16], iframe->r[17], iframe->r[18], iframe->r[19]);
	printf("x20 0x%16" PRIx64" x21 0x%16" PRIx64" x22 0x%16" PRIx64" x23 0x%16" PRIx64"\n",
		   iframe->r[20], iframe->r[21], iframe->r[22], iframe->r[23]);
	printf("x24 0x%16" PRIx64" x25 0x%16" PRIx64" x26 0x%16" PRIx64" x27 0x%16" PRIx64"\n",
		   iframe->r[24], iframe->r[25], iframe->r[26], iframe->r[27]);
	printf("x28 0x%16" PRIx64" x29 0x%16" PRIx64" lr  0x%16" PRIx64" sp  0x%16" PRIx64"\n",
		   iframe->r[28], iframe->r[29], iframe->r[30], iframe->r[31]);
	printf("elr 0x%16" PRIx64"\n", iframe->elr);
	printf("spsr 0x%16" PRIx64"\n", iframe->spsr);

}

static void dump_esr(void)
{
	uint32_t esr = ARM64_READ_TARGET_SYSREG(ESR_ELx);
	uint32_t ec = esr >> 26;
	uint32_t il = (esr >> 25) & 0x1;
	uint32_t iss = esr & ((1<<24) - 1);
	uint64_t far = ARM64_READ_TARGET_SYSREG(FAR_ELx);

	switch (ec) {
		case 0x0:
			printf("UNKNOWN EXCEPTION\n");
			break;
		case 0x7:
			printf("ACCESS TO ASIMD/FPU\n");
			break;
		case 0xE:
			printf("ILLEGAL INSTRUCTION SET\n");
			break;
		case 0x15:
			printf("SYSCALL\n");
			break;
		case 0x20:
			printf("INSTRUCTION ABORT FROM LOWER EL\n");
			break;
		case 0x21:
			printf("INSTRUCTION ABORT\n");
			break;
		case 0x22:
			printf("PC ALIGNMENT\n");
			break;
		case 0x24:
			printf("DATA ABORT FROM LOWER EL\n");
			break;
		case 0x25:
			printf("DATA ABORT (FAR: %" PRIx64")\n", far);
			break;
		case 0x26:
			printf("SP ALIGNMENT\n");
			break;
		case 0x28:
			printf("FP EXCEPTION\n");
			break;
		case 0x2F:
			printf("SERROR\n");
			break;
		return;
	}
	printf("-----------------------------------------------\n");
	if (ec == 0x25) {
#if ARM64_WITH_EL2
		asm volatile ("at s1e2w, %0" : : "r"(far));
#else
		asm volatile ("at s1e1w, %0" : : "r"(far));
#endif
		printf("PAR_ELX: 0x%" PRIx64"\n", ARM64_READ_SYSREG(PAR_EL1));
	}
	printf("\nESR 0x%x: ec 0x%x, il 0x%x, iss 0x%x\n", esr, ec, il, iss);
}

void arm64_sync_exception(struct arm64_iframe_long *iframe)
{
	printf("\n-----------------------------------------------\n");
	printf("Synchronous Exception: ");
	dump_esr();
	printf("-----------------------------------------------\n");
	printf(" [Stack Trace]\n\n");
	print_stacktrace(iframe);
	printf("-----------------------------------------------\n");
	dump_iframe(iframe);
	printf("-----------------------------------------------\n");
	panic("die\n");
}

void arm64_invalid_exception(struct arm64_iframe_long *iframe, unsigned int which)
{
	printf("-----------------------------------------------\n");
	printf("invalid exception, which 0x%x\n", which);
	dump_esr();
	printf("-----------------------------------------------\n");
	dump_iframe(iframe);
	printf("-----------------------------------------------\n");

	panic("die\n");
}



