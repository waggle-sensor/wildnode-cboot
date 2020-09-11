/*
 * Copyright (c) 2018, NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#ifndef INCLUDED_TEGRABL_BOOT_TYPE_H
#define INCLUDED_TEGRABL_BOOT_TYPE_H

#include <stdint.h>
#include <tegrabl_compiler.h>
#include <tegrabl_carveout_id.h>

/* macro tegra boot type */
typedef uint32_t tegra_boot_type_t;
	/**< Specifies a default (unset) value. */
#define BOOT_TYPE_NONE 0U
	/**< Specifies a cold boot */
#define BOOT_TYPE_COLD 1U
	/**< Specifies the BR entered RCM */
#define BOOT_TYPE_RECOVERY 2U
	/**< Specifies UART boot (only available internal to NVIDIA) */
#define BOOT_TYPE_UART 3U
	/**< Specifies that the BR immediately exited for debugging */
	/**< purposes. This can only occur when NOT in ODM production mode, */
	/**< and when a special BOOT_SELECT value is set. */
#define BOOT_TYPE_EXITRCM 4U
#define BOOT_TYPE_FORCE32 0x7fffffffU

#endif /* INCLUDED_TEGRABL_BOOT_TYPE_H */

