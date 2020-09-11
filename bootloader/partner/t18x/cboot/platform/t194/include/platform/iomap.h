/*
 * Copyright (c) 2017, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited
 */

#ifndef __PLATFORM_T194_IOMAP_H_
#define __PLATFORM_T194_IOMAP_H_

#include <tegrabl_addressmap.h>

/* Base Addresses of Nvidia modules */
/* GIC base */
#define T186_GICD_BASE  (NV_ADDRESS_MAP_CCPLEX_GIC_BASE)

/* Tos params offset in MB1 BCT */
#define CPUPARAMS_TOS_PARAMS_OFFSET 0x2188
#define CPUPARAMS_TOS_START_OFFSET  0x21a8

#endif /* __PLATFORM_T194_IOMAP_H_ */

