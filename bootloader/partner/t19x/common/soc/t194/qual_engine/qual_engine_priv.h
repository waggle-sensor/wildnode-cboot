/*
* Copyright (c) 2017, NVIDIA Corporation.  All Rights Reserved.
*
* NVIDIA Corporation and its licensors retain all intellectual property and
* proprietary rights in and to this software and related documentation.  Any
* use, reproduction, disclosure or distribution of this software and related
* documentation without an express license agreement from NVIDIA Corporation
* is strictly prohibited.
*/
#ifndef INCLUDE_TEGRABL_QUAL_ENGINE_PRIV_H
#define INCLUDE_TEGRABL_QUAL_ENGINE_PRIV_H

/* NOTE: Below code is minimal extract from MSS code drop (nvboot_sdram_int.h) to support qual engine init */

#include <tegrabl_io.h>
#include <tegrabl_error.h>
#include <nvboot_util_int.h>

/**
  *
  * To allow for partial byte-enable writes to any memory region protected by DRAM ECC,
  * it needs to be initialized with known data first, so that MSS can generate and store corresponding ECC as well in DRAM.
  * This must be done after SDRAM initialization is completed and DRAM ECC is enabled.
  *
  * NvBootQualEngineDramInit does neceesary Qual Engine programming and starts DRAM initialization
  * for specified address range.
  *
  * NvBootQualEngineDramCheckStatus returns status of Qual Engine (0=IDLE, 1=BUSY), which can be used to
  * implement a polling mechanism.
  *
  */
NvU32 CodeDrop_NvBootQualEngineDramInit(NvU32 pageStart, NvU32 pageEnd, NvU32 isFpga);

NvU32 CodeDrop_NvBootQualEngineCheckStatus(void);

/* tegrabl wrappers to call code-drop/priv APIs */
static inline tegrabl_error_t tegrabl_qualengine_init_scrub(uint32_t page_start, uint32_t page_end,
															bool is_fpga)
{
	return CodeDrop_NvBootQualEngineDramInit(page_start, page_end, (uint32_t)is_fpga);
}

static inline bool tegrabl_qualengine_check_status(void)
{
	return (CodeDrop_NvBootQualEngineCheckStatus() > 0UL);
}

#endif /* INCLUDE_TEGRABL_QUAL_ENGINE_PRIV_H */
