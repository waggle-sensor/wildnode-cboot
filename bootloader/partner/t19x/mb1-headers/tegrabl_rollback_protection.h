/*
 * Copyright (c) 2018, NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#ifndef INCLUDED_TEGRABL_ROLLBACK_PROTECTION_H
#define INCLUDED_TEGRABL_ROLLBACK_PROTECTION_H

#include <stdint.h>
#include <stdbool.h>
#include <tegrabl_compiler.h>
#include <tegrabl_nvtypes.h>

/**
 * @brief Defines ratchet levels of binaries that need rollback protection
 */
struct ratchet {
	const uint8_t mb2;
	const uint8_t cpubl;
	const uint8_t bpmp_fw;
	const uint8_t tos;
	const uint8_t tsec;
	const uint8_t nvdec;
	const uint8_t srm;
	const uint8_t tsec_gsc_ucode;
	const uint8_t early_spe_fw;
	const uint8_t extended_spe_fw;
	const uint8_t xusb;
	const uint8_t sce;
	const uint8_t rce;
	const uint8_t ape;
};

#define OEM_FW_INDEX_RESERVED	0UL
#define OEM_FW_INDEX_SPEFW		1UL
#define OEM_FW_INDEX_DRAMECC	2UL
#define OEM_FW_INDEX_PREBOOT	3UL
#define OEM_FW_INDEX_MCE		4UL
#define OEM_FW_INDEX_MTS		5UL
#define OEM_FW_INDEX_MB2		6UL

#define MAX_MB1_BINS		10U

#define MAX_OEM_FW_RATCHET_INDEX	104U

TEGRABL_PACKED(
struct tegrabl_mb1bct_rollback {
	uint8_t version;
	struct ratchet min_ratchet;
	uint8_t reserved[20];
}
);

#endif /* INCLUDED_TEGRABL_ROLLBACK_PROTECTION_H */
