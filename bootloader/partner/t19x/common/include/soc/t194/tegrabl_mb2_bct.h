/*
 * Copyright (c) 2017-2018, NVIDIA Corporation. All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

/**
 * @file tegrabl_mb2_bct.h
 */

#ifndef INCLUDED_TEGRABL_MB2_BCT_H
#define INCLUDED_TEGRABL_MB2_BCT_H

#include <stdint.h>
#include <tegrabl_compiler.h>

#define MB2_BCT_SIZE 1024

/**
 * @brief Defines bit allocation for settings of a feature
 * in mb2. If fields crosses uint64_t then move it to next data.
 */
struct tegrabl_mb2_feature_fields {
	union {
		uint64_t data1;
		struct {
			uint64_t disable_cpu_l2ecc:1;
			uint64_t enable_sce:1;
			uint64_t enable_rce:1;
			uint64_t enable_ape:1;
			uint64_t enable_combined_uart:1;
			uint64_t enable_ccplex_lock_step:1;
			uint64_t enable_emmc_send_cmd0_cmd1:1;
		};
	};
	union {
		uint64_t data2;
	};
	union {
		uint64_t data3;
	};
	union {
		uint64_t data4;
	};
};

TEGRABL_PACKED(
struct tegrabl_mb2_bct {
	uint32_t version;
	uint32_t padding;

	TEGRABL_DECLARE_ALIGNED(struct tegrabl_mb2_feature_fields feature_fields, 8);

	/* offset in respective carveout where corresponding binary is loaded */
	uint32_t cpubl_load_offset;
	uint32_t ape_fw_load_offset;
	uint32_t bpmp_fw_load_offset;
	uint32_t sce_fw_load_offset;
	uint32_t rce_fw_load_offset;

	/* offset of binary entry point from the load address */
	uint32_t cpubl_entry_offset;
	uint32_t ape_fw_entry_offset;
	uint32_t bpmp_fw_entry_offset;
	uint32_t sce_fw_entry_offset;
	uint32_t rce_fw_entry_offset;

	/* VA of the carveout in correspodning AST */
	uint32_t cpubl_carveout_ast_va;
	uint32_t ape_carveout_ast_va;
	uint32_t apr_carveout_ast_va;
	uint32_t bpmp_carveout_ast_va;
	uint32_t sce_carveout_ast_va;
	uint32_t rce_carveout_ast_va;
	uint32_t camera_task_carveout_ast_va;

	/* SPE's UART instance */
	uint32_t spe_uart_instance;

	/* Ensure that the total size of structure is 1024 bytes */
	uint8_t reserved[MB2_BCT_SIZE - 112];
}
);

/* Check if mb2 bct structure size matches with pre-defined size */
TEGRABL_COMPILE_ASSERT(sizeof(struct tegrabl_mb2_bct) == MB2_BCT_SIZE, "mb2 bct size mismatch");

#endif /* INCLUDED_TEGRABL_MB2_BCT_H */
