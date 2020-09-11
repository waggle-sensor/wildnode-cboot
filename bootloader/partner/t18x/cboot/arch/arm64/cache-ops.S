/*
 * Copyright (c) 2014-2016, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited
 */

#include <asm.h>
#include <arch/ops.h>
#include <arch/defines.h>

#define SCTLR_M			 (0x1 << 0)
#define SCTLR_C			 (0x1 << 2)
#define SCTLR_I			 (0x1 << 12)

#if ARM64_WITH_EL2
#define SCTLR_ELx		sctlr_el2
#else
#define SCTLR_ELx		sctlr_el1
#endif

/* performs a data/unified operation by set/way */
.macro dcache_op_setway cacheop
	/* the following sequence is taken from DDI0487A_b_armv8_arm */
	mrs x6, clidr_el1
	and w3, w6, #0x7000000  /* get 2 x level of coherency */
	lsr w3, w3, #23
	cbz w3, 5f
	mov w10, wzr /* w10 = 2 * cache level */
	mov w14, #1 /* w14 = constant 0x1 */
1:
	add w2, w10, w10, lsr #1 /* calculate 3 * cache level */
	lsr w1, w6, w2  /* extract 3-bit cache type for this level */
	and w1, w1, #0x7
	cmp w1, w2
	blt 4f /* no data/unified cache at this level */
	msr csselr_el1, x10 /* select this cache level */
	isb
	mrs x1, ccsidr_el1 /* read cache geometry */
	and w2, w1, #7 /* w2 = log2(linelen)-4 */
	add w2, w2, #4 /* w2 = log2(linelen) */
	ubfx w4, w1, #3, #10 /* w4 = max way number, right aligned */
	clz w5, w4 /* w5 = 32-log2(ways). bit position of way in DC operand */
	lsl w9, w4, w5 /* w9 = may way number, aligned to position in DC operand */
	lsl w12, w14, w5 /* w12 = amount to decrement way number per iteration */
2:
	ubfx w7, w1, #13, #15 /* w7 = max set number, right aligned */
	lsl w7, w7, w2 /* w7 = max set number, aligned to position in dc operand */
	lsl w13, w14, w2 /* w13 = amount to decrement set number per iteration */
3:
	orr w11, w10, w9 /* w11 = combine way number and cache number ... */
	orr w11, w11, w7 /*  ... and set number for dc operand */
	dc \cacheop, x11 /* do dcache operation by set and way */
	subs w7, w7, w13 /* decrement set number */
	bge 3b
	subs x9, x9, x12 /* decrement way number */
	bge 2b
4:
	add w10, w10, #2  /* increment 2 * cache-level */
	cmp w3, w10
	dsb sy
	bgt 1b
5:
.endm

#if ARM_WITH_CACHE
/* void tegrabl_arch_disable_cache(uint flags); */
FUNCTION(tegrabl_arch_disable_cache)
	mrs x15, daif
	msr daifset, #3
	tst w0, #DCACHE
	beq .Licache_disable
.Ldcache_disable:
	mrs x0, SCTLR_ELx
	mov x9, #SCTLR_C
	bic x0, x0, x9
	msr SCTLR_ELx, x0

	/* Now clean+invalidate data/unified cache to PoC */
	dcache_op_setway cisw

.Licache_disable:
	tst w0, #ICACHE
	beq .Lcache_disable_done
	mrs x0, SCTLR_ELx
	mov x9, #SCTLR_I
	bic x0, x0, x9
	msr SCTLR_ELx, x0
	ic  iallu	   /* invalidate I-cache to PoU */
.Lcache_disable_done:
	msr daif, x15
	ret

/* void tegrabl_arch_enable_cache(uint flags); */
FUNCTION(tegrabl_arch_enable_cache)
	mrs x15, daif
	msr daifset, #3
	tst w0, #DCACHE
	beq .Licache_enable

.Ldcache_enable:
	/* First invalidate data/unified cache to PoC */
	dcache_op_setway isw

	mrs x0, SCTLR_ELx
	orr x0, x0, #SCTLR_C
	msr SCTLR_ELx, x0
.Licache_enable:
	tst w0, #ICACHE
	beq .Lcache_enable_done
	ic  iallu	   /* invalidate I-cache to PoU */
	mrs x0, SCTLR_ELx
	orr x0, x0, #SCTLR_I
	msr SCTLR_ELx, x0
.Lcache_enable_done:
	msr daif, x15
	ret

/* void tegrabl_arch_clean_dcache_range(addr_t start, size_t len); */
FUNCTION(tegrabl_arch_clean_dcache_range)
	mrs x15, daif
	msr daifset, #3
	add x2, x0, x1 /* calculate the end address */
	bic x0, x0, #(CACHE_LINE - 1) /* align the start with a cache line */
1:
	dc cvac, x0 /* clean cache to PoC by VA */
	add x0, x0, #CACHE_LINE
	cmp x0, x2
	blt 1b
	mov x0, xzr
	dsb sy
	msr daif, x15
	ret

/* void tegrabl_arch_invalidate_dcache_range(addr_t start, size_t len); */
FUNCTION(tegrabl_arch_invalidate_dcache_range)
	mrs x15, daif
	msr daifset, #3
	add x2, x0, x1 /* calculate the end address */
	bic x0, x0, #(CACHE_LINE - 1) /* align the start with a cache line */
1:
	dc ivac, x0 /* invalidate cache to PoC by VA */
	add x0, x0, #CACHE_LINE
	cmp x0, x2
	blt 1b
	mov x0, xzr
	dsb sy
	msr daif, x15
	ret

/* void tegrabl_arch_clean_invalidate_dcache_range(addr_t start, size_t len); */
FUNCTION(tegrabl_arch_clean_invalidate_dcache_range)
	mrs x15, daif
	msr daifset, #3
	add x2, x0, x1 /* calculate the end address */
	bic x0, x0, #(CACHE_LINE - 1) /* align the start with a cache line */
1:
	dc civac, x0 /* clean invalidate cache to PoC by VA */
	add x0, x0, #CACHE_LINE
	cmp x0, x2
	blt 1b
	mov x0, xzr
	dsb sy
	msr daif, x15
	ret

/* void tegrabl_arch_sync_dcache_range(addr_t start, size_t len); */
FUNCTION(tegrabl_arch_sync_dcache_range)
	mrs x15, daif
	msr daifset, #3
	add x2, x0, x1 /* calculate the end address */
	mov x4, x2
	bic x0, x0, #(CACHE_LINE - 1) /* align the start with a cache line */
	mov x3, x0
1:
	dc cvac, x0 /* clean d-cache to PoC by VA */
	add x0, x0, #CACHE_LINE
	cmp x0, x2
	blt 1b
	dsb sy
	mov x0, x3
	mov x2, x4
1:
	ic ivau, x0 /* invalidate i-cache to PoU by VA */
	add x0, x0, #CACHE_LINE
	cmp x0, x2
	blt 1b
	isb
	mov x0, xzr
	msr daif, x15
	ret
#else
FUNCTION(tegrabl_arch_disable_cache)
	ret

FUNCTION(tegrabl_arch_enable_cache)
	ret

FUNCTION(tegrabl_arch_clean_dcache_range)
	ret

FUNCTION(tegrabl_arch_invalidate_dcache_range)
	ret

FUNCTION(tegrabl_arch_clean_invalidate_dcache_range)
	ret

FUNCTION(tegrabl_arch_sync_dcache_range)
	ret
#endif
