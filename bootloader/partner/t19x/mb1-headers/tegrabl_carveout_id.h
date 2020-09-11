/*
 * Copyright (c) 2016-2018, NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#ifndef INCLUDED_CARVEOUT_ID_H
#define INCLUDED_CARVEOUT_ID_H

#include <tegrabl_compiler.h>

/**
 * Tracks the base and size of the Carveout
 */

struct tegrabl_carveout_info {
	uint64_t base;
	uint64_t size;
	union {
		struct {
			uint64_t ecc_protected:1;
			uint64_t reserved:63;
		};
		uint64_t flags;
	};
};

/*macro carve_out_type*/
typedef uint32_t carve_out_type_t;
#define CARVEOUT_NONE 0U
#define CARVEOUT_GSC1 1U
#define CARVEOUT_NVDEC CARVEOUT_GSC1
#define CARVEOUT_GSC2 2U
#define CARVEOUT_WPR1 CARVEOUT_GSC2
#define CARVEOUT_GSC3 3U
#define CARVEOUT_WPR2 CARVEOUT_GSC3
#define CARVEOUT_GSC4 4U
#define CARVEOUT_TSECA CARVEOUT_GSC4
#define CARVEOUT_GSC5 5U
#define CARVEOUT_TSECB CARVEOUT_GSC5
#define CARVEOUT_GSC6 6U
#define CARVEOUT_BPMP CARVEOUT_GSC6
#define CARVEOUT_GSC7 7U
#define CARVEOUT_APE CARVEOUT_GSC7
#define CARVEOUT_GSC8 8U
#define CARVEOUT_SPE CARVEOUT_GSC8
#define CARVEOUT_GSC9 9U
#define CARVEOUT_SCE CARVEOUT_GSC9
#define CARVEOUT_GSC10 10U
#define CARVEOUT_APR CARVEOUT_GSC10
#define CARVEOUT_GSC11 11U
#define CARVEOUT_TZRAM CARVEOUT_GSC11
#define CARVEOUT_GSC12 12U
#define CARVEOUT_IPC_SE_TSEC CARVEOUT_GSC12
#define CARVEOUT_GSC13 13U
#define CARVEOUT_BPMP_RCE CARVEOUT_GSC13
#define CARVEOUT_GSC14 14U
#define CARVEOUT_BPMP_DMCE CARVEOUT_GSC14
#define CARVEOUT_GSC15 15U
#define CARVEOUT_SE_SC7 CARVEOUT_GSC15
#define CARVEOUT_GSC16 16U
#define CARVEOUT_BPMP_SPE CARVEOUT_GSC16
#define CARVEOUT_GSC17 17U
#define CARVEOUT_RCE CARVEOUT_GSC17
#define CARVEOUT_GSC18 18U
#define CARVEOUT_CPU_TZ_BPMP CARVEOUT_GSC18
#define CARVEOUT_GSC19 19U
#define CARVEOUT_VM_ENCRYPT1 CARVEOUT_GSC19
#define CARVEOUT_GSC20 20U
#define CARVEOUT_CPU_NS_BPMP CARVEOUT_GSC20
#define CARVEOUT_GSC21 21U
#define CARVEOUT_OEM_SC7 CARVEOUT_GSC21
#define CARVEOUT_GSC22 22U
#define CARVEOUT_IPC_SE_SPE_SCE_BPMP CARVEOUT_GSC22
#define CARVEOUT_GSC23 23U
#define CARVEOUT_SC7_RF CARVEOUT_GSC23
#define CARVEOUT_GSC24 24U
#define CARVEOUT_CAMERA_TASK CARVEOUT_GSC24
#define CARVEOUT_GSC25 25U
#define CARVEOUT_SCE_BPMP CARVEOUT_GSC25
#define CARVEOUT_GSC26 26U
#define CARVEOUT_CV CARVEOUT_GSC26
#define CARVEOUT_GSC27 27U
#define CARVEOUT_VM_ENCRYPT2 CARVEOUT_GSC27
#define CARVEOUT_GSC28 28U
#define CARVEOUT_HYPERVISOR CARVEOUT_GSC28
#define CARVEOUT_GSC29 29U
#define CARVEOUT_SMMU CARVEOUT_GSC29
#define CARVEOUT_GSC30 30U
#define CARVEOUT_GSC31 31U
#define CARVEOUT_MTS 32U
#define CARVEOUT_VPR 33U
#define CARVEOUT_TZDRAM 34U
#define CARVEOUT_MB2 35U
#define CARVEOUT_CPUBL 36U
#define CARVEOUT_MISC 37U
#define CARVEOUT_OS 38U
#define CARVEOUT_RCM_BLOB 39U
#define CARVEOUT_ECC_TEST 40U
#define CARVEOUT_RESERVED1 41U
#define CARVEOUT_RESERVED2 42U
#define CARVEOUT_RESERVED3 43U
#define CARVEOUT_RESERVED4 44U
#define CARVEOUT_RESERVED5 45U
#define	CARVEOUT_NUM 46U
#define CARVEOUT_FORCE32 0x7ffffffful

#endif
