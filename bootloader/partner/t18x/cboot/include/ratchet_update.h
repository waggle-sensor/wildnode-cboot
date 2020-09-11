/*
 * Copyright (c) 2017, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited
 */

#ifndef __RATCHET_UPDATE_H
#define __RATCHET_UPDATE_H

/**
 * @brief Handle on-field ratchet fuse burning.
 *
 * Check for external factors and if they are satisfied, check if the ratchet
 * fuse needs to be updated.
 *
 * return TEGRABL_NO_ERROR on success otherwise appropriate error
 */
tegrabl_error_t update_ratchet_fuse(void);

#endif
