/*
 * Copyright (c) 2008 Travis Geiselbrecht
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
 * Copyright (c) 2014-2016, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited
 */

#include <debug.h>
#include <trace.h>
#include <malloc.h>
#include <string.h>
#include <lib/heap.h>
#include <arch/ops.h>
#include <arch/defines.h>

#include <tegrabl_malloc.h>

#define LOCAL_TRACE   0

void *malloc(size_t size)
{
	return (void *)tegrabl_malloc(size);
}

void *memalign(size_t boundary, size_t size)
{
	return tegrabl_memalign(boundary, size);
}

void *calloc(size_t count, size_t size)
{
	return tegrabl_calloc(count, size);
}

void *realloc(void *ptr, size_t size)
{
	if (!ptr)
		return malloc(size);
	return tegrabl_realloc(ptr, size);
}

void free(void *ptr)
{
	return tegrabl_free(ptr);
}

#if WITH_MMU
void *malloc_uncached(size_t size, size_t *asize)
{
	void *res;
	size_t boundary = PAGE_SIZE;
	if (asize == NULL)
		return NULL;
	LTRACEF("boundary: 0x%lx, size: 0x%lx\n", boundary, size);
	/* Make size multiple of PAGE_SIZE */
	size = (size + PAGE_SIZE - 1) & ~((uintptr_t)PAGE_SIZE - 1);
	LTRACEF("boundary: 0x%lx, size: 0x%lx\n", boundary, size);
	res = memalign(boundary, size);
	if (res) {
		arch_map_uncached((addr_t)res, size);
		*asize = size;
		LTRACEF("res:%p, asize:%zu\n", res, *asize);
	}
	return res;
}

void *memalign_uncached(size_t boundary, size_t size, size_t *asize)
{
	void *res;
	if (asize == NULL)
		return NULL;
	LTRACEF("boundary: 0x%lx, size: 0x%lx\n", boundary, size);
	/* Align boundary to PAGE_SIZE */
	if (boundary)
		boundary = (boundary + PAGE_SIZE - 1) & ~((uintptr_t)
				PAGE_SIZE - 1);
	else
		boundary = PAGE_SIZE;
	/* Make size multiple of PAGE_SIZE */
	size = (size + PAGE_SIZE - 1) & ~((uintptr_t)PAGE_SIZE - 1);
	LTRACEF("boundary: 0x%lx, size: 0x%lx\n", boundary, size);
	res = memalign(boundary, size);
	if (res) {
		arch_map_uncached((addr_t)res, size);
		*asize = size;
		LTRACEF("res:%p, asize:%zu\n", res, *asize);
	}
	return res;
}

void free_uncached(void *ptr, size_t asize)
{
	LTRACEF("ptr:%p, asize:%zu\n", ptr, asize);
	arch_map_cached((addr_t)ptr, asize);
	return free(ptr);
}
#else
void *malloc_uncached(size_t size, size_t *asize)
{
	return malloc(size);
}

void *memalign_uncached(size_t boundary, size_t size, size_t *asize)
{
	return memalign(boundary, size);
}

void free_uncached(void *ptr, size_t asize)
{
	return free(ptr);
}
#endif
