/*
 * Copyright (c) 2017, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited
 */

#ifndef __STORAGE_LOADER_H
#define __STORAGE_LOADER_H

/**
 * @brief Load and compare SC7 primary and recovery images.
 *
 * @param *are_bins_same Varibale to output comparision result.
 *
 * return TEGRABL_NO_ERROR on success otherwise appropriate error
 */
tegrabl_error_t load_and_compare_sc7_images(bool *are_bins_same);

#endif
