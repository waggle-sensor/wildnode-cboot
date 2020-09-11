/*
 * Copyright (c) 2014 - 2016, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited
 */

#include <stdint.h>

#ifndef __COMMON_H
#define __COMMON_H

typedef uint64_t	u64;
typedef uint32_t	u32;
typedef uint16_t	u16;
typedef uint8_t		u8;

typedef int64_t		s64;
typedef int32_t		s32;
typedef int16_t		s16;
typedef int8_t		s8;

#define INVERT_OF_NSEC 1000000000
#define ABS(x) (((x) < 0) ? -(x) : (x))


/**
* @brief Extracts basename from the given path.
*
* @param Specifies the entire path including the basename.
*
* @return Returns the basename.
*/
const char *get_basename(const char *path);

#endif
