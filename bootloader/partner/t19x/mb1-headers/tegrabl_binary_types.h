/*
 * Copyright (c) 2016-2019, NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#ifndef INCLUDED_BINARY_TYPES_H
#define INCLUDED_BINARY_TYPES_H

#if defined (WIN32)
#define uint32_t NvU32
#endif

/*# Loaded by MB1 #*/
#define TYPE_MB1_BCT            0UL
#define TYPE_EARLY_SPEFW        1UL
#define TYPE_BLACKLIST_INFO     2UL
#define TYPE_MB2                3UL
#define TYPE_DRAM_ECC           4UL
#define TYPE_MTS_PREBOOT        5UL
#define TYPE_MTS_MCE            6UL
#define TYPE_MTS_PROPER         7UL
#define TYPE_IST_UCODE          8UL
#define TYPE_BPMP_IST           9UL
#define TYPE_IST_CONFIG         10UL
#define TYPE_XUSB               11UL
#define TYPE_FUSEBYPASS         12UL
#define TYPE_FSKP               13UL
#define TYPE_SC7_RESUME_FW      14UL
#define TYPE_MEM_BCT0           15UL
#define TYPE_MEM_BCT1           16UL
#define TYPE_MEM_BCT2           17UL
#define TYPE_MEM_BCT3           18UL
#define TYPE_MB2_APPLET         19UL
#define TYPE_RESERVED2          20UL
#define TYPE_MB1BCT_TYPE_MAX    21UL
/*# Loaded by BR #*/
#define TYPE_BR_BCT             22UL
#define TYPE_MB1                23UL
/*# Misc #*/
#define TYPE_SMD                24UL
#define TYPE_EXTENDED_SPE_FW    25UL
#define TYPE_SCE                26UL
#define TYPE_APE                27UL
#define TYPE_RCE                28UL
#define TYPE_BPMP_FW            29UL
#define TYPE_BPMP_FW_DTB        30UL
#define TYPE_EXTENDED_CAN       31UL
#define TYPE_TOS                32UL
#define TYPE_EKS                33UL
#define TYPE_RPB                34UL
#define TYPE_CPU_BL             35UL
#define TYPE_CPU_BL_DTB         36UL
#define TYPE_KERNEL             37UL
#define TYPE_KERNEL_DTB         38UL
#define TYPE_RECOVERY_KERNEL    39UL
#define TYPE_NCT                40UL
#define TYPE_KERNEL_DTBO        41UL
#define TYPE_MBR                42UL
#define TYPE_BOOTLOADER         43UL
#define TYPE_PARTITIONTABLE     44UL
#define TYPE_OSL                45UL
#define TYPE_GUEST_DTB          46UL
#define TYPE_GPH                47UL
#define TYPE_GUEST_IMG          48UL
#define TYPE_RAMDISK_IMG        49UL
#if defined(CONFIG_ENABLE_L4T_RECOVERY)
#define TYPE_RECOVERY_IMG       50UL
#define TYPE_RECOVERY_DTB       51UL
#define TYPE_KERNEL_BOOTCTRL    52UL
#define TYPE_PARTITIONTABLE_LEVEL2 53UL

#define TYPE_INVALID            54UL
#define TYPE_BLACKLIST_INFO_1   55UL
#define TYPE_MAX                56UL
#else
#define TYPE_PARTITIONTABLE_LEVEL2 50UL

#define TYPE_INVALID            51UL
#define TYPE_BLACKLIST_INFO_1   52UL
#define TYPE_MAX                53UL
#endif

/**
 * @brief Defines various binaries which can be
 * loaded via loader.
 */
	/*# Loaded by MB1 #*/
	/* MB1-BCT */
#define TEGRABL_BINARY_MB1_BCT TYPE_MB1_BCT
	/* MEM-BCT0 */
#define TEGRABL_BINARY_MEM_BCT0 TYPE_MEM_BCT0
	/* MEM-BCT1 */
#define TEGRABL_BINARY_MEM_BCT1 TYPE_MEM_BCT1
	/* MEM-BCT2 */
#define TEGRABL_BINARY_MEM_BCT2 TYPE_MEM_BCT2
	/* MEM-BCT3 */
