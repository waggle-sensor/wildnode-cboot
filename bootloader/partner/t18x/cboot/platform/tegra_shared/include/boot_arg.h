/*
 * Copyright (c) 2014 - 2017, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited
 */

#ifndef __BOOT_ARG_H
#define __BOOT_ARG_H

#include <sys/types.h>

/*---------------------------------------------------------------------- */
/* NOTE: The contents of this file should always be in sync with nvtboot */
/*---------------------------------------------------------------------- */

/**
  * Maximum possible wb0 size(bytes)
  */
#define WBO_MAXIMUM_CODE_SIZE  4096

 /**
  * Maximum possible EKS size(bytes)
  */
#define EKS_MAXIMUM_CODE_SIZE  512

 /**
  * Maximum possible DTB size(bytes)
  */
#define DTB_MAXIMUM_CODE_SIZE  (200*1024)

 /**
  * Defines the memory layout type to be queried for its base/size
  */
/* macro mem layout */
	/* Invalid entry */
#define MEM_LAYOUT_NONE 0x0

	/* Primary memory layout */
#define MEM_LAYOUT_PRIMARY 0x1

	/* Extended memory layout */
#define MEM_LAYOUT_EXTENDED 0x2

	/* VPR carveout information */
#define MEM_LAYOUT_VPR 0x3

	/* TSEC carveout information */
#define MEM_LAYOUT_TSEC 0x4

	/* SECUREOS carveout information */
#define MEM_LAYOUT_SECUREOS 0x5

	/* LP0 carveout information */
#define MEM_LAYOUT_LP0 0x6

	/* NCK carveout information */
#define MEM_LAYOUT_NCK 0x7

	/* DEBUG carveout information */
#define MEM_LAYOUT_DEBUG 0x8

	/* BPMP FW carveout information */
#define MEM_LAYOUT_BPMPFW 0x9

	/* RAMDump carveout information */
#define MEM_LAYOUT_NVDUMPER 0xA

	/* carveout for GSC1 */
#define MEM_LAYOUT_GSC1 0xB

	/* carveout for GSC2 */
#define MEM_LAYOUT_GSC2 0xC

	/* carveout for GSC3 */
#define MEM_LAYOUT_GSC3 0xD

	/* carveout for GSC4 */
#define MEM_LAYOUT_GSC4 0xE

	/* carveout for GSC5 */
#define MEM_LAYOUT_GSC5 0xF

	/* Total types of memory layout */
#define MEM_LAYOUT_NUM 0x10
typedef uint32_t mem_layout;

/**
 * Reason for poweron
 */
/* macro poweron source */
#define poweron_source_pmcpor 0
#define poweron_source_apwdt_rst 1
#define poweron_source_sensor_rst 2
#define poweron_source_sw_rst 3
#define poweron_source_deepsleep_rst 4
#define poweron_source_aotag_rst 5
#define poweron_source_onkey 6
#define poweron_source_resetkey 7
#define poweron_source_usbhotplug 8
#define poweron_source_rtcalarm 9
#define poweron_source_pmuwdt 10
#define poweron_source_unknown 0x7fffffff
typedef uint32_t poweron_source;

 /**
  * Tracks the base and size of the Carveout
  */
typedef struct {
	/* Base of the carveout */
	 uint64_t base;

	/* Size of the carveout */
	 uint64_t size;
} carveout_info;

 /**
  * Stores the information related to memory params
  */
typedef struct {
	/* Size and Base of the carveout */
	carveout_info car_info[MEM_LAYOUT_NUM];

	/* DRAM bottom information */
	uint64_t dram_carveout_bottom;
} mem_info;

/**
 * Stores the information related to memory params
 */
/* macro dram memory type */
    /// Specifies the memory type to be undefined
#define dram_memory_Type_None 0

    /// Specifies the memory type to be LPDDR2 SDRAM
#define dram_memory_type_lpddr2 1

    /// Specifies the memory type to be DDR3 SDRAM
#define dram_memory_type_ddr3 2

#define dram_memory_type_lpddr4 3

#define dram_memory_type_num 4
#define dram_memory_type_force32 0x7FFFFFF
typedef uint32_t dram_memory_type;

/**
 * Defines the BootDevice Type available
 */
/* macro fuse boot device */
	/* boot device is sdmmc */
#define BOOT_DEVICE_SDMMC 0

	/* boot device is snor */
#define BOOT_DEVICE_SNOR_FLASH 1

	/* boot device is spi */
#define BOOT_DEVICE_SPI_FLASH 2

	/* boot device is sata */
#define BOOT_DEVICE_SATA 3

	/* boot device is usb3 */
#define BOOT_DEVICE_USB3 4

	/*reserved boot device */
