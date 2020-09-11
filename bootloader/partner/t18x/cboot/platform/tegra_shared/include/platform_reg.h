
/*
 * Copyright (c) 2014-2015, NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#ifndef __PLATFORM_TEGRA_REG_H
#define __PLATFORM_TEGRA_REG_H

#define PA_BASE_ADDRESS		0x70000000
#define GP_HIDREV			0x804
#define GP_EMU_REVID		0x860

#define PMC_BASE_ADDRESS	0x7000E400
#define PMC_CNTRL			0x00
#define PMC_SCRATCH20		0xa0
#define PMC_SCRATCH0		0x50
#define PMC_RESET_STATUS	0x1B4

#define MC_VIDEO_PROTECT_ALLOW_TZ_WRITE_ACCESS_ENABLED (1 << 1)

#define EMEM_ADR_CFG_DEV0	0x58
#define EMEM_ADR_CFG_DEV1	0x5c

/* Has the EMC project.h has been included before this file?
 *
 * Note: EMC project.h will later be removed from CBOOT.  After that
 * these defines can always be done. This only prevents re-definition
 * warning messages during the transition phase.
 */
#if !defined(SOME_PROJECT_H)

#define MC_BASE_ADDRESS		0x70019000

#define MC_EMEM_ADR_CFG_0	0x54
#define MC_EMEM_CFG_0		0x50
#define MC_VIDEO_PROTECT_REG_CTRL_0	0x650
#define EMC_FBIO_CFG5_0		0x104
#define EMC_FBIO_CFG7_0		0x584

#define EMC_BASE_ADDRESS	0x7001B000

#endif /* !defined(SOME_PROJECT_H) */

#endif /* __PLATFORM_TEGRA_REG_H */
