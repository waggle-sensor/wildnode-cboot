/*
* Copyright (c) 2017-2018, NVIDIA Corporation.  All Rights Reserved.
*
* NVIDIA Corporation and its licensors retain all intellectual property and
* proprietary rights in and to this software and related documentation.  Any
* use, reproduction, disclosure or distribution of this software and related
* documentation without an express license agreement from NVIDIA Corporation
* is strictly prohibited.
*/

/**
 * @file qual_engine.h
 *
 * Qual engine scrub interface
 *
 */

#ifndef INCLUDE_TEGRABL_QUAL_ENGINE_H
#define INCLUDE_TEGRABL_QUAL_ENGINE_H

#include <tegrabl_error.h>

/**
 * @brief Initialize/Scrub memory using MSS qual engine
 *
 * @param phy_addr_start Start address of the physical address range
 * @param size Page-aligned memory size to scrub
 *
 * @return TEGRABL_NO_ERROR if successful, else appropriate error
 */
tegrabl_error_t tegrabl_sdram_qual_engine_init(uint64_t phy_addr_start, uint64_t size);

/**
 * @brief Wait for Qual engine to be idle
 *
 * @return TEGRABL_NO_ERROR when idle and no errors; else appropriate error
 */
tegrabl_error_t tegrabl_sdram_qual_engine_wait_for_idle(void);

#endif /* INCLUDE_TEGRABL_QUAL_ENGINE_H */
