/*
 * Copyright (c) 2014 Travis Geiselbrecht
 * Copyright (c) 2016, NVIDIA CORPORATION.  All rights reserved.
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
#pragma once

#include <stdbool.h>
#include <sys/types.h>
#include <compiler.h>

__BEGIN_CDECLS

#define DSB __asm__ volatile("dsb sy" ::: "memory")
#define ISB __asm__ volatile("isb" ::: "memory")

#define ARM64_READ_SYSREG(reg) \
({ \
    uint64_t _val; \
    __asm__ volatile("mrs %0," #reg : "=r" (_val)); \
    _val; \
})

#define ARM64_WRITE_SYSREG(reg, val) \
({ \
    __asm__ volatile("msr " #reg ", %0" :: "r" (val)); \
    ISB; \
})

#if ARM64_WITH_EL2
#define SCTLR_ELx	sctlr_el2
#define TCR_ELx		tcr_el2
#define TTBR0_ELx	ttbr0_el2
#define MAIR_ELx	mair_el2
#define ESR_ELx		esr_el2
#define ELR_ELx		elr_el2
#define FAR_ELx		far_el2
#define VBAR_ELx	vbar_el2
#else
#define SCTLR_ELx	sctlr_el1
#define TCR_ELx		tcr_el1
#define TTBR0_ELx	ttbr0_el1
#define MAIR_ELx	mair_el1
#define ESR_ELx		esr_el1
#define ELR_ELx		elr_el1
#define FAR_ELx		far_el1
#define VBAR_ELx	vbar_el1
#endif

/* Following macros are meant to be used with one of the _ELx macros
 * defined above */
#define ARM64_READ_TARGET_SYSREG(r)		ARM64_READ_SYSREG(r)
#define ARM64_WRITE_TARGET_SYSREG(r, v)	ARM64_WRITE_SYSREG(r, v)

static inline uint32_t arm64_get_cpu_idx(void)
{
#if !(defined(MAX_CPU_CLUSTERS) && defined(MAX_CPUS_PER_CLUSTER))
	return 0;
#else
	uint64_t mpidr = 0;
	uint32_t cpu_num = 0;

	mpidr = ARM64_READ_SYSREG(MPIDR_EL1);

	/* Get the MPIDR[AFF0] */
	cpu_num = mpidr & (MAX_CPUS_PER_CLUSTER - 1);
	/* Offset based on MPIDR[AFF1] */
	cpu_num += (((mpidr) >> 8) & (MAX_CPU_CLUSTERS - 1)) * MAX_CPUS_PER_CLUSTER;

	return cpu_num;
#endif
}

void arm64_context_switch(vaddr_t *old_sp, vaddr_t new_sp);

/* exception handling */
struct arm64_iframe_long {
    uint64_t r[32];
    uint64_t elr;
    uint64_t spsr;
    uint64_t vfp[64];
    uint64_t fpcr;
    uint64_t fpsr;
};

struct arm64_iframe_short {
    uint64_t r[20];
    uint64_t elr;
    uint64_t spsr;
    uint64_t vfp[48];
    uint64_t fpcr;
    uint64_t fpsr;
};

static inline void arm64_enable_serror(void)
{
    CF;
    __asm__ volatile("msr daifclr, #4" ::: "memory");
}

static inline void arm64_disable_serror(void)
{
    __asm__ volatile("msr daifset, #4" ::: "memory");
    CF;
}

extern void arm64_exception_base(void);
void arm64_el3_to_el2(void);
void arm64_el3_to_el1(void);

__END_CDECLS

