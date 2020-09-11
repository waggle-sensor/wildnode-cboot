/*
 * Copyright (c) 2016-2018, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited
 */

#include <stdint.h>
#include <tegrabl_cache.h>
#include <tegrabl_module.h>
#include <tegrabl_dmamap.h>
#include <tegrabl_compiler.h>
#include <tegrabl_addressmap.h>

#define SYSRAM_PA(addr)				\
	((addr) - NV_ADDRESS_MAP_REMAP_BASE + NV_ADDRESS_MAP_SYSRAM_0_BASE)

#define NV_ADDRESS_MAP_SYSRAM_REMAP_BASE	NV_ADDRESS_MAP_REMAP_BASE
#define NV_ADDRESS_MAP_SYSRAM_REMAP_END		(NV_ADDRESS_MAP_REMAP_BASE + \
											NV_ADDRESS_MAP_SYSRAM_0_IMPL_SIZE)

dma_addr_t tegrabl_dma_map_buffer(tegrabl_module_t module, uint8_t instance,
								  void *buffer, size_t size,
								  tegrabl_dma_data_direction direction)
{
	TEGRABL_UNUSED(instance);

	if (direction & TEGRABL_DMA_TO_DEVICE) {
		tegrabl_arch_clean_invalidate_dcache_range((uintptr_t)buffer, size);
	} else {
		tegrabl_arch_invalidate_dcache_range((uintptr_t)buffer, size);
	}

	return tegrabl_dma_va_to_pa(module, buffer);
}

void tegrabl_dma_unmap_buffer(tegrabl_module_t module, uint8_t instance,
							  void *buffer, size_t size,
							  tegrabl_dma_data_direction direction)
{
	TEGRABL_UNUSED(module);
	TEGRABL_UNUSED(instance);

	if (direction & TEGRABL_DMA_FROM_DEVICE)
		tegrabl_arch_invalidate_dcache_range((uintptr_t)buffer, size);
}

dma_addr_t tegrabl_dma_va_to_pa(tegrabl_module_t module, void *va)
{
	dma_addr_t retval;
	uintptr_t virt_addr;
	uint64_t par_el1;

	TEGRABL_UNUSED(module);

	virt_addr = (uintptr_t)va;

	/* Do Stage-1 EL2 VA-PA address-translations */
	asm volatile ("at s1e2r, %[va]" : : [va]"r"(virt_addr));
	asm volatile ("mrs %[par], par_el1" : [par]"=r"(par_el1));

	/* PAR_EL1[0] == 1 means translation was unsuccessful, so return 0
	 * otherwise PAR_EL1[47:12] holds the PA corresponding to VA[63:12] */
	retval = (par_el1 & 1LLU) ?
		0LLU : ((par_el1 & 0xFFFFFFFFF000LLU) | (virt_addr & 0xFFFLLU));

	return retval;
}
