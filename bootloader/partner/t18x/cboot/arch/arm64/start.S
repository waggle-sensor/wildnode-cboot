#
# Copyright (c) 2016-2018, NVIDIA CORPORATION.  All rights reserved.
#
# NVIDIA CORPORATION and its licensors retain all intellectual property
# and proprietary rights in and to this software and related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA CORPORATION is strictly prohibited.
#

#include <asm.h>
#include <tegrabl_addressmap.h>
#include <arscratch.h>
#include <platform/iomap.h>

#define RELOC_R_AARCH64_RELATIVE_TYPE	1027

.section .text.boot
.globl _start
FUNCTION(_start)
	b .L__real_start

	.balign 8
CpuBlParamsPtr:
   .dword (NV_ADDRESS_MAP_SCRATCH_BASE + SCRATCH_SCRATCH_7)
CpuBlParams:
   .dword 0x0
GicDistBase:
	.word 0x03881000
GicCpuBase:
	.word 0x03882000

.L__real_start:
	/* Update the .rela.dyn offsets */
	adr x4, _start
	ldr x5, =MEMBASE
	/* Calculate offset of load address relative to link address */
	sub x18, x4, x5
	adr x4, __reloc_begin
	adr x5, __reloc_end
	b reloc_point
	/* Add offsets to relocation table */
reloc_loop:
	/* Check if reloc type is R_AARCH64_RELATIVE*/
	ldr x0, [x4, #8]
	cmp w0, #RELOC_R_AARCH64_RELATIVE_TYPE
	b.ne incr_loop
	/* Load r_offset */
	ldr x0, [x4]
	add x0, x0, x18
	/* Load r_addend */
	ldr x1, [x4, #16]
	add x1, x1, x18
	/* Store addend at updated offset */
	str x1, [x0]
incr_loop:
	add x4, x4, #24
reloc_point:
	cmp x4, x5
	b.lo reloc_loop

	/* Read the scratch register and save the CPUBL-params pointer */
	ldr x10, CpuBlParamsPtr
	adr x11, CpuBlParams
	mov x19, xzr
	ldr w19, [x10]
	ldr w20, =0x80000000 /* Start of SDRAM address*/
	cmp x19, x20
	/*
	 * Absolute address is passed if it lies above start of DRAM address
	 * So skip restoring it
	 */
	bge skip_shift
	lsl x19, x19, #(CONFIG_PAGE_SIZE_LOG2)
skip_shift:
	str x19, [x11]

	mrs x0, CurrentEL
	cmp x0, #0xc
	bne .L__configure_el2

.L__configure_el3:
	mov x0, #0x30
	orr x0, x0, #(1 << 10)	/* Ensure 64bit EL2 */
	orr x0, x0, #(1 << 0)	/* Ensure non-secure lower EL */
	msr scr_el3, x0

	/* Disable coprocessor traps to EL3 */
	msr cptr_el3, xzr

	msr sctlr_el2, xzr

	/* If Secure-OS is loaded, skip GIC programming */
	ldr x20, [x19, #(CPUPARAMS_TOS_START_OFFSET)]
	cbnz x20, .L__configure_gic_done

	/* Configure GIC */
.L_configure_gic:
	adr x0, GicDistBase
	ldr w1, [x0]
	mrs x0, mpidr_el1
	tst x0, #0xf
	bne .L__configure_gic_dist_local
.L__configure_gic_dist:
	/* Enable Group0 and Group 1 interrupts */
	mov w0, #3
	str w0, [x1, #0x0] /* GICD_CTRL */
	add x2, x1, #0x84  /* GICD_IGROUP1 */
	/* Configure all SPIs as non-secure interrupts on core-0 */
	movn w3, #0
	str w3, [x2], #4
	str w3, [x2], #4
	str w3, [x2], #4
	str w3, [x2], #4
	str w3, [x2], #4
	str w3, [x2], #4
.L__configure_gic_dist_local:
	/* SGIs and PPIs for all cores are marked non-secure */
	str w3, [x1, #0x80]
.L__configure_gic_cpu:
	adr x0, GicCpuBase
	ldr w2, [x0]
	mov x0, #3
	str w0, [x2]	   /* GICC_CTRL */
	mov x0, #1 << 7    /* Allow NS access to GICC_PMR */
	str w0, [x2, #4]   /* GICC_PMR */
.L__configure_gic_done:

#if 0
	/* Jump to EL3 configure routine */
	bl nvtboot_cpu_tzinit
#endif

	/* Prepare for jump to SecureOS */
	ldr x10, CpuBlParams
	ldr x8, [x10, #(CPUPARAMS_TOS_START_OFFSET)]
	/* If TLK is not loaded (SecureOsAddr = 0) skip jump to SecureOS */
	cbz x8, .L__configure_el2

	mov x11, #(CPUPARAMS_TOS_PARAMS_OFFSET)
	add x11, x10, x11
	ldp x0, x1, [x11, #0x00]
	ldp x2, x3, [x11, #0x10]
	cbz x2, 1f
	/*
	 * Fill in non-secure world's entrypoint address; Tboot-BPMP stores
	 * a pointer in x2 that the secure monitor would use during its
	 * boot.
	 */
	adr x10, .L__real_start
	str x10, [x2]
1:
	blr x8

.L__configure_el2:
	mrs x0, CurrentEL
	cmp x0, #0x4
	blt .L__configure_el2_skip

	mov x0, #(1 << 31)		/* Ensure 64bit EL1 */
	orr x0, x0, #(1 << 27)  /* Route non-secure interrupts to EL2 */
	orr x0, x0, #(1 << 5)	/* Take async-aborts in EL2 */
	msr hcr_el2, x0
.L__configure_el2_skip:

	/* Allow FPU accesses */
	mov x0, #(3 << 20)
	msr cpacr_el1, x0

	/* Ensure we use exception stack */
	msr spsel, #1
	adr x0, __stack_end
	mov sp, x0

	/* clear bss */
.L__do_bss:
	/* NOTE: relies on __bss_start and __bss_end being 8 byte aligned */
	adr x0, __bss_start
	adr x1, __bss_end
	mov x2, #0
	sub x1, x1, x0
	cbz x1, 2f
1:
	sub x1, x1, #8
	str x2, [x0], #8
	cbnz x1, 1b
2:

#if WITH_CPU_EARLY_INIT
	/* call platform/arch/etc specific init code */
	bl __cpu_early_init
#endif

	bl	lk_main
	b	.

.ltorg

.section .bss.prebss.stack
	.align 8
DATA(__stack)
	.skip 0x2000
DATA(__stack_end)

