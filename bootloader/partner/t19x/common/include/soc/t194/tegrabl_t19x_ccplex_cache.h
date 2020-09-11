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
 * @file tegrabl_t19x_ccplex_cache.h
 */

#ifndef INCLUDED_TEGRABL_T19X_CCPLEX_CACHE_H
#define INCLUDED_TEGRABL_T19X_CCPLEX_CACHE_H

/**
 * @brief CCPLEX level cache clean and invalidate for all interface
 */
void tegrabl_t19x_ccplex_flush_cache_all(void);

/**
 * @brief CCPLEX level cache clean and invalidate for data interface
 */
void tegrabl_t19x_ccplex_flush_dcache_all(void);

/**
 * @brief CCPLEX level data-cache clean
 */
void tegrabl_t19x_ccplex_clean_dcache_all(void);

#endif /* INCLUDED_TEGRABL_T19X_CCPLEX_CACHE_H */

