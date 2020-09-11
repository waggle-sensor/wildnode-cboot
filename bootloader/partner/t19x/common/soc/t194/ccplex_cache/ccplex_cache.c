/*
 * Copyright (c) 2017, NVIDIA Corporation. All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

/**
 * @file ccplex_cache.c
 */

#include "build_config.h"
#include <stdint.h>
#include <tegrabl_debug.h>
#include <tegrabl_armv8a.h>
#include <tegrabl_t19x_ccplex_cache.h>

/*
 * These NV cache maintainence instructions are interruptible and can fail;
 * they return 0 if the operation didn't complete and 1 in case of success;
 * To ensure the do complete we retry the operation until it succeeds;
 */
#define retry_nvcache_ops(operation, count)							\
	{																\
		uint64_t ret;												\
		uint32_t retry = (count);									\
		do {														\
			ret = operation;										\
			retry--;												\
		} while ((ret != 1ULL) && (retry != 0UL));					\
		if (ret == 0ULL) {											\
			pr_info("%s failed\n", #operation);						\
		}															\
		tegrabl_dsb();												\
	}


void tegrabl_t19x_ccplex_flush_cache_all(void)
{
	retry_nvcache_ops(tegrabl_nv_cache_flush(), 10);
}

void tegrabl_t19x_ccplex_flush_dcache_all(void)
{
	retry_nvcache_ops(tegrabl_nv_dcache_flush(), 10);
}

void tegrabl_t19x_ccplex_clean_dcache_all(void)
{
	retry_nvcache_ops(tegrabl_nv_dcache_clean(), 10);
}
