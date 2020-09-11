/*
 * Copyright (c) 2018, NVIDIA CORPORATION.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#ifndef TEGRABL_VPRINFO_H
#define TEGRABL_VPRINFO_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief get whether vpr reset is enabled or not
 *
 * @return true if it is enabled, false if it is not enabled
 */
bool tegrabl_is_vpr_resize_enabled(void);

#endif /* TEGRABL_VPRINFO_H */

