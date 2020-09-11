/*
 * Copyright (c) 2014 - 2016, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited
 */

#ifndef __ARCH_ARM64_MMU_H
#define __ARCH_ARM64_MMU_H

#include <sys/types.h>
#include <compiler.h>

__BEGIN_CDECLS

/* Flags for arm64_mmu_map() API */
#define MMU_FLAG_CACHED 0x1
#define MMU_FLAG_WRITETHROUGH 0x2
#define MMU_FLAG_DEVICE 0x4
#define MMU_FLAG_READWRITE 0x8
#define MMU_FLAG_EXECUTE_NOT 0x10

/* Configures and turns on the mmu with 4GB mapped as strongly-ordered memory */
void arm64_mmu_init(void);

/* Creates a MMU mapping
 *
 * @param paddr Physical Address of the region mapped
 * @param vaddr Virtual Address of the region mapped
 * @param size Size of the region mapped
 * @param flags One or more of the MMU_FLAGs defined above
 */
void arm64_mmu_map(addr_t paddr, addr_t vaddr, addr_t size, uint flags);

__END_CDECLS

#endif
