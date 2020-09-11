/*
 * Copyright (c) 2016-2017, NVIDIA CORPORATION. All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited
 */

#ifndef TEGRABL_UFS_H
#define TEGRABL_UFS_H

#include <tegrabl_blockdev.h>
#include <tegrabl_error.h>
#include <tegrabl_ufs.h>

/** @brief Initializes the host controller for sata and card with the given
 *         instance.
 *
 *  @param init value
 *  @param ufs_params - UFS platform parameters required for configuring the
 *  UFS device.
 *
 *  @return TEGRABL_NO_ERROR if init is successful else appropriate error.
 */
tegrabl_error_t tegrabl_ufs_bdev_open(bool reinit, struct tegrabl_ufs_platform_params *ufs_params);

#endif /* TEGRABL_UFS_H */
