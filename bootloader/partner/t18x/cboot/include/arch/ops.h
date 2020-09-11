/*
 * Copyright (c) 2008-2014 Travis Geiselbrecht
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
 * Copyright (c) 2014, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited
 */

#ifndef __ARCH_OPS_H
#define __ARCH_OPS_H

#ifndef ASSEMBLY

#include <sys/types.h>
#include <stddef.h>
#include <stdbool.h>
#include <compiler.h>

__BEGIN_CDECLS

/* fast routines that most arches will implement inline */

/**
 * @brief Enable interrupts
 */
static void arch_enable_ints(void);

/**
 * @brief Disables interrupts
 */
static void arch_disable_ints(void);

/**
 * @brief Checks whether interrupts are disabled.
 *
 * @return TRUE if interrupts are disabled, otherwise FALSE.
 */
static bool arch_ints_disabled(void);

/**
 * @brief Atomically replaces the value in a variable with a specified value.
 *
 * @param ptr Pointer to the variable whose value needs to be replaced.
 * @param val The new value to be updated in the atomic variable.
 *
 * @return The older/previous value contained in ptr.
 */
static int atomic_swap(volatile int *ptr, int val);

/**
 * @brief Atomically adds a specified value to the value in a variable.
 *
 * @param ptr Pointer to the variable whose value needs to be updated.
 * @param val The new value to be added to the variable.
 *
 * @return The older/previous value contained in ptr.
 */
static int atomic_add(volatile int *ptr, int val);

/**
 * @brief Atomically clear bits from a variable.
 *
 * @param ptr Pointer to the variable whose value needs to be updated.
 * @param val The value specifying the bit-mask to be cleared.
 *
 * @return The older/previous value contained in ptr.
 */
static int atomic_and(volatile int *ptr, int val);

/**
 * @brief Atomically sets bits from a variable.
 *
 * @param ptr Pointer to the variable whose value needs to be updated.
 * @param val The value specifying the bit-mask to be set.
 *
 * @return The older/previous value contained in ptr.
 */
static int atomic_or(volatile int *ptr, int val);

/**
 * @brief Counts the cpu cycles
 *
 * @return Returns the number of cpu cycles
 */
static uint32_t arch_cycle_count(void);

/**
 * @brief Ensures that memory operations prior to this call have completed.
 */
static void arch_memory_barrier(void);

#endif // !ASSEMBLY
#define ICACHE 1
#define DCACHE 2
#define UCACHE (ICACHE|DCACHE)
#ifndef ASSEMBLY

/**
 * @brief Disables specified type of cache.
 *
 * @param flags One of the values { ICACHE, DCACHE, UCACHE }
 */
void arch_disable_cache(uint flags);

/**
 * @brief Enables specified type of cache.
 *
 * @param flags One of the values { ICACHE, DCACHE, UCACHE }
 */
void arch_enable_cache(uint flags);

/**
 * @brief Cleans the cached contents of a specifed address range.
 *        The cached blocks are not marked invalid.
 *
 * @param start Start of the address range to be cleaned
 * @param len Size of the address range to be cleaned in bytes
 */
void arch_clean_cache_range(addr_t start, size_t len);

/**
 * @brief Cleans the cached contents of a specifed address range.
 *        The cached blocks are marked invalid
 *
 * @param start Start of the address range to be cleaned
 * @param len Size of the address range to be cleaned in bytes
 */
void arch_clean_invalidate_cache_range(addr_t start, size_t len);

/**
 * @brief Invalidate the blocks in the caches holding data from a specifed
 *        address range. It is not guaranteed that the invalidated contents will
 *        not be written back to memory.
 *
 * @param start Start of the address range to be invalidated
 * @param len Size of the address range to be invalidated in bytes
 */
void arch_invalidate_cache_range(addr_t start, size_t len);

/**
 * @brief Synchronizes the cached contents of a specifed address range ensuring
 *        instructions fetched from that region (if any) will be updated even if
 *        they have been modified in data-caches.
 *
 * @param start Start of the address range to be cleaned
 * @param len Size of the address range to be cleaned in bytes
 */
void arch_sync_cache_range(addr_t start, size_t len);

/**
 * @brief Create identity mapping for the specified address-range
 *        with uncached memory attribute
 *
 * @param vaddr Start of the virtual address range
 * @param size Size of the virtual address range in bytes
 */
void arch_map_uncached(addr_t vaddr, addr_t size);

/**
 * @brief Create identity mapping for the specified address-range
 *        with cached memory attribute
 *
 * @param vaddr Start of the virtual address range
 * @param size Size of the virtual address range in bytes
 */
void arch_map_cached(addr_t vaddr, addr_t size);

/**
 * @brief Keeps the processors in low-power state when cpu is idle.
 */
void arch_idle(void);

/**
 * @brief Disables MMU, thereby also disabling all forms of caching.
 */
void arch_disable_mmu(void);

__END_CDECLS

#endif // !ASSEMBLY

#include <arch/arch_ops.h>

#endif
