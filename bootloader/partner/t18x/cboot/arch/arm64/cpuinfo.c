/*
 * Copyright (c) 2014, NVIDIA CORPORATION. All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include <debug.h>
#include <arch.h>
#include <arch/ops.h>
#include <arch/arm64.h>

struct cpu_id {
	uint32_t midr;  /* Main ID register of CPU */
	char *name;     /* Name of the CPU */
};

static struct cpu_id cpu_id_table[] = {
	{
		0x4e0f0000,
		"Nvidia Denver",
	},
	{
		0x410fd030,
		"ARM Cortex A53",
	},
	{
		0x410fd070,
		"ARM Cortex A57",
	},
	{
		0x0,
		NULL,
	},
};

void print_cpuid(void)
{
	uint32_t midr, mpidr;
	struct cpu_id *cpu;

	mpidr = (uint32_t)ARM64_READ_SYSREG(MPIDR_EL1);
	midr = (uint32_t)ARM64_READ_SYSREG(MIDR_EL1);
	for (cpu = cpu_id_table; cpu->midr != 0x0; cpu++) {
		/* Mask the major and minor version number fields of MIDR and compare */
		if (cpu->midr == (midr & 0xFF8FFFF0)) {
			dprintf(SPEW, "CPU: %s\n", cpu->name);
			break;
		}
	}
	dprintf(SPEW, "CPU: MIDR: 0x%08X, MPIDR: 0x%08X\n", midr, mpidr);
}

