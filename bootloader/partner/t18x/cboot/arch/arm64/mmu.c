/*
 * Copyright (c) 2014-2018, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited
 */

#include <debug.h>
#include <trace.h>
#include <assert.h>
#include <string.h>
#include <compiler.h>
#include <sys/types.h>
#include <kernel/thread.h>
#include <arch.h>
#include <arch/mmu.h>
#include <arch/arm64.h>
#include <arch/defines.h>
#include <tegrabl_carveout_id.h>
#include <tegrabl_addressmap.h>
#include <tegrabl_cpubl_params.h>
#include <tegrabl_io.h>
#include <arscratch.h>

#undef ICACHE
#undef DCACHE
#undef UCACHE

#include <tegrabl_cache.h>

#define LOCAL_TRACE  0

#define KB (1024LU)
#define MB (1024*KB)
#define GB (1024*MB)
#define TB (1024*GB)

#define VA_SIZE_BITS 39
#define PA_SIZE_BITS 40  /* 1 TB */

#if PAGE_SIZE == (4 * KB)

#define PAGE_LEVELS 3
#define LEVEL0_PAGE_SHIFT 39
#define LEVEL1_PAGE_SHIFT 30
#define LEVEL2_PAGE_SHIFT 21
#define LEVEL3_PAGE_SHIFT 12
#define FIRST_LEVEL 1
#define LAST_LEVEL 3

#elif PAGE_SIZE == (16 * KB)

#define PAGE_LEVELS 3
#define LEVEL0_PAGE_SHIFT 47
#define LEVEL1_PAGE_SHIFT 36
#define LEVEL2_PAGE_SHIFT 25
#define LEVEL3_PAGE_SHIFT 14
#define FIRST_LEVEL 1
#define LAST_LEVEL 3

#elif PAGE_SIZE == (64 * KB)

#define PAGE_LEVELS 2
#define LEVEL0_PAGE_SHIFT 0
#define LEVEL1_PAGE_SHIFT 42
#define LEVEL2_PAGE_SHIFT 29
#define LEVEL3_PAGE_SHIFT 16
#define FIRST_LEVEL 2
#define LAST_LEVEL 2

#else
#error "Unsupported PAGE_SIZE (only 4KB,16KB or 64KB page sizes are supported)"
#endif

#define PAGE_INDEX_SIZE (LEVEL2_PAGE_SHIFT - LEVEL3_PAGE_SHIFT)
#define PTE_SIZE 		8
#define PTES_PER_PAGE 	(PAGE_SIZE/PTE_SIZE)
#define LEVEL0_PAGE_SIZE (1UL << LEVEL0_PAGE_SHIFT)
#define LEVEL1_PAGE_SIZE (1UL << LEVEL1_PAGE_SHIFT)
#define LEVEL2_PAGE_SIZE (1UL << LEVEL2_PAGE_SHIFT)
#define LEVEL3_PAGE_SIZE (1UL << LEVEL3_PAGE_SHIFT)

/* LPAE doesn't support single mapping of > 1GB */
#define MAX_MAPPING_ALLOWED (1*GB)

extern int _end;

static struct {
	uint64_t size;
	uint32_t shift;
} tt_level[] = {
	{ LEVEL0_PAGE_SIZE, LEVEL0_PAGE_SHIFT },
	{ LEVEL1_PAGE_SIZE, LEVEL1_PAGE_SHIFT },
	{ LEVEL2_PAGE_SIZE, LEVEL2_PAGE_SHIFT },
	{ LEVEL3_PAGE_SIZE, LEVEL3_PAGE_SHIFT },
};

/* The reserved space for the page-tables is NUM_PAGE_TABLE_PAGES * PAGE_SIZE */
#define NUM_PAGE_TABLE_PAGES		32

#if NUM_PAGE_TABLE_PAGES > 64
#error "Free page bitmap for translation tables is 64bit only"
#endif

