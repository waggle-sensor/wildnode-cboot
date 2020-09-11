/*
 * Copyright (c) 2016-2018, NVIDIA CORPORATION. All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#ifndef BPMP_ABI_MACH_T194_RESET_H
#define BPMP_ABI_MACH_T194_RESET_H

/**
 * @file
 * @defgroup bpmp_reset_ids Reset ID's
 * @brief Identifiers for Resets controllable by firmware
 * @{
 */
#define TEGRA194_RESET_ACTMON			1
#define TEGRA194_RESET_ADSP_ALL			2
#define TEGRA194_RESET_AFI			3
#define TEGRA194_RESET_CAN1			4
#define TEGRA194_RESET_CAN2			5
#define TEGRA194_RESET_DLA0			6
#define TEGRA194_RESET_DLA1			7
#define TEGRA194_RESET_DPAUX			8
#define TEGRA194_RESET_DPAUX1			9
#define TEGRA194_RESET_DPAUX2			10
#define TEGRA194_RESET_DPAUX3			11
#define TEGRA194_RESET_EQOS			17
#define TEGRA194_RESET_GPCDMA			18
#define TEGRA194_RESET_GPU			19
#define TEGRA194_RESET_HDA			20
#define TEGRA194_RESET_HDA2CODEC_2X		21
#define TEGRA194_RESET_HDA2HDMICODEC		22
#define TEGRA194_RESET_HOST1X			23
#define TEGRA194_RESET_I2C1			24
#define TEGRA194_RESET_I2C10			25
#define TEGRA194_RESET_RSVD_26			26
#define TEGRA194_RESET_RSVD_27			27
#define TEGRA194_RESET_RSVD_28			28
#define TEGRA194_RESET_I2C2			29
#define TEGRA194_RESET_I2C3			30
#define TEGRA194_RESET_I2C4			31
#define TEGRA194_RESET_I2C6			32
#define TEGRA194_RESET_I2C7			33
#define TEGRA194_RESET_I2C8			34
#define TEGRA194_RESET_I2C9			35
#define TEGRA194_RESET_ISP			36
#define TEGRA194_RESET_MIPI_CAL			37
#define TEGRA194_RESET_MPHY_CLK_CTL		38
#define TEGRA194_RESET_MPHY_L0_RX		39
#define TEGRA194_RESET_MPHY_L0_TX		40
#define TEGRA194_RESET_MPHY_L1_RX		41
#define TEGRA194_RESET_MPHY_L1_TX		42
#define TEGRA194_RESET_NVCSI			43
#define TEGRA194_RESET_NVDEC			44
#define TEGRA194_RESET_NVDISPLAY0_HEAD0		45
#define TEGRA194_RESET_NVDISPLAY0_HEAD1		46
#define TEGRA194_RESET_NVDISPLAY0_HEAD2		47
#define TEGRA194_RESET_NVDISPLAY0_HEAD3		48
#define TEGRA194_RESET_NVDISPLAY0_MISC		49
#define TEGRA194_RESET_NVDISPLAY0_WGRP0		50
#define TEGRA194_RESET_NVDISPLAY0_WGRP1		51
#define TEGRA194_RESET_NVDISPLAY0_WGRP2		52
#define TEGRA194_RESET_NVDISPLAY0_WGRP3		53
#define TEGRA194_RESET_NVDISPLAY0_WGRP4		54
#define TEGRA194_RESET_NVDISPLAY0_WGRP5		55
#define TEGRA194_RESET_RSVD_56			56
#define TEGRA194_RESET_RSVD_57			57
#define TEGRA194_RESET_RSVD_58			58
#define TEGRA194_RESET_NVENC			59
#define TEGRA194_RESET_NVENC1			60
#define TEGRA194_RESET_NVJPG			61
#define TEGRA194_RESET_PCIE			62
#define TEGRA194_RESET_PCIEXCLK			63
#define TEGRA194_RESET_RSVD_64			64
#define TEGRA194_RESET_RSVD_65			65
#define TEGRA194_RESET_PVA0_ALL			66
#define TEGRA194_RESET_PVA1_ALL			67
#define TEGRA194_RESET_PWM1			68
#define TEGRA194_RESET_PWM2			69
#define TEGRA194_RESET_PWM3			70
#define TEGRA194_RESET_PWM4			71
#define TEGRA194_RESET_PWM5			72
#define TEGRA194_RESET_PWM6			73
#define TEGRA194_RESET_PWM7			74
#define TEGRA194_RESET_PWM8			75
#define TEGRA194_RESET_QSPI0			76
#define TEGRA194_RESET_QSPI1			77
#define TEGRA194_RESET_SATA			78
#define TEGRA194_RESET_SATACOLD			79
#define TEGRA194_RESET_SCE_ALL			80
#define TEGRA194_RESET_RCE_ALL			81
#define TEGRA194_RESET_SDMMC1			82
#define TEGRA194_RESET_RSVD_83			83
#define TEGRA194_RESET_SDMMC3			84
#define TEGRA194_RESET_SDMMC4			85
#define TEGRA194_RESET_SE			86
#define TEGRA194_RESET_SOR0			87
#define TEGRA194_RESET_SOR1			88
#define TEGRA194_RESET_SOR2			89
#define TEGRA194_RESET_SOR3			90
#define TEGRA194_RESET_SPI1			91
#define TEGRA194_RESET_SPI2			92
#define TEGRA194_RESET_SPI3			93
#define TEGRA194_RESET_SPI4			94
#define TEGRA194_RESET_TACH			95
#define TEGRA194_RESET_RSVD_96			96
#define TEGRA194_RESET_TSCTNVI			97
#define TEGRA194_RESET_TSEC			98
#define TEGRA194_RESET_TSECB			99
#define TEGRA194_RESET_UARTA			100
#define TEGRA194_RESET_UARTB			101
#define TEGRA194_RESET_UARTC			102
#define TEGRA194_RESET_UARTD			103
#define TEGRA194_RESET_UARTE			104
#define TEGRA194_RESET_UARTF			105
#define TEGRA194_RESET_UARTG			106
#define TEGRA194_RESET_UARTH			107
#define TEGRA194_RESET_UFSHC			108
#define TEGRA194_RESET_UFSHC_AXI_M		109
#define TEGRA194_RESET_UFSHC_LP_SEQ		110
#define TEGRA194_RESET_RSVD_111			111
#define TEGRA194_RESET_VI			112
#define TEGRA194_RESET_VIC			113
#define TEGRA194_RESET_XUSB_PADCTL		114
#define TEGRA194_RESET_NVDEC1			115
#define TEGRA194_RESET_PEX0_CORE_0		116
#define TEGRA194_RESET_PEX0_CORE_1		117
#define TEGRA194_RESET_PEX0_CORE_2		118
#define TEGRA194_RESET_PEX0_CORE_3		119
#define TEGRA194_RESET_PEX0_CORE_4		120
#define TEGRA194_RESET_PEX0_CORE_0_APB		121
#define TEGRA194_RESET_PEX0_CORE_1_APB		122
#define TEGRA194_RESET_PEX0_CORE_2_APB		123
#define TEGRA194_RESET_PEX0_CORE_3_APB		124
#define TEGRA194_RESET_PEX0_CORE_4_APB		125
#define TEGRA194_RESET_PEX0_COMMON_APB		126
#define TEGRA194_RESET_SLVSEC			127
#define TEGRA194_RESET_NVLINK			128
#define TEGRA194_RESET_PEX1_CORE_5		129
#define TEGRA194_RESET_PEX1_CORE_5_APB		130
#define TEGRA194_RESET_CVNAS			131
#define TEGRA194_RESET_CVNAS_FCM		132
#define TEGRA194_RESET_NVHS_UPHY		133
#define TEGRA194_RESET_NVHS_UPHY_PLL0		134
#define TEGRA194_RESET_NVHS_UPHY_L0		135
#define TEGRA194_RESET_NVHS_UPHY_L1		136
#define TEGRA194_RESET_NVHS_UPHY_L2		137
#define TEGRA194_RESET_NVHS_UPHY_L3		138
#define TEGRA194_RESET_NVHS_UPHY_L4		139
#define TEGRA194_RESET_NVHS_UPHY_L5		140
#define TEGRA194_RESET_NVHS_UPHY_L6		141
#define TEGRA194_RESET_NVHS_UPHY_L7		142
#define TEGRA194_RESET_NVHS_UPHY_PM		143
#define TEGRA194_RESET_DMIC5			144
#define TEGRA194_RESET_APE			145
#define TEGRA194_RESET_PEX_USB_UPHY		146
#define TEGRA194_RESET_PEX_USB_UPHY_L0		147
#define TEGRA194_RESET_PEX_USB_UPHY_L1		148
#define TEGRA194_RESET_PEX_USB_UPHY_L2		149
#define TEGRA194_RESET_PEX_USB_UPHY_L3		150
#define TEGRA194_RESET_PEX_USB_UPHY_L4		151
#define TEGRA194_RESET_PEX_USB_UPHY_L5		152
#define TEGRA194_RESET_PEX_USB_UPHY_L6		153
#define TEGRA194_RESET_PEX_USB_UPHY_L7		154
#define TEGRA194_RESET_PEX_USB_UPHY_L8		155
#define TEGRA194_RESET_PEX_USB_UPHY_L9		156
#define TEGRA194_RESET_PEX_USB_UPHY_L10		157
#define TEGRA194_RESET_PEX_USB_UPHY_L11		158
#define TEGRA194_RESET_PEX_USB_UPHY_PLL0	159
#define TEGRA194_RESET_PEX_USB_UPHY_PLL1	160
#define TEGRA194_RESET_PEX_USB_UPHY_PLL2	161
#define TEGRA194_RESET_PEX_USB_UPHY_PLL3	162
#define TEGRA194_RESET_MSSNVL			180

/** @} */

#endif
