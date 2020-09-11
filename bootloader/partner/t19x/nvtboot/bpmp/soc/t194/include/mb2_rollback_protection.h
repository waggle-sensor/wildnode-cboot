/*
 * Copyright (c) 2018, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

/**
 * @file mb2_rollback_protection.h
 */

#include <tegrabl_rollback_protection.h>

#ifndef INCLUDED_MB2_ROLLBACK_PROTECTION_H
#define INCLUDED_MB2_ROLLBACK_PROTECTION_H

#define OEM_FW_RATCHET_IDX_CPU_BL 		(MAX_MB1_BINS + 1U)
#define OEM_FW_RATCHET_IDX_TOS			OEM_FW_RATCHET_IDX_CPU_BL + 1U
#define OEM_FW_RATCHET_IDX_EKS			OEM_FW_RATCHET_IDX_TOS + 1U
#define OEM_FW_RATCHET_IDX_BPMP_FW		OEM_FW_RATCHET_IDX_EKS + 1U
#define OEM_FW_RATCHET_IDX_BPMP_FW_DTB	OEM_FW_RATCHET_IDX_BPMP_FW + 1U
#define OEM_FW_RATCHET_IDX_SCE			OEM_FW_RATCHET_IDX_BPMP_FW_DTB + 1U
#define OEM_FW_RATCHET_IDX_RCE			OEM_FW_RATCHET_IDX_SCE + 1U
#define OEM_FW_RATCHET_IDX_APE			OEM_FW_RATCHET_IDX_RCE + 1U
#define OEM_FW_RATCHET_IDX_CPU_BL_DTB 	OEM_FW_RATCHET_IDX_APE + 1U
#define RESERVED_FIELDS					10U
#define MAX_MB2_BINS					(OEM_FW_RATCHET_IDX_CPU_BL_DTB - MAX_MB1_BINS + RESERVED_FIELDS)
#define MB2_BINS_END					(MAX_MB1_BINS + MAX_MB2_BINS)

#endif