/* the location of the table may be brought in from outside */
#if WITH_EXTERNAL_TRANSLATION_TABLE
#if !defined(MMU_TRANSLATION_TABLE_ADDR)
#error must set MMU_TRANSLATION_TABLE_ADDR in the make configuration
#endif
static uint8_t *tt_base = (void *)MMU_TRANSLATION_TABLE_ADDR;
#else
/* the main translation table */
static uint8_t tt_base[PAGE_SIZE * NUM_PAGE_TABLE_PAGES] __ALIGNED(PAGE_SIZE)
					__SECTION(".bss.prebss.translation_table");
#endif

#define PTE_VALID				(1ULL)
#define PTE_SECTION				(1ULL)
#define PTE_TABLE				(2ULL)
#define PTE_PAGE				(3ULL)
#define PTE_MEMATTR_INDEX_SHIFT	(2)
#define PTE_MEMATTR_INDEX_MASK	(7ULL << PTE_MEMATTR_INDEX_SHIFT)
#define PTE_NON_SECURE			(1ULL << 5)
#define PTE_AP_MASK				(3ULL << 6)
#define PTE_PERM_READWRITE		(0ULL << 6)
#define PTE_PERM_READONLY		(2ULL << 6)
#define PTE_SH_MASK				(3ULL << 8)
#define PTE_SH_NON				(0ULL << 8)
#define PTE_SH_OUTER			(2ULL << 8)
#define PTE_SH_INNER			(3ULL << 8)
#define PTE_ACCESS_FLAG			(1ULL << 10)
#define PTE_NON_GLOBAL			(1ULL << 11)
#define PTE_CONTIGUOUS			(1ULL << 52)
#define PTE_PXN					(1ULL << 53)
#define PTE_XN					(1ULL << 54)
#define PTE_PXNTABLE			(1ULL << 59)
#define PTE_XNTABLE				(1ULL << 60)
#define PTE_APTABLE_MASK		(3ULL << 61)
#define PTE_NSTABLE				(1ULL << 63)
#define PTE_OUTPUT_ADDR_MASK ((((1ULL << PA_SIZE_BITS) - 1) >> 12) << 12)
#define PTE_NEXT_LEVEL_TABLE_ADDR_MASK ((((1ULL << PA_SIZE_BITS) - 1) >> 12) << 12)

#define MEM_TYPE_INDEX_DEV_nGnRnE		0
#define MEM_TYPE_INDEX_NORMAL_UC		1
#define MEM_TYPE_INDEX_NORMAL_WT		2
#define MEM_TYPE_INDEX_NORMAL_WB		3

#define MAIR_FIELD(val, idx)	(((val) & 0xFFULL) << ((idx) * 8))

#define MAIR_VALUE  (MAIR_FIELD(0x00, MEM_TYPE_INDEX_DEV_nGnRnE) | \
					 MAIR_FIELD(0x44, MEM_TYPE_INDEX_NORMAL_UC) | \
					 MAIR_FIELD(0x88, MEM_TYPE_INDEX_NORMAL_WT) | \
					 MAIR_FIELD(0xFF, MEM_TYPE_INDEX_NORMAL_WB))

#define PTE_NON_SHAREABLE		(0ULL << 8)
#define PTE_OUTER_SHAREABLE		(2ULL << 8)
#define PTE_INNER_SHAREABLE		(3ULL << 8)

#define PTE_ATTRIBS_MASK  		((0x3LU << 52) | 0xFFCLU)

#define MEM_TYPE_NORMAL_WB  (MEM_TYPE_INDEX_NORMAL_WB << PTE_MEMATTR_INDEX_SHIFT)
#define MEM_TYPE_NORMAL_WT  (MEM_TYPE_INDEX_NORMAL_WT << PTE_MEMATTR_INDEX_SHIFT)
#define MEM_TYPE_NORMAL_UC  (MEM_TYPE_INDEX_NORMAL_UC << PTE_MEMATTR_INDEX_SHIFT)
#define MEM_TYPE_DEVICE_SO  (MEM_TYPE_INDEX_DEV_nGnRnE << PTE_MEMATTR_INDEX_SHIFT)