#define TEGRABL_BINARY_MEM_BCT3 TYPE_MEM_BCT3
	/* SPE firmware */
#define TEGRABL_BINARY_EARLY_SPEFW TYPE_EARLY_SPEFW
	/* Blacklist Info */
#define TEGRABL_BINARY_BLACKLIST_INFO TYPE_BLACKLIST_INFO
	/* Blacklist Info 1*/
#define TEGRABL_BINARY_BLACKLIST_INFO_1 TYPE_BLACKLIST_INFO_1
	/* MB2 Binary */
#define TEGRABL_BINARY_MB2 TYPE_MB2
	/* DRAM ECC */
#define TEGRABL_BINARY_DRAM_ECC TYPE_DRAM_ECC
	/* Preboot MTS binary */
#define TEGRABL_BINARY_MTS_PREBOOT TYPE_MTS_PREBOOT
	/* MTS-MCE Binary */
#define TEGRABL_BINARY_MTS_MCE TYPE_MTS_MCE
	/* MTS-Proper Binary */
#define TEGRABL_BINARY_MTS_PROPER TYPE_MTS_PROPER
	/* Below is deprecated. TODO: Remove after dependencies are corrected */
#define TEGRABL_BINARY_MTS TYPE_MTS_PROPER
	/* IST-ucode */
#define TEGRABL_BINARY_IST_UCODE TYPE_IST_UCODE
	/* BPMP-IST */
#define TEGRABL_BINARY_BPMP_IST TYPE_BPMP_IST
	/* IST Config */
#define TEGRABL_BINARY_IST_CONFIG TYPE_IST_CONFIG
	/* XUSB Firmware */
#define TEGRABL_BINARY_XUSB TYPE_XUSB
	/* Fuse bypass */
#define TEGRABL_BINARY_FUSEBYPASS TYPE_FUSEBYPASS
	/* FSKP binary */
#define TEGRABL_BINARY_FSKP TYPE_FSKP
	/* sc7 resume fw / warmboot binary */
#define TEGRABL_BINARY_SC7_RESUME_FW TYPE_SC7_RESUME_FW
	/* FW - Reserved and Cardinality */
#define TEGRABL_BINARY_MB2_APPLET TYPE_MB2_APPLET
#define TEGRABL_BINARY_RESERVED2 TYPE_RESERVED2
#define TEGRABL_BINARY_MB1BCT_TYPE_MAX TYPE_MB1BCT_TYPE_MAX

	/*# Loaded by BR #*/
	/* Bootrom BCT */
#define TEGRABL_BINARY_BR_BCT TYPE_BR_BCT
	/* MB1 binary */
#define TEGRABL_BINARY_MB1 TYPE_MB1

	/*# Misc #*/
	/* SMD binary */
#define TEGRABL_BINARY_SMD TYPE_SMD
	/* Extended SPE-FW */
#define TEGRABL_BINARY_EXTENDED_SPE_FW TYPE_EXTENDED_SPE_FW
	/* SCE binary */
#define TEGRABL_BINARY_SCE TYPE_SCE
	/* APE binary */
#define TEGRABL_BINARY_APE TYPE_APE
	/* RCE Firmware */
#define TEGRABL_BINARY_RCE TYPE_RCE
	/* bpmp firware */
#define TEGRABL_BINARY_BPMP_FW TYPE_BPMP_FW
	/* bpmp firware dtb*/
#define TEGRABL_BINARY_BPMP_FW_DTB TYPE_BPMP_FW_DTB
	/* TSEC firmware */
#define TEGRABL_BINARY_EXTENDED_CAN TYPE_EXTENDED_CAN
	/* TLK image */
#define TEGRABL_BINARY_TOS TYPE_TOS
	/* eks image */
#define TEGRABL_BINARY_EKS TYPE_EKS
	/* RPB token */
#define TEGRABL_BINARY_RPB TYPE_RPB
	/* Tboot-CPU/ CPU bootloader */
#define TEGRABL_BINARY_CPU_BL TYPE_CPU_BL
	/* CPU bootloader DTB*/
#define TEGRABL_BINARY_CPU_BL_DTB TYPE_CPU_BL_DTB
	/* Kernel related */
