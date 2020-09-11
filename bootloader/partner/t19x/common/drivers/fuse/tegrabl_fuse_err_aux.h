/*
 * Copyright (c) 2018, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#ifndef INCLUDED_TEGRABL_FUSE_ERR_AUX_PRIVATE_H
#define INCLUDED_TEGRABL_FUSE_ERR_AUX_PRIVATE_H

#include <stdint.h>

#define AUX_INFO_FUSE_QUERY_SIZE				0x01U
#define AUX_INFO_BURN_FUSES_1					0x02U
#define AUX_INFO_BURN_FUSES_2					0x03U
#define AUX_INFO_FUSE_READ						0x04U
#define AUX_INFO_FUSE_CHECK_ECID				0x05U
#define AUX_INFO_SET_MACRO_AND_BURN				0x06U
#define AUX_INFO_FUSE_WRITE						0x07U
#define AUX_INFO_FUSE_WRITE_PRE_PROCESS			0x08U
#define AUX_INFO_FUSE_CONFIRM_BURN				0x09U
#define AUX_INFO_GET_FUSE_VALUE_1				0x0aU
#define AUX_INFO_GET_FUSE_VALUE_2				0x0bU
#define AUX_INFO_VERIFY_BURNT_FUSES				0x0cU

#endif