#define TCR_T0SZ(VA_SIZE)	(((64 - (VA_SIZE)) & 0x3FUL) << 0)
#define TCR_EPD0			(0x1UL << 7)
#define TCR_IRGN0_NC		(0x0UL << 8)
#define TCR_IRGN0_WBWA		(0x1UL << 8)
#define TCR_IRGN0_WT		(0x2UL << 8)
#define TCR_IRGN0_WB		(0x3UL << 8)
#define TCR_ORGN0_NC		(0x0UL << 10)
#define TCR_ORGN0_WBWA		(0x1UL << 10)
#define TCR_ORGN0_WT		(0x2UL << 10)
#define TCR_ORGN0_WB		(0x3UL << 10)
#define TCR_SH0_NON			(0x0UL << 12)
#define TCR_SH0_OUTER		(0x2UL << 12)
#define TCR_SH0_INNER		(0x3UL << 12)
#define TCR_TG0_4KB			(0x0UL << 14)
#define TCR_TG0_16KB		(0x2UL << 14)
#define TCR_TG0_64KB		(0x1UL << 14)
#define TCR_T1SZ(VA_SIZE)	(((64 - (VA_SIZE)) & 0x3FUL) << 16)
#define TCR_A1				(0x1UL << 22)
#define TCR_EPD1			(0x1UL << 23)
#define TCR_IRGN1_MASK		(0x3UL << 24)
#define TCR_ORGN1_MASK		(0x3UL << 26)
#define TCR_SH1_NON			(0x0UL << 28)
#define TCR_SH1_OUTER		(0x2UL << 28)
#define TCR_SH1_INNER		(0x3UL << 28)
#define TCR_TG1_4KB			(0x2UL << 30)
#define TCR_TG1_16KB		(0x1UL << 30)
#define TCR_TG1_64KB		(0x3UL << 30)
#define TCR_IPS_4GB			(0x0UL << 32)
#define TCR_IPS_64GB		(0x1UL << 32)
#define TCR_IPS_1TB			(0x2UL << 32)
#define TCR_IPS_4TB			(0x3UL << 32)
#define TCR_IPS_16TB		(0x4UL << 32)
#define TCR_IPS_256TB		(0x5UL << 32)
#define TCR_ASID_16BIT		(0x1UL << 36)
#define TCR_TBI0			(0x1UL << 37)
#define TCR_TBI1			(0x1UL << 38)
#define TCR_PS_4GB			(0x0UL << 16)
#define TCR_PS_64GB			(0x1UL << 16)
#define TCR_PS_1TB			(0x2UL << 16)
#define TCR_PS_4TB			(0x3UL << 16)
#define TCR_PS_16TB			(0x4UL << 16)
#define TCR_PS_256TB		(0x5UL << 16)

#define SCTLR_M				(0x1UL << 0)
#define SCTLR_C				(0x1UL << 2)
#define SCTLR_I				(0x1UL << 12)

/* Free pages are marked as 1 */
static uint64_t free_page_table_bitmap = ((1UL << NUM_PAGE_TABLE_PAGES) - 1);

/* Allocates a free page for translation tables (TT) */
static uint64_t *allocate_tt_page(void)
{
	static uint64_t *result = NULL;
	uint32_t i = 0;

	while ((i < (sizeof(free_page_table_bitmap) * 8)) &&
		   (i < NUM_PAGE_TABLE_PAGES)) {
		if (free_page_table_bitmap & (1UL << i)) {
			result = (uint64_t *)tt_base + (i * PTES_PER_PAGE);
			memset(result, 0, PAGE_SIZE);
			free_page_table_bitmap &= ~(1UL << i);
			break;
		}
		i++;
	}
	LTRACEF("free_page_table_bitmap: 0x%" PRIx64"\n", free_page_table_bitmap);

	if (result)
		LTRACEF("next_tt: %p\n", result);
	else
		dprintf(CRITICAL, "%s:  No free pages available for TT\n", __func__);
	return result;
}