#define BOOT_DEVICE_RESVD 5

	/* max number of boot devices */
#define BOOT_DEVICE_MAX 6
typedef uint32_t fuse_boot_device;

/**
 * Device type and instance
 */
typedef struct {
	/* boot of storage device type */
	fuse_boot_device device;

	/* instance of the device */
	uint32_t instance;
} dev_info;

/**
 * Holds the chip's unique ID.
 */
typedef struct {
	/* Bytes 3-0 of the ecid */
	uint32_t ecid_0;

	/* Bytes 7-4 of ecid */
	uint32_t ecid_1;

	/* Bytes 11-8 of ecid */
	uint32_t ecid_2;

	/* Bytes 15-12 of ecid */
	uint32_t ecid_3;
} unique_chip;

/**
 * Defines the board information stored on eeprom
 */
typedef struct {
	/* Holds the version number. */
	uint16_t version;

	/* Holds the size of 2 byte value of \a BoardId data that follows. */
	uint16_t size;

	/* Holds the board number. */
	uint16_t board_id;

	/* Holds the SKU number. */
	uint16_t sku;

	/* Holds the FAB number. */
	uint8_t fab;

	/* Holds the part revision. */
	uint8_t revision;

	/* Holds the minor revision level. */
	uint8_t minor_revision;

	/* Holds the memory type. */
	uint8_t mem_type;

	/* Holds the power configuration values. */
	uint8_t power_config;

	/* Holds the SPL reworks, mech. changes. */
	uint8_t misc_config;

	/* Holds the modem bands value. */
	uint8_t modem_bands;

	/* Holds the touch screen configs. */
	uint8_t touch_screen_configs;

	/* Holds the display configs. */
	uint8_t display_configs;
} board_info;

/**
 * Specifies different modes of booting.
 */
/* macro boot mode */
	/* Specifies a default (unset) value. */
#define BOOT_MODE_NONE 0

	/* Specifies a cold boot */
#define BOOT_MODE_COLD 1

	/* Specifies the BR entered RCM */
#define BOOT_MODE_RECOVERY 2

	/* Specifies UART boot (only available internal to NVIDIA) */
#define BOOT_MODE_UART 3
typedef uint32_t boot_mode;

/**
 * Specifies different operating modes.
 */
/* macro opmode type */
	/* Invalid Operating mode */
#define OPMODE_NONE 0

	/* Preproduction mode */
#define OPMODE_PRE_PRODUCTION 1

	/* failure analysis mode */
#define OPMODE_FAILURE_ANALYSIS 2

	/* nvproduction mode */
#define OPMODE_NV_PRODUCTION 3

	/* non-secure mode */
#define OPMODE_ODM_PRODUCTION_NON_SECURE 4

	/* sbk mode */
#define OPMODE_ODM_PRODUCTION_SECURE_SBK 5

	/* pkc mode */
#define OPMODE_ODM_PRODUCTION_SECURE_PKC 6
typedef uint32_t opmode_type;

typedef struct {
	/* Active BCT block */
	uint32_t active_bct_block;

	/* Active BCT sector */
	uint32_t active_bct_sector;

} active_bct_info;

/**
 * Specifies shared structure.
 */
typedef struct {
	/* Revision of common structure */
	uint32_t revision;

	/* CRC of this structure */
	uint32_t crc;

	/* Early Uart Address */
	uint64_t early_uart_addr;

	/* Memory layout Information */
	mem_info  params;

	/* Poweron Source */
	poweron_source pwron_src;

	/* Pmic shutdown reason */
	uint8_t pmic_reset_reason;

	/* Information about bootdevice */
	dev_info boot_dev;

	/* Information about storage device */
	dev_info storage_dev;

	/* Unique Chip Id */
	unique_chip uid;

	dram_memory_type dram_type;

	/* Processor Board Details */
	board_info proc_board_details;

	/* Pmu Board Details */
	board_info pmu_board_details;

	/* Display BoardDetails */
	board_info display_board_details;

	/* Size of Bct in bytes */
	uint32_t bct_size;

	/* Size of start Offset of tboot in bytes */
	uint64_t tboot_start_offset_inbytes;

	/* Size of boot device block size in  bytes */
	uint32_t block_size_inbytes;

	/* Size of boot device page size in bytes */
	uint32_t page_size_inbytes;

	/* Boot mode can be cold boot, uart or recovery */
	boot_mode boot_type;

	/* Operating Mode of device */
	opmode_type op_mode;

	/* Size of start Offset of mts preboot in bytes */
	uint64_t mts_start_offset_inbytes;

	/* Info of the bct being used */
	active_bct_info active_bct;

	/* Bootloader dtb load address */
	uint32_t bl_dtb_load_addr;

	/* Kernel dtb load address */
	uint32_t kernel_dtb_load_addr;

	/* powerkey long press state */
	uint32_t powerkey_long_press_state;

	/* Reserved Bytes at the end */
	uint8_t reserved[252];
} boot_arg;


