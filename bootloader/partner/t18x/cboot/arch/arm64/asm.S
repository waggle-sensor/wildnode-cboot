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
#include <asm.h>

.macro push ra, rb
stp \ra, \rb, [sp,#-16]!
.endm

.macro pop ra, rb
ldp \ra, \rb, [sp], #16
.endm

/* void arm64_context_switch(vaddr_t *old_sp, vaddr_t new_sp); */
FUNCTION(arm64_context_switch)
    /* save old frame */
    push d14, d15
    push d12, d13
    push d10, d11
    push d8, d9

    push x28, x29
    push x26, x27
    push x24, x25
    push x22, x23
    push x20, x21
    push x18, x19
    str  x30, [sp,#-8]!

    /* save old sp */
    mov  x15, sp
    str  x15, [x0]

    /* load new sp */
    mov  sp, x1

    /* restore new frame */
    ldr  x30, [sp], #8
    pop  x18, x19
    pop  x20, x21
    pop  x22, x23
    pop  x24, x25
    pop  x26, x27
    pop  x28, x29

    pop  d8, d9
    pop  d10, d11
    pop  d12, d13
    pop  d14, d15

    ret

FUNCTION(arm64_el3_to_el1)
    mrs x0, scr_el3
    orr x0, x0, #(1<<10) /* Set RW bit (64bit lower-el) */
    msr scr_el3, x0

    /* set EL1 to 64bit */
    mov x0, #(1<<31)
    msr hcr_el2, x0

    /* disable EL2 coprocessor traps */
    mov x0, #0x33ff
    msr cptr_el2, x0

    /* disable EL1 FPU traps */
    mov x0, #(0b11<<20)
    msr cpacr_el1, x0

    /* set up the EL1 bounce interrupt */
    mov x0, sp
    msr sp_el1, x0

    adr x0, .Ltarget_el1
    msr elr_el3, x0

    mov x0, #((0b1111 << 6) | (0b0101)) /* EL1h runlevel */
    msr spsr_el3, x0
    isb

    eret

.Ltarget_el1:
    ret

FUNCTION(arm64_el3_to_el2)
	msr sctlr_el2, xzr

	mov x0, #0x30
    orr x0, x0, #(1<<10) /* Set RW bit (64bit lower-el) */
    orr x0, x0, #(1<<0)  /* Set NS bit */
    msr scr_el3, x0

    /* disable coprocessor traps to EL3 */
    msr cptr_el3, xzr

    /* disable EL1 FPU traps */
    mov x0, #(0b11<<20)
    msr cpacr_el1, x0

    /* set up the EL2 bounce interrupt */
    mov x0, sp
    msr sp_el2, x0

    adr x0, .Ltarget_el2
    msr elr_el3, x0

    mov x0, #((0b1111 << 6) | (0b1001)) /* EL2h runlevel */
    msr spsr_el3, x0
    isb

    eret

.Ltarget_el2:
    ret

/*
 * Reference: ARM SMC Calling convention (ARM DEN 0028A)
 *
 * void arm64_send_smc(struct arm64_smc_regs *regs)
 */
FUNCTION(arm64_send_smc)
	sub sp, sp, #16
	/* Only x19 is used as scratch register, but need to keep sp aligned to
	 * 16-bytes across function-calls */
	stp x19, x20, [sp]
	/* Use x19 as scratch register to store 'regs' pointer as this is
	 * guaranteed to be preserved across SMC calls */
	mov x19, x0
	/* Load parameters (func-id in x0 and params in x1..x6) */
	ldp x0, x1, [x19, #00]
	ldp x2, x3, [x19, #16]
	ldp x4, x5, [x19, #32]
	ldr x6, [x19, #48]
	isb
	smc #0
	/* Restore result (4 registers) */
	stp x0, x1, [x19, #00]
	stp x2, x3, [x19, #16]
	ldp x19, x20, [sp], #16
	ret