/* Frees up a translation table page */
static void free_tt_page(uint64_t *tt)
{
	uint32_t index = (((uint64_t)tt - (uint64_t)tt_base) >> LEVEL3_PAGE_SHIFT);
	LTRACEF("%s() called for tt=0x%p\n", __func__, tt);

	if (((uint64_t)tt & (PAGE_SIZE - 1)) || (index >= NUM_PAGE_TABLE_PAGES)) {
		panic("invalid tt: %p used in free_tt()\n", tt);
	}

	free_page_table_bitmap |= (1UL << index);
	LTRACEF("free_page_table_bitmap: 0x%" PRIx64"\n", free_page_table_bitmap);
}

/* Initializes entire page table of particular level based on
 * a template PTE. The output address is determined from the template PTE
 * and then increment as pet the page-table level. The attributes are
 * also derived from the sample PTE */
static void fill_tt(uint64_t *tt, int level, uint64_t template_pte)
{
	uint32_t i;
	uint64_t paddr = (template_pte & PTE_OUTPUT_ADDR_MASK);

	LTRACEF("%s: tt:%p, L%d, template_pte:0x%" PRIx64"\n", __func__,
			tt, level, template_pte);
	for ( i = 0; i < PTES_PER_PAGE; i++) {
		if (level < 3) {
			tt[i] = (paddr & PTE_OUTPUT_ADDR_MASK) | \
					(template_pte & PTE_ATTRIBS_MASK) | \
					PTE_ACCESS_FLAG | PTE_SECTION;
		} else {
			tt[i] = (paddr & PTE_OUTPUT_ADDR_MASK) | \
					(template_pte & PTE_ATTRIBS_MASK) | \
					PTE_ACCESS_FLAG | PTE_PAGE;
		}
		LTRACEF("%s: (L%d) index = %u, pte = 0x%" PRIx64"\n", __func__,
				level, i, tt[i]);
		paddr += tt_level[level].size;
	}
	tegrabl_arch_clean_dcache_range((addr_t)tt, PAGE_SIZE);
}

