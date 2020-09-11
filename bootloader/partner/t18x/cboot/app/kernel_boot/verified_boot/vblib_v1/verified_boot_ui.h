/*
 * Copyright (c) 2015-2018, NVIDIA Corporation.	All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#ifndef __VERIFIED_BOOT_UI__
#define __VERIFIED_BOOT_UI__

#include <sys/types.h>
#include <mincrypt/rsa.h>

/**
 * @brief Display UI for Red State
 *
 * @return NO_ERROR on success, else apt error
 */
status_t verified_boot_red_state_ui(void);

/**
 * @brief Display UI for Yellow State
 * @param boot_pub_key The public key used to verify the boot.img
 * @param dtb_pub_key The public key used to verify the kernel dtb
 *
 * @return NO_ERROR on success, else apt error
 */
status_t verified_boot_yellow_state_ui(struct rsa_public_key *boot_pub_key,
									   struct rsa_public_key *dtb_pub_key);

/**
 * @brief Display UI for Orange State
 *
 * @return NO_ERROR on success, else apt error
 */
status_t verified_boot_orange_state_ui(void);

#endif /* __VERIFIED_BOOT_UI__ */