#define TEGRABL_BINARY_KERNEL TYPE_KERNEL
#define TEGRABL_BINARY_KERNEL_DTB TYPE_KERNEL_DTB
#define TEGRABL_BINARY_RECOVERY_KERNEL TYPE_RECOVERY_KERNEL
#define TEGRABL_BINARY_NCT TYPE_NCT
#define TEGRABL_BINARY_KERNEL_DTBO TYPE_KERNEL_DTBO
	/* MBR */
#define TEGRABL_BINARY_MBR TYPE_MBR
	/* GP1 */
#define TEGRABL_BINARY_BOOTLOADER TYPE_BOOTLOADER
	/* Partition Table */
#define TEGRABL_BINARY_PARTITIONTABLE TYPE_PARTITIONTABLE
#define TEGRABL_BINARY_GPH TYPE_GPH
#define TEGRABL_BINARY_GUEST_IMG TYPE_GUEST_IMG
#define TEGRABL_BINARY_RAMDISK TYPE_RAMDISK_IMG

#if defined(CONFIG_ENABLE_L4T_RECOVERY)
#define TEGRABL_BINARY_RECOVERY_IMG TYPE_RECOVERY_IMG
#define TEGRABL_BINARY_RECOVERY_DTB TYPE_RECOVERY_DTB
#define TEGRABL_BINARY_KERNEL_BOOTCTRL TYPE_KERNEL_BOOTCTRL
#endif

	/* Level 2 partition table */
#define TEGRABL_BINARY_PARTITIONTABLE_LEVEL2 TYPE_PARTITIONTABLE_LEVEL2
	/* Invalid */
#define TEGRABL_BINARY_INVALID TYPE_INVALID
	/* cardinality */
#define TEGRABL_BINARY_MAX TYPE_MAX

typedef uint32_t tegrabl_binary_type_t;
/**
 * @brief Defines various imgtypes (aka binaries) which can be loaded via loader.
 * This is to have backward compatibility for projects using obselete 'enum tegrabl_imgtype'.
 */
	/*# Loaded by MB1 #*/
	/* MB1-BCT */
#define TEGRABL_IMGTYPE_MB1_BCT TYPE_MB1_BCT
	/* SPE firmware */
#define TEGRABL_IMGTYPE_EARLY_SPEFW TYPE_EARLY_SPEFW
	/* Blacklist Info */
#define TEGRABL_IMGTYPE_BLACKLIST_INFO TYPE_BLACKLIST_INFO
	/* MB2 Binary */
#define TEGRABL_IMGTYPE_MB2 TYPE_MB2
	/* DRAM ECC */
#define TEGRABL_IMGTYPE_DRAM_ECC TYPE_DRAM_ECC
	/* Preboot MTS binary */
#define TEGRABL_IMGTYPE_MTS_PREBOOT TYPE_MTS_PREBOOT
	/* MTS-MCE Binary */
#define TEGRABL_IMGTYPE_MTS_MCE TYPE_MTS_MCE
	/* MTS-Proper Binary */
#define TEGRABL_IMGTYPE_MTS_PROPER TYPE_MTS_PROPER
	/* IST-ucode */
#define TEGRABL_IMGTYPE_IST_UCODE TYPE_IST_UCODE
	/* BPMP-IST */
#define TEGRABL_IMGTYPE_BPMP_IST TYPE_BPMP_IST
	/* IST Config */
#define TEGRABL_IMGTYPE_IST_CONFIG TYPE_IST_CONFIG
	/* XUSB Firmware */
#define TEGRABL_IMGTYPE_XUSB TYPE_XUSB
	/* Fuse bypass */
#define TEGRABL_IMGTYPE_FUSEBYPASS TYPE_FUSEBYPASS
	/* FSKP binary */
#define TEGRABL_IMGTYPE_FSKP TYPE_FSKP
	/* sc7 resume fw / warmboot binary */
#define TEGRABL_IMGTYPE_SC7_RESUME_FW TYPE_SC7_RESUME_FW
	/* FW - Reserved and Cardinality */
#define TEGRABL_IMGTYPE_RESERVED1 TYPE_RESERVED1
#define TEGRABL_IMGTYPE_RESERVED2 TYPE_RESERVED2
#define TEGRABL_IMGTYPE_MB1BCT_TYPE_MAX TYPE_MB1BCT_TYPE_MAX

	/*# Loaded by BR #*/
	/* Bootrom BCT */