/* Returns the amount of memory actually mapped */
static addr_t arm64_mmu_map_level(uint64_t *tt, uint32_t level, addr_t vaddr,
		addr_t paddr, addr_t size, uint32_t flags)
{
	uint32_t index;
	uint64_t *next_tt = NULL;
	addr_t map_size;
	addr_t total_size = 0;
	uint64_t attribs;
	bool needs_flush;
	uint64_t old_mem_attrib, new_mem_attrib;

	index = (vaddr >> tt_level[level].shift) & ((1 << PAGE_INDEX_SIZE) - 1);

	LTRACEF("TT: %p, L%d, VA:0x%lx, PA:0x%lx, SZ:0x%lx, FLAGS:0x%08X\n",
			tt, level, vaddr, paddr, size, flags);

	for (; (index < PTES_PER_PAGE) && (size > 0); index++) {
		LTRACEF("(L%d) index = %u, size:0x%lx\n", level, index, size);
		/* We cannot create a section mapping for > 1GB and if the vaddr
		 * is aligned to this level's size. */
		if (((tt_level[level].size > MAX_MAPPING_ALLOWED) ||
			((vaddr & (tt_level[level].size - 1)) != 0) ||
			(size < tt_level[level].size)) && (level < LAST_LEVEL)) {
			if (tt[index] & PTE_VALID) {
				if (tt[index] & PTE_TABLE) {
					LTRACEF("Using existing next-tt\n");
					next_tt = (uint64_t *)(tt[index] &
							PTE_NEXT_LEVEL_TABLE_ADDR_MASK);
				} else {
					LTRACEF("Overwriting PTE:0x%08" PRIx64" for VA:0x%08lx\n",
							tt[index], vaddr);
					next_tt = allocate_tt_page();
					assert(next_tt != NULL);
					fill_tt(next_tt, level+1, tt[index]);
					tt[index] = (((uint64_t) next_tt & PTE_NEXT_LEVEL_TABLE_ADDR_MASK) |
							PTE_TABLE | PTE_VALID);
					asm volatile ("dc civac, %0\n" : : "r" (tt + index));
					LTRACEF("NEW_ENTRY: 0x%016" PRIx64"\n", tt[index]);
				}
			} else {
				LTRACEF("Creating next-tt\n");
				next_tt = allocate_tt_page();
				assert(next_tt != NULL);
				tt[index] = (((uint64_t)next_tt & PTE_NEXT_LEVEL_TABLE_ADDR_MASK) |
					PTE_TABLE | PTE_VALID);
				asm volatile ("dc civac, %0\n" : : "r" (tt + index));
				LTRACEF("NEW_ENTRY: 0x%016" PRIx64"\n", tt[index]);
			}
			map_size = (size < tt_level[level].size) ? size :
					tt_level[level].size;
			map_size = arm64_mmu_map_level(next_tt, level+1, vaddr, paddr,
					map_size, flags);
			vaddr += map_size;
			paddr += map_size;
			if (map_size > size)
				size = 0;
			else
				size -= map_size;
			total_size += map_size;
		} else {
			old_mem_attrib = 0x0;
			/* check if mapping exists */
			LTRACEF("Creating/Updating page/section\n");
			if (tt[index] & PTE_VALID) {
				LTRACEF("Overwriting PTE:0x%08" PRIx64" for VA:%08lx\n",
						tt[index], vaddr);
				old_mem_attrib = tt[index] & PTE_MEMATTR_INDEX_MASK;
				/* If mapped using next level mapping table, free it */
				if ((level < LAST_LEVEL) && (tt[index] & PTE_TABLE)) {
					/* Free the sub-table */
					next_tt = (uint64_t *)(tt[index] &
							PTE_NEXT_LEVEL_TABLE_ADDR_MASK);
					free_tt_page(next_tt);
				}
			}

			attribs = 0;
			new_mem_attrib = 0;
			/* Memory attributes */
			if (flags & MMU_FLAG_DEVICE) {
				new_mem_attrib = MEM_TYPE_DEVICE_SO;
			} else {
				if (flags & MMU_FLAG_CACHED) {
					if (flags & MMU_FLAG_WRITETHROUGH) {
						new_mem_attrib = MEM_TYPE_NORMAL_WT;
					} else {
						new_mem_attrib = MEM_TYPE_NORMAL_WB;
					}
				} else {
					new_mem_attrib = MEM_TYPE_NORMAL_UC;
				}
			}
			attribs |= new_mem_attrib;

			/* Certain types of cacheability attribute changes demand flush of
			 * affected region to avoid possible coherency errors */
			needs_flush = ((old_mem_attrib != MEM_TYPE_NORMAL_UC) &&
						   (old_mem_attrib != MEM_TYPE_DEVICE_SO) &&
						   (old_mem_attrib != new_mem_attrib));

			LTRACEF("[mem-attr] %" PRIx64" => %" PRIx64"\n", old_mem_attrib, new_mem_attrib);
			if (needs_flush) {
				LTRACEF("[%s:%d] flushing 0x%lx\n", __func__, __LINE__, vaddr);
				tegrabl_arch_clean_invalidate_dcache_range(vaddr,
														tt_level[level].size);
			}

			/* Execute Not */
			if (flags & MMU_FLAG_EXECUTE_NOT)
				attribs |= PTE_XN | PTE_PXN;

			/* Access Permission */
			if (flags & MMU_FLAG_READWRITE)
				attribs |= PTE_PERM_READWRITE;
			else
				attribs |= PTE_PERM_READONLY;

			if (level < 3)
				tt[index] = (paddr & PTE_OUTPUT_ADDR_MASK) | attribs |
					PTE_ACCESS_FLAG  | PTE_SH_OUTER | PTE_SECTION;
			else
				tt[index] = (paddr & PTE_OUTPUT_ADDR_MASK) | attribs |
					PTE_ACCESS_FLAG  | PTE_SH_OUTER | PTE_PAGE;
			__asm__ volatile ("dc civac, %0\n" : : "r" (tt + index));
			/* Invalidate TLB by MVA */
#if ARM64_WITH_EL2
			__asm__ volatile ("tlbi vae2, %0\n" : : "r" (paddr >> 12));
#else
			__asm__ volatile ("tlbi vae1, %0\n" : : "r" (paddr >> 12));
#endif
			DSB;
			ISB;

			LTRACEF("NEW_ENTRY: 0x%016" PRIx64"\n", tt[index]);
			if (tt_level[level].size > size)
				size = 0;
			else
				size -= tt_level[level].size;
			paddr += tt_level[level].size;
			vaddr += tt_level[level].size;
			total_size += tt_level[level].size;
		}
	}

	return total_size;
}

