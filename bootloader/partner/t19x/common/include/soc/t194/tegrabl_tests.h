/*
 * Copyright (c) 2016-2017, NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#ifndef INCLUDED_TEGRABL_TESTS_H
#define INCLUDED_TEGRABL_TESTS_H

#include <tegrabl_carveout_id.h>
#include <stdbool.h>

/**
 * @brief Defines the number of access register for a carveout.
 */
#define CARVEOUT_NUM_ACCESS_REGISTERS 8

/* macro m groups */
typedef int32_t m_groups_t;
#define NONE_GRP -1
#define CPU_TZ0 0
#define CPU_G1 1
#define BPMP_G2 2
#define FLCN_G3 3
#define SPE_G4 4
#define SCE_G5 5
#define DPMU_G6 6
#define APE_G7 7

/* FIXME */
/* macro permissions */
typedef uint32_t permissons_t;
#define NONE 0
#define READ 1
#define WRITE 2

extern uint32_t tboot_sysram_scrub_test_result, tboot_btcm_scrub_test_result;

extern volatile uint32_t tboot_data_abort_bypass;

/**
 * @brief Defines data structure which holds access permissions for
 * carveouts.
 */
struct carveout_access_permissions {
	/* Id of the carveout */
	carve_out_type_t carveout_id;
	/* Offset of cfg0 register of carveout */
	uint32_t cfg0_offset;
	/* CFG0 value */
	uint32_t cfg0_value;
	/* Values for access0, access1, access2, access3, access4, access5
	 * registers of the carveout.
	 */
	uint32_t access_values[CARVEOUT_NUM_ACCESS_REGISTERS];
};

/**
 * @brief Sets the bypass data abort flag.
 * Only first data abort after this call will be
 * bypassed and execution will resume with next
 * instruction of instruction which caused data abort.
 * Data aborts after first data abort will not be
 * bypassed untill this function is called again.
 */
static inline void tegrabl_bypass_data_abort(void)
{
	tboot_data_abort_bypass = 1;
}

/**
 * @brief Clears the bypass abort flag.
 */
static inline void tegrabl_handle_data_abort(void)
{
	tboot_data_abort_bypass = 0;
}

/**
 * @brief Checks if any data abort occurred after
 * tegrabl_bypass_data_abort() and before tegrabl_handle_data_abort().
 *
 * @return true if data abort occurred else false.
 */
static inline bool tegrabl_is_data_abort_occurred(void)
{
	return tboot_data_abort_bypass == 2;
}

/**
 * @brief run wdtcr configuration test
 */
void tegrabl_wdtcr_configuration_test(void);

/**
 * @brief Run predefined set of mb1 tests from bpmpbl.
 */
void tegrabl_run_bpmpbl_mb1_tests(void);

/**
 * @brief Runs predfined set of mb1 tests from cpubl.
 */
void tegrabl_run_cpubl_mb1_tests(void);

/**
 * @brief Run predefined set of bpmpbl tests.
 */
void tegrabl_run_bpmpbl_tests(void);

/**
 * @brief Reads access permissions and verfies it agains passed
 * permission settings.
 *
 * @param permissions Array of access permissions of carveouts to be tested
 * @param array_size Number of carveouts in array
 */
void tegrabl_test_carveout_access_permissions(
		struct carveout_access_permissions *permissions, uint32_t array_size);

/**
 * @brief tests the access(read/write) capability of all the
 *        carveout memories for given master based on the access register value
 *        corresponding to the master
 */
void tegrabl_test_carveout_rw(void);

/**
 * @brief run tests to check if oem carveouts are locked by mb2.
 */
void tegrabl_test_oem_carveout_lock(void);

/**
 * @brief Checks access permissions of oem carveouts configured by mb2
 */
void tegrabl_test_oem_carveout_access_permissions(void);

/**
 * @brief Tests memory operations and print throughput.
 */
void tegrabl_mem_operation_test(void);

/**
 * @brief Verify qspi driver and print throughput
 */
void tegrabl_qspi_test(void);

/**
 * @brief Test binaries are divided into groups.
 * Get the group type in which test binary belongs.
 *
 * @return Group of test binary
 */
m_groups_t tegrabl_tests_get_master_group(void);

/**
 * @brief Set the group in which test binary belongs.
 *
 * @param group Group type
 */
void tegrabl_tests_set_master_group(m_groups_t group);

/**
 * @brief Verifies read/write permissions for registers in aon block.
 */
void test_rw_aon_block_reg(void);

/**
 * @brief Verifies access permissions for SysRAM & DRAM.
 */
void test_sysram_dram_access(void);

#endif /* INCLUDED_TEGRABL_TESTS_H */