/**
 * Specifies enum for accessing shared information.
 */
/* macro boot arg type */
	/* get revision */
#define BOOT_ARG_REVISION 0x0

	/* get crc of shared structure */
#define BOOT_ARG_CRC 0x1

	/* get early uart base */
#define BOOT_ARG_UART_ADDR 0x2

	/* get dram botton size */
#define BOOT_ARG_DRAM_BOTTOM 0x3

	/* get primary mem layout */
#define BOOT_ARG_MEM_LAYOUT_PRIMARY 0x4

	/* get extended mem layout */
#define BOOT_ARG_MEM_LAYOUT_EXTENDED 0x5

	/* get vpr mem layout */
#define BOOT_ARG_MEM_LAYOUT_VPR 0x6

	/* get tsec mem layout */
#define BOOT_ARG_MEM_LAYOUT_TSEC 0x7

	/* get secureos mem layout */
#define BOOT_ARG_MEM_LAYOUT_SECUREOS 0x8

	/* get lp0 mem layout */
#define BOOT_ARG_MEM_LAYOUT_LP0 0x9

	/* get xusb mem layout */
#define BOOT_ARG_MEM_LAYOUT_XUSB 0xA

	/* get nck mem layout */
#define BOOT_ARG_MEM_LAYOUT_NCK 0xB

	/* get debug mem layout */
#define BOOT_ARG_MEM_LAYOUT_DEBUG 0xC

	/* get poweron source */
#define BOOT_ARG_POWERON_SRC 0xD

	/* get poweron source */
#define BOOT_ARG_PMIC_RESET_REASON 0xE

	/* get secondary boot device info */
#define BOOT_ARG_BOOT_DEV 0xF

	/* get storage boot device info */
#define BOOT_ARG_STORAGE_DEV 0x10

	/* get uid of the chip */
#define BOOT_ARG_UID 0x11

	/* get memory type of dram */
#define BOOT_ARG_MEM_TYPE 0x12

	/* get processor board id */
#define BOOT_ARG_PROC_BOARD 0x13

	/* get pmu board id */
#define BOOT_ARG_PMU_BOARD 0x14

	/* get display board id */
#define BOOT_ARG_DISPLAY_BOARD 0x15

	/* get bct size in bytes */
#define BOOT_ARG_BCT_SIZE 0x16

	/* get tboot start offset */
#define BOOT_ARG_TBOOT_START 0x17

	/* get block sie in bytes */
#define BOOT_ARG_BLOCK_SIZE 0x18

	/* get page size in bytes */
#define BOOT_ARG_PAGE_SIZE 0x19

	/* get boot type  (cold, uart or recovery) */
#define BOOT_ARG_BOOT_TYPE 0x1A

	/* get the operating mode */
#define BOOT_ARG_OPMODE 0x1B

	/* get the Mts Start Address */
#define BOOT_ARG_MTS_START 0x1C
	/* get Bpmp FW mem layout */
#define BOOT_ARG_MEM_LAYOUT_BPMPFW 0x1D

	/* get RAM dump mem layout */
#define BOOT_ARG_MEM_LAYOUT_NVDUMPER 0x1E

	/* get Active BCT info */
#define BOOT_ARG_ACTIVE_BCT 0x1F

	/* get bl dtb load address */
#define BOOT_ARG_BL_DTB_LOAD_ADDR 0x20

	/* get kernel dtb load address */
#define BOOT_ARG_KERNEL_DTB_LOAD_ADDR 0x21

	/* get powerkey long press state */
#define BOOT_ARG_POWERKEY_LONG_PRESS_STATE 0x22
typedef uint32_t boot_arg_type;

/** @brief Validates CRC of the shared structure.
 *
 *  @param Void.
 *
 *  @return Void.
 */
void boot_arg_validation(void);

/** @brief Prints the contents of the shared structure.
 *
 *  @param Void.
 *
 *  @return Void.
 */
void  boot_arg_print_info(void);

/** @brief Queries the contents of the members of shared structure.
 *
 *  @param type Type of the input boot argument
 *              whose contents is to be queried.
 *
 *  @param buf Pointer to store contents of the boot argument.
 *  @param size Pointer to store size of the boot argument.
 *
 *  @return NO_ERROR if query contents is successful.
 */
status_t boot_arg_query_info(boot_arg_type type,
							 void *buf, uint32_t *size);
#endif /* __BOOT_ARG_H */