static void arm64_invalidate_tlb(void)
{
#if ARM64_WITH_EL2
	asm volatile ("tlbi alle2is\n");
#else
	asm volatile ("tlbi vmalle1is\n");
#endif
	DSB;
	ISB;
}

void arm64_mmu_map(addr_t paddr, addr_t vaddr, addr_t size, uint32_t flags)
{
	enter_critical_section();
	arm64_mmu_map_level((uint64_t *)tt_base, FIRST_LEVEL,
			vaddr, paddr, size, flags);
	arm64_invalidate_tlb();
	exit_critical_section();
}

void arch_map_uncached(addr_t vaddr, addr_t size)
{
	LTRACEF("%s: [addr:0x%lx ; size:0x%lx]\n", __func__, vaddr, size);
	arm64_mmu_map(vaddr, vaddr, size, MMU_FLAG_READWRITE);
}

void arch_map_cached(addr_t vaddr, addr_t size)
{
	LTRACEF("%s: [addr:0x%lx ; size:0x%lx]\n", __func__, vaddr, size);
	arm64_mmu_map(vaddr, vaddr, size, MMU_FLAG_CACHED | MMU_FLAG_READWRITE);
}

static void arm64_init_boot_param(struct tboot_cpubl_params **boot_params)
{
	uint32_t scratch7;

	/* Read CPUBL params pointer from SCRATCH_7 register */
	scratch7 = NV_READ32(NV_ADDRESS_MAP_SCRATCH_BASE + SCRATCH_SCRATCH_7);

	if (scratch7 < NV_ADDRESS_MAP_EMEM_BASE) {
		*boot_params = (struct tboot_cpubl_params *)(uintptr_t)((uint64_t)scratch7 << CONFIG_PAGE_SIZE_LOG2);
	} else {
		*boot_params = (struct tboot_cpubl_params *)(uintptr_t)scratch7;
	}
}

