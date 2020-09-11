/*
 * Copyright (c) 2017, NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#ifndef TEGRABL_RAMDUMP_H
#define TEGRABL_RAMDUMP_H

/* ramdump state definitions */
typedef uint32_t tegrabl_ramdump_state_t;
#define TEGRABL_RAMDUMP_STATE_CLEAN 0xf000caf3U
#define TEGRABL_RAMDUMP_STATE_DIRTY 0x2badfaceU
#define TEGRABL_RAMDUMP_STATE_DIRTY_DUMP 0xdeadbeefU
#define TEGRABL_RAMDUMP_STATE_WDT_DUMP 0x2badbeefU

/*
 * @brief update ramdump state
 *
 * @param state Ramdump state flag to be set
 *
 * @return TEGRABL_NO_ERROR if success, or error code
 */
tegrabl_error_t tegrabl_ramdump_set_state(tegrabl_ramdump_state_t state);

/*
 * @brief check ramdump request state by checking ramdump flag and reset source
 *
 * @param ramdump_req Return true if ramdump is required or false
 *
 * @return TEGRABL_NO_ERROR if success, or error code
 */
tegrabl_error_t tegrabl_ramdump_check_request(bool *ramdump_req);

/*
 * @brief unmap ast bpmp mapping region
 *
 */
void tegrabl_ramdump_ast_unmap_bpmp_region(void);

/*
 * @brief BPMP ast map for ramdump
 *
 * @param mapped_va Virtual address mapped to
 * @param offset_pa Physical address
 * @param mapped_size mapped memory size
 *
 * @return TEGRABL_NO_ERROR if success, or error code
 */
tegrabl_error_t tegrabl_ramdump_ast_map_bpmp(uint64_t mapped_va,
						uint64_t offset_pa, uint64_t mapped_size);

#endif /* TEGRABL_RAMDUMP_H */
