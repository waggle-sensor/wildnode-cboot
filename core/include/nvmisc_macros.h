/*
 * Copyright (c) 1993-2017, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#ifndef _NVMISC_MACROS_H
#define _NVMISC_MACROS_H

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

/* These macros are taken from desktop driver branch file
 *  //sw/dev/gpu_drv/$BRANCH/sdk/nvidia/inc/nvmisc.h
 * This is to handle difference between DRF macros
 * between Mobile and Desktop
 */

// nvmisc_macros.h can't access nvmisc.h in the Mobile tree... but in some places both are used.
#ifndef __NV_MISC_H

#define DEVICE_BASE(d)          (0 ? d)  // what's up with this name? totally non-parallel to the macros below
#define DEVICE_EXTENT(d)        (1 ? d)  // what's up with this name? totally non-parallel to the macros below

// the first bit occupied by the field in a HW value (e.g., register)
#define DRF_BASE(drf)           ((unsigned int)(0 ? drf))

// the last bit occupied by the field in a HW value
#define DRF_EXTENT(drf)         ((unsigned int)(1 ? drf))

// the bit shift amount for accessing the field
#define DRF_SHIFT(drf)          (DRF_BASE(drf) % 32U)

// bitmask for the value (unshifted)
#define DRF_MASK(drf)           (0xFFFFFFFFU >> (31U - (DRF_EXTENT(drf) % 32U) + (DRF_BASE(drf) % 32U)))

// bitmask for the value (shifted for HW value)
#define DRF_SHIFTMASK(drf)      (DRF_MASK(drf) << (DRF_SHIFT(drf)))

// size of the field (in bits)
#define DRF_SIZE(drf)           (DRF_EXTENT(drf) - DRF_BASE(drf) + 1U)

// constant value for HW, e.g., reg |= DRF_DEF(A06F, _GP_ENTRY1, _LEVEL, _SUBROUTINE);
#define DRF_DEF(d,r,f,c)        (((unsigned int)(NV ## d ## r ## f ## c)) << DRF_SHIFT(NV ## d ## r ## f))

// numeric value for HW, e.g., reg |= DRF_NUM(A06F, _GP_ENTRY1, _LENGTH, numWords);
// Note: n should be unsigned 32-bit integer
#define DRF_NUM(d,r,f,n)        (((n) & DRF_MASK(NV ## d ## r ## f)) << DRF_SHIFT(NV ## d ## r ## f))

// numeric value from HW value, e.g., unsigned int numWords = DRF_VAL(A06F, _GP_ENTRY1, _LENGTH, reg);
// Note: v should be unsigned 32-bit integer
#define DRF_VAL(d,r,f,v)        (((v) >> DRF_SHIFT(NV ## d ## r ## f)) & DRF_MASK(NV ## d ## r ## f))

#endif

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //_NVMISC_MACROS_H
