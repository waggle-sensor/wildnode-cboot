/*
 * Copyright (c) 2016-2018, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited
 */

/*
 * Copyright (c) 2009, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef __FASTBOOT_H
#define __FASTBOOT_H

#include <tegrabl_error.h>

/* USB state returned during notify call */
typedef enum fastboot_usb_state {
	/* Dev not configured */
	FASTBOOT_USB_STATE_NOT_CONFIGURED = 0,
	/* Dev enumeration done and configured */
	FASTBOOT_USB_STATE_CONFIGURED = 1,
	/* Cable is not connected */
	FASTBOOT_USB_STATE_CABLE_NOT_CONNECTED = 2,
} fastboot_usb_state_t;

typedef enum fastboot_thread_status {
	RUNNING,
	PAUSED,
	TERMINATED,
} fastboot_thread_status_t;

/**
 * @brief initialize fastboot
 *
 * @return TEGRABL_NO_ERROR if success, specific error if fails
 */
tegrabl_error_t fastboot_init(void);

/**
 * @brief get fastboot thread status
 *
 * @return fastboot thread status
 */
fastboot_thread_status_t is_fastboot_running(void);

/**
 * @brief check whether to enter fastboot or not
 *
 * @param out boolean value returning true if fastboot can be entered,
 *  fasle if fastboot cannot be entered.
 * @return TEGRABL_NO_ERROR if success, specific error if fails
 */
tegrabl_error_t check_enter_fastboot(bool *out);

/**
 * @brief locks bootloader
 * @return TEGRABL_NO_ERROR if success, specific error if fails
 */
tegrabl_error_t fastboot_lock_bootloader(void);

/**
 * @brief unlocks bootloader
 * @return TEGRABL_NO_ERROR if success, specific error if fails
 */
tegrabl_error_t fastboot_unlock_bootloader(void);

/**
 * @brief Init and register cboot specific fastboot commands ( aka
 *        bootloader lock/unlock) to tegrabl fastboot module
 *
 * @return TEGRABL_NO_ERROR if success, else appropriate error code
 */
tegrabl_error_t tegrabl_fastboot_set_callbacks(void);

#endif
