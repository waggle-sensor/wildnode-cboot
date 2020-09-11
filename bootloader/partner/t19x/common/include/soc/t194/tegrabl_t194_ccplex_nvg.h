/*
 * Copyright (c) 2018, NVIDIA Corporation. All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

/**
 * @file tegrabl_t194_ccplex_nvg.h
 */

#ifndef INCLUED_TEGRABL_T194_CCPLEX_NVG_H
#define INCLUED_TEGRABL_T194_CCPLEX_NVG_H

#include <stdint.h>

/**
 * @brief Probe the NVG interface version supported by the CCPLEX HW.
 *
 * @param major (output) Major version of NVG interface
 * @param minor (output) Minor version of NVG interface
 */
void tegrabl_ccplex_nvg_get_hw_version(uint32_t *major, uint32_t *minor);

/**
 * @brief Determine the number of cores enabled as per floorsweeping configuration.
 *
 * @return Number of enabled cores
 */
uint32_t tegrabl_ccplex_nvg_num_cores(void);

/**
 * @brief Determine the MPIDR value of a given logical CCPLEX CPU core.
 *
 * @param core Logical CCPLEX CPU core in the range [0,num_cores-1] as probed from
 * tegrabl_ccplex_nvg_num_cores.
 *
 * @return MPIDR of the given logical core.
 */
uint32_t tegrabl_ccplex_nvg_logical_to_mpidr(uint32_t core);

#endif /* INCLUED_TEGRABL_T194_CCPLEX_NVG_H */