void arm64_mmu_init(void)
{
	uint64_t reg;
	struct tboot_cpubl_params *boot_params;

	/* Initialize SCTLR with sane value */
	ARM64_WRITE_TARGET_SYSREG(SCTLR_ELx, 0x30D00800);

	/* initialize MAIR */
	LTRACEF("MAIR: 0x%llx\n", MAIR_VALUE);
	ARM64_WRITE_TARGET_SYSREG(MAIR_ELx, MAIR_VALUE);

	/* initialize TCR */
	reg = TCR_T0SZ(VA_SIZE_BITS) | TCR_SH0_OUTER | TCR_EPD1;
	/* page-table region is marked write-back write-allocate cacheable */
	reg |= TCR_IRGN0_WBWA | TCR_ORGN0_WBWA;
#if PAGE_SIZE == (4 * KB)
	reg |= TCR_TG0_4KB;
#elif PAGE_SIZE == (16 * KB)
	reg |= TCR_TG0_16KB;
#elif PAGE_SIZE == (64 * KB)
	reg |= TCR_TG0_64KB;
#endif
#if ARM64_WITH_EL2
	reg |= TCR_PS_1TB;
#else
	reg |= TCR_IPS_1TB;
#endif
	dprintf(SPEW, "TCR: 0x%" PRIx64"\n", reg);
	ARM64_WRITE_TARGET_SYSREG(TCR_ELx, reg);

	LTRACEF("free_page_table_bitmap: 0x%" PRIx64"\n", free_page_table_bitmap);
	/* Marked the first page as allocated for TT-base */
	free_page_table_bitmap &= ~1UL;
	memset(tt_base, 0, PAGE_SIZE);
	LTRACEF("free_page_table_bitmap: 0x%" PRIx64"\n", free_page_table_bitmap);
	dprintf(SPEW, "Translation Table Base: %p\n", tt_base);
	/* set up the translation table base */
	ARM64_WRITE_TARGET_SYSREG(TTBR0_ELx, (uint64_t)tt_base);

	/* Read boot param pointer from scratch7 register to get carveout info shared from previous boot stage */
	arm64_init_boot_param(&boot_params);

#if defined(IS_T186)
	/* Map cboot region */
	arm64_mmu_map(boot_params->global_data.carveout[CARVEOUT_CPUBL].base,
				  boot_params->global_data.carveout[CARVEOUT_CPUBL].base,
				  (addr_t)boot_params->global_data.carveout[CARVEOUT_CPUBL].size,
				  MMU_FLAG_CACHED | MMU_FLAG_READWRITE);
#else
	/* Map cboot region */
	arm64_mmu_map(boot_params->carveout_info[CARVEOUT_CPUBL].base,
				  boot_params->carveout_info[CARVEOUT_CPUBL].base,
				  (addr_t)boot_params->carveout_info[CARVEOUT_CPUBL].size,
				  MMU_FLAG_CACHED | MMU_FLAG_READWRITE);
#endif

	/* turn on the mmu */
	ARM64_WRITE_TARGET_SYSREG(SCTLR_ELx, ARM64_READ_TARGET_SYSREG(SCTLR_ELx) | SCTLR_M  | SCTLR_I | SCTLR_C);
	ISB;
}

void arch_disable_mmu(void)
{
	uint64_t val;

	/* disable mmu and cache */
	val = ARM64_READ_TARGET_SYSREG(SCTLR_ELx);
	val &= ~(SCTLR_I | SCTLR_C | SCTLR_M);
	ARM64_WRITE_TARGET_SYSREG(SCTLR_ELx, val);
	ISB;

	/* flush caches */
	tegrabl_arch_disable_cache(UCACHE);
}

#if LK_DEBUGLEVEL > CRITICAL
#if WITH_LIB_CONSOLE

#include <lib/console.h>

static int mmu_info(int argc, const cmd_args *argv)
{
	uint64_t reg;

	reg =  ARM64_READ_SYSREG(ID_AA64MMFR0_EL1);
	dprintf(INFO, "ID_AA64MMFR0_EL1: 0x%08" PRIx64"\n", reg);

	dprintf(INFO, "MAIR_ELx: 0x%08" PRIx64"\n",
			ARM64_READ_TARGET_SYSREG(MAIR_ELx));

	dprintf(INFO, "TTBR0_ELx: 0x%08" PRIx64"\n",
			ARM64_READ_TARGET_SYSREG(TTBR0_ELx));

	dprintf(INFO, "TCR_ELx: 0x%08" PRIx64"\n",
			ARM64_READ_TARGET_SYSREG(TCR_ELx));

	dprintf(INFO, "PAGE_SIZE: %d KB\n", PAGE_SIZE >> 10);

	reg = ARM64_READ_TARGET_SYSREG(SCTLR_ELx);
	dprintf(INFO, "SCTLR_ELx: 0x%08" PRIx64" (MMU: ", reg);
	if (reg & SCTLR_M)
		dprintf(INFO, "enabled)\n");
	else
		dprintf(INFO, "disabled)\n");

	dprintf(INFO, "Free-page bitmap for TT: 0x%" PRIx64"\n", free_page_table_bitmap);

	return 0;
}

STATIC_COMMAND_START
STATIC_COMMAND("mmu", "info about mmu", &mmu_info)
STATIC_COMMAND_END(mmu);
#endif
#endif
