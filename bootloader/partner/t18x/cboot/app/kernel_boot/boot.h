/*
 * Copyright (c) 2014-2018, NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#ifndef __KERNEL_BOOT_H
#define __KERNEL_BOOT_H

#include <sys/types.h>
#include <tegrabl_error.h>
#include <linux_load.h>

#define ANDROID_MAGIC				"ANDROID!"
#define ANDROID_MAGIC_SIZE			8
#define ANDROID_BOOT_NAME_SIZE		16
#define ANDROID_BOOT_CMDLINE_SIZE	512
#define IGNORE_FASTBOOT_CMD			"ignorefastboot"

#define ZIMAGE_START_OFFSET			(0x80000UL)		/* @ 0.5 MB. */
#define KERNEL_IMAGE_RESERVE_SIZE	(0x1600000)		/* < 22 MB. */
#define DTB_MAX_SIZE				(0x400000)		/* < 4 MB. */
#define RAMDISK_START_OFFSET		(0x4100000)		/* @ 65 MB. */
#define RAMDISK_MAX_SIZE			(0xC800000)		/* < 200 MB. */
#define FDT_SIZE_BL_DT_NODES		(256 * 1024)     /* @ 256KB */

#define COMMAND_LINE_SIZE			1024

/**
 * Header fields of android image(boot.img)
 */
typedef struct {
	uint8_t magic[ANDROID_MAGIC_SIZE];
	uint32_t kernel_size;
	uint32_t kernel_addr;

	uint32_t ramdisk_size;
	uint32_t ramdisk_addr;

	uint32_t second_size;
	uint32_t second_addr;

	uint32_t tags_addr;
	uint32_t page_size;
	uint32_t unused[2];

	uint8_t name[ANDROID_BOOT_NAME_SIZE];
	uint8_t cmdline[ANDROID_BOOT_CMDLINE_SIZE];

	uint32_t id[8];
} android_boot_img;

/* macro debug console port */
	/* Debug console is undefined. */
#define DEBUG_CONSOLE_UNDEFINED 0

	/* No debug console is to be used and use hsport for debug UART */
#define DEBUG_CONSOLE_NONE 1

	/* ARM Debug Communication Channel (Dcc) port */
#define DEBUG_CONSOLE_DCC 2

	/* UARTs A to E */
#define DEBUG_CONSOLE_UARTA 3
#define DEBUG_CONSOLE_UARTB 4
#define DEBUG_CONSOLE_UARTC 5
#define DEBUG_CONSOLE_UARTD 6
#define DEBUG_CONSOLE_UARTE 7

	/* uSD to UARTA adapter is to be used for debug console */
#define DEBUG_CONSOLE_UARTSD 8

	/* No debug console is to be used and use lsport for debug */
#define DEBUG_CONSOLE_AUTOMATION 0x10

#define DEBUG_CONSOLE_NUM 0x11
typedef uint32_t debug_console_port;

/* macro kernel type */
#define ANDROID_KERNEL 0  /* boot.img */
#define RECOVERY_KERNEL 1 /* recovery.img */
#define FASTBOOT_KERNEL 2 /* boot.img loaded with fastboot boot command */
typedef uint32_t kernel_type;

/* macro android boot mode */
#define kernel_boot_mode_regular 0
#define kernel_boot_mode_charger 1
typedef uint32_t kernel_boot_mode;

/**
 * @brief load the kernel and boot the device
 *
 * @kernel tegrabl_kernel_bin struct
 *
 * @return ERR_type on error otherwise NO_ERROR
 */
tegrabl_error_t load_and_boot_kernel(struct tegrabl_kernel_bin *kernel);

/**
 * @brief define kernel type to load
 *
 * @mode type of the kernel - charger or android
 */
void set_kernel_boot_mode(kernel_boot_mode mode);

/**
 * @brief returns kernel type to be loaded
 *
 * @return boot mode - charger or android
 */
kernel_boot_mode get_kernel_boot_mode(void);

#endif
