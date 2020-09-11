/*
 * Copyright (c) 2017-2019, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

/**
 * @file qual_engine.c
 *
 * Qual engine scrub interface
 *
 */

#define MODULE TEGRABL_ERR_QUAL_ENGINE

#include <qual_engine_priv.h>
#include <qual_engine.h>
#include "build_config.h"
#include <tegrabl_error.h>
#include <tegrabl_debug.h>
#include <tegrabl_utils.h>
#include <tegrabl_addressmap.h>
#include <tegrabl_soc_misc.h>
#include <tegrabl_timer.h>

#define QUAL_PAGE_SIZE_SHIFT (14)

/* FIXME: FPGA: 10minutes. Optimize this */
#define SCRUB_TIMEOUT_FPGA (10 * 60 * 1000 * 1000)
/* FIXME: Silicon: 15seconds. Optimize this */
#define SCRUB_TIMEOUT (15 * 1000 * 1000)

tegrabl_error_t tegrabl_sdram_qual_engine_init(uint64_t phy_addr_start, uint64_t size)
{
	uint32_t page_start;
	uint32_t page_end;
	uint32_t num_pages;
	tegrabl_error_t err = TEGRABL_NO_ERROR;
	bool is_fpga;

	if ((MOD_LOG2(phy_addr_start, QUAL_PAGE_SIZE_SHIFT) != 0ULL) ||
		(MOD_LOG2(size, QUAL_PAGE_SIZE_SHIFT) != 0ULL)) {
		pr_warn("start address / size should be aligned to page size.\n");
		err = TEGRABL_ERR_BAD_ADDRESS;
	} else {
		page_start = (uint32_t)DIV_FLOOR_LOG2(phy_addr_start, QUAL_PAGE_SIZE_SHIFT);
		num_pages = (uint32_t)DIV_FLOOR_LOG2(size, QUAL_PAGE_SIZE_SHIFT);
		page_end = ((page_start + num_pages) - 1UL);

		pr_trace("scrub: page_start = 0x%08x, page_end = 0x%08x\n",
				page_start, page_end);
		is_fpga = tegrabl_is_fpga();

		err = tegrabl_qualengine_init_scrub(page_start, page_end, is_fpga);
		if (err != TEGRABL_NO_ERROR) {
			pr_error("qual scrub failed\n");
			err = TEGRABL_ERROR(TEGRABL_ERR_NOT_INITIALIZED, 0);
		}
	}
	return err;
}

tegrabl_error_t tegrabl_sdram_qual_engine_wait_for_idle(void)
{
	tegrabl_error_t error = TEGRABL_NO_ERROR;
	time_t start_time_us = tegrabl_get_timestamp_us();
	time_t timeout_us;

	if (tegrabl_is_fpga()) {
		timeout_us = SCRUB_TIMEOUT_FPGA;
	} else {
		timeout_us = SCRUB_TIMEOUT;
	}

	/* status of Qual Engine (0=IDLE, 1=BUSY) */
	while (tegrabl_qualengine_check_status()) {
		if ((tegrabl_get_timestamp_us() - start_time_us) > timeout_us) {
			pr_error("Qual engine NOT idle post timeout\n");
			error = TEGRABL_ERROR(TEGRABL_ERR_TIMEOUT, 0);
			break;
		}
		/* delay before next check */
		tegrabl_udelay(1ULL);
	}
	return error;
}