#define TEGRABL_IMGTYPE_BR_BCT TYPE_BR_BCT
	/* MB1 binary */
#define TEGRABL_IMGTYPE_MB1 TYPE_MB1

	/*# Misc #*/
	/* SMD binary */
#define TEGRABL_IMGTYPE_SMD TYPE_SMD
	/* Extended SPE-FW */
#define TEGRABL_IMGTYPE_EXTENDED_SPE_FW TYPE_EXTENDED_SPE_FW
	/* SCE binary */
#define TEGRABL_IMGTYPE_SCE TYPE_SCE
	/* APE binary */
#define TEGRABL_IMGTYPE_APE TYPE_APE
	/* RCE Firmware */
#define TEGRABL_IMGTYPE_RCE TYPE_RCE
	/* bpmp firware */
#define TEGRABL_IMGTYPE_BPMP_FW TYPE_BPMP_FW
	/* bpmp firware dtb*/
#define TEGRABL_IMGTYPE_BPMP_FW_DTB TYPE_BPMP_FW_DTB
	/* TSEC firmware */
#define TEGRABL_IMGTYPE_EXTENDED_CAN TYPE_EXTENDED_CAN
	/* TLK image */
#define TEGRABL_IMGTYPE_TOS TYPE_TOS
	/* eks image */
#define TEGRABL_IMGTYPE_EKS TYPE_EKS
	/* RPB token */
#define TEGRABL_IMGTYPE_RPB TYPE_RPB
	/* Tboot-CPU/ CPU bootloader */
#define TEGRABL_IMGTYPE_CPU_BL TYPE_CPU_BL
	/* CPU bootloader DTB*/
#define TEGRABL_IMGTYPE_CPU_BL_DTB TYPE_CPU_BL_DTB
	/* Kernel related */
#define TEGRABL_IMGTYPE_KERNEL TYPE_KERNEL
#define TEGRABL_IMGTYPE_KERNELDTB TYPE_KERNEL_DTB
#define TEGRABL_IMGTYPE_RECOVERY_KERNEL TYPE_RECOVERY_KERNEL
#define TEGRABL_IMGTYPE_NCT TYPE_NCT
#define TEGRABL_IMGTYPE_KERNEL_DTBO TYPE_KERNEL_DTBO
	/* MBR */
#define TEGRABL_IMGTYPE_MBR TYPE_MBR
	/* GP1 */
#define TEGRABL_IMGTYPE_BOOTLOADER TYPE_BOOTLOADER
	/* Partition Table */
#define TEGRABL_IMGTYPE_PARTITIONTABLE TYPE_PARTITIONTABLE
#define TEGRABL_IMGTYPE_GPH TYPE_GPH
#define TEGRABL_IMGTYPE_GUEST_IMG TYPE_GUEST_IMG
#define TEGRABL_IMGTYPE_RAMDISK TYPE_RAMDISK_IMG

#if defined(CONFIG_ENABLE_L4T_RECOVERY)
#define TEGRABL_IMGTYPE_RECOVERY_IMG TYPE_RECOVERY_IMG
#define TEGRABL_IMGTYPE_RECOVERY_DTB TYPE_RECOVERY_DTB
#define TEGRABL_IMGTYPE_KERNEL_BOOTCTRL TYPE_KERNEL_BOOTCTRL
#endif

	/* Level 2 partition table */
#define TEGRABL_IMGTYPE_PARTITIONTABLE_LEVEL2 TYPE_PARTITIONTABLE_LEVEL2

	/* Invalid */
#define TEGRABL_IMGTYPE_INVALID TYPE_INVALID
	/* cardinality */
#define TEGRABL_IMGTYPE_MAX TYPE_MAX
typedef uint32_t tegrabl_imgtype_t;
/**
 * @brief Binary identifier to indicate which copy of the binary needs
 * to be loaded
 */
/* macro tegrabl binary copy */
typedef uint32_t tegrabl_binary_copy_t;
#define TEGRABL_BINARY_COPY_PRIMARY 0
#define TEGRABL_BINARY_COPY_RECOVERY 1
	/* cardinality */
#define TEGRABL_BINARY_COPY_MAX 2

#endif
