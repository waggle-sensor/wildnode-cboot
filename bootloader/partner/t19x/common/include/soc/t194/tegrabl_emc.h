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
 * @file tegrabl_emc.h Interface for EMC related utilities
 */

#ifndef INCLUDED_TEGRABL_EMC_H
#define INCLUDED_TEGRABL_EMC_H

#include <stdbool.h>

#define MAX_EMC_CHANNELS 16UL

/**
 * @brief Unmask EMC error interrupts.
 */
void tegrabl_emc_enable_error_interrupts(void);

/**
 * @brief Check for EMC errors and dump them.
 */
bool tegrabl_emc_check_errors(void);

#endif
