/*
 * Copyright (c) 2017-2019, NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */
#define MODULE TEGRABL_ERR_CLK_RST

#include <tegrabl_error.h>
#include <tegrabl_module.h>
#include <tegrabl_debug.h>
#include <tegrabl_clock.h>
#include <tegrabl_bpmp_fw_interface.h>
#include <tegrabl_qspi.h>
#include <tegrabl_soc_misc.h>
#include <tegrabl_soc_clock.h>

#include <bpmp_abi.h>
#include <clk-t194.h>
#include <reset-t194.h>
#include <powergate-t194.h>

#define BPMP_CLK_CMD(cmd, id) ((id) | ((cmd) << 24))
#define MODULE_NOT_SUPPORTED (TEGRA194_MAX_CLK_ID)
#define MODULE_NOT_SUPPORTED_SKIPPED (TEGRA194_MAX_CLK_ID + 1)
#define MAX_PARENTS 16

#define HZ_1K       (1000)
#define KHZ_P4M     (400)
#define KHZ_13M     (13000)
#define KHZ_19P2M   (19200)
#define KHZ_12M     (12000)
#define KHZ_26M     (26000)
#define KHZ_16P8M   (16800)
#define KHZ_38P4M   (38400)
#define KHZ_48M     (48000)

#define RATE_XUSB_DEV_KHZ (102000)
#define RATE_XUSB_SS_KHZ  (120000)

#define NUM_USB_CLKS     15
#define NUM_USB_TRK_CLKS 3

#define NAME_INDEX          0

#define USB_RATE_INDEX      1
#define USB_PARENT_INDEX    2

#define NUM_UFS_CLKS        18
#define NUM_UFS_RSTS        8
#define UFS_RATE_INDEX      1
#define UFS_PARENT_INDEX    2

#define TEGRA194_MAX_CLK_ID 500

static uint32_t pllc4_muxed_rate;

static uint32_t tegrabl_pllid_to_bpmp_pllid[TEGRABL_CLK_PLL_ID_MAX] = {
		[TEGRABL_CLK_PLL_ID_PLLP] = TEGRA194_CLK_PLLP,
		[TEGRABL_CLK_PLL_ID_PLLC4] = TEGRA194_CLK_PLLC4,
		[TEGRABL_CLK_PLL_ID_PLLD] = TEGRA194_CLK_PLLD,
		[TEGRABL_CLK_PLL_ID_PLLD2] = TEGRA194_CLK_PLLD2,
		[TEGRABL_CLK_PLL_ID_PLLD3] = TEGRA194_CLK_PLLD3,
		[TEGRABL_CLK_PLL_ID_PLLDP] = TEGRA194_CLK_PLLDP,
		[TEGRABL_CLK_PLL_ID_PLLE] = TEGRA194_CLK_PLLE,
		[TEGRABL_CLK_PLL_ID_PLLM] = TEGRA194_MAX_CLK_ID,
		[TEGRABL_CLK_PLL_ID_SATA_PLL] = TEGRA194_CLK_PLLP_OUT0,
		[TEGRABL_CLK_PLL_ID_UTMI_PLL] = TEGRA194_CLK_UTMIPLL,
		[TEGRABL_CLK_PLL_ID_XUSB_PLL] = TEGRA194_CLK_PLLP_OUT0,
		[TEGRABL_CLK_PLL_ID_AON_PLL] = TEGRA194_CLK_PLLAON,
		[TEGRABL_CLK_PLL_ID_PLLDISPHUB] = TEGRA194_CLK_PLLDISPHUB,
		[TEGRABL_CLK_PLL_ID_PLL_NUM] = TEGRA194_MAX_CLK_ID,
		[TEGRABL_CLK_PLL_ID_PLLMSB] = TEGRA194_MAX_CLK_ID,
		[TEGRABL_CLK_PLL_ID_PLLREFE] = TEGRA194_CLK_PLLREFE_VCOOUT,
};

#define MOD_CLK                     0
#define MOD_RST                     1

#define UART_MAX_INSTANCES_A2G      7
#define SDMMC_MAX_INSTANCES_1TO4    4
#define I2C_MAX_INSTANCES_1TO14     14
#define SPI_MAX_INSTANCES_1TO4      4
#define QSPI_MAX_INSTANCES_1TO2     2
#define PWM_MAX_INSTANCES           8

static uint32_t uart_module_instances[UART_MAX_INSTANCES_A2G][2] = {
	{TEGRA194_CLK_UARTA, TEGRA194_RESET_UARTA},
	{TEGRA194_CLK_UARTB, TEGRA194_RESET_UARTB},
	{TEGRA194_CLK_UARTC, TEGRA194_RESET_UARTC},
	{TEGRA194_CLK_UARTD, TEGRA194_RESET_UARTD},
	{TEGRA194_CLK_UARTE, TEGRA194_RESET_UARTE},
	{TEGRA194_CLK_UARTF, TEGRA194_RESET_UARTF},
	{TEGRA194_CLK_UARTG, TEGRA194_RESET_UARTG}
};

static uint32_t sdmmc_module_instances[SDMMC_MAX_INSTANCES_1TO4][2] = {
	{TEGRA194_CLK_SDMMC1, TEGRA194_RESET_SDMMC1},
	{MODULE_NOT_SUPPORTED, MODULE_NOT_SUPPORTED},
	{TEGRA194_CLK_SDMMC3, TEGRA194_RESET_SDMMC3},
	{TEGRA194_CLK_SDMMC4, TEGRA194_RESET_SDMMC4}
};

static uint32_t i2c_module_instances[I2C_MAX_INSTANCES_1TO14][2] = {
	{TEGRA194_CLK_I2C1,    TEGRA194_RESET_I2C1},
	{TEGRA194_CLK_I2C2,    TEGRA194_RESET_I2C2},
	{TEGRA194_CLK_I2C3,    TEGRA194_RESET_I2C3},
	{TEGRA194_CLK_I2C4,    TEGRA194_RESET_I2C4},
	{MODULE_NOT_SUPPORTED,    MODULE_NOT_SUPPORTED},
	{TEGRA194_CLK_I2C6,    TEGRA194_RESET_I2C6},
	{TEGRA194_CLK_I2C7,    TEGRA194_RESET_I2C7},
	{TEGRA194_CLK_I2C8,    TEGRA194_RESET_I2C8},
	{TEGRA194_CLK_I2C9,    TEGRA194_RESET_I2C9},
	{MODULE_NOT_SUPPORTED,   TEGRA194_RESET_I2C10},
	{MODULE_NOT_SUPPORTED, MODULE_NOT_SUPPORTED},
	{MODULE_NOT_SUPPORTED,   MODULE_NOT_SUPPORTED},
	{MODULE_NOT_SUPPORTED,   MODULE_NOT_SUPPORTED},
	{MODULE_NOT_SUPPORTED,   MODULE_NOT_SUPPORTED}
};

static uint32_t qspi_module_instances[QSPI_MAX_INSTANCES_1TO2][2] = {
	{TEGRA194_CLK_QSPI0, TEGRA194_RESET_QSPI0},
	{TEGRA194_CLK_QSPI1, TEGRA194_RESET_QSPI1},
};

static uint32_t mphy_instances[TEGRABL_CLK_MPHY_MAX_INSTANCES][2] = {
	[TEGRABL_CLK_MPHY_IOBIST_RST]		= {MODULE_NOT_SUPPORTED, MODULE_NOT_SUPPORTED_SKIPPED},
	[TEGRABL_CLK_MPHY_CLK_CTL_RST]		= {MODULE_NOT_SUPPORTED, TEGRA194_RESET_MPHY_CLK_CTL},
	[TEGRABL_CLK_MPHY_L1_RX_RST]		= {MODULE_NOT_SUPPORTED, TEGRA194_RESET_MPHY_L1_RX},
	[TEGRABL_CLK_MPHY_L1_TX_RST]		= {MODULE_NOT_SUPPORTED, TEGRA194_RESET_MPHY_L1_TX},
	[TEGRABL_CLK_MPHY_L0_RX_RST]		= {MODULE_NOT_SUPPORTED, TEGRA194_RESET_MPHY_L0_RX},
	[TEGRABL_CLK_MPHY_L0_TX_RST]		= {MODULE_NOT_SUPPORTED, TEGRA194_RESET_MPHY_L0_TX},
	[TEGRABL_CLK_MPHY_CORE_PLL_FIXED]	= {TEGRA194_CLK_MPHY_CORE_PLL_FIXED, MODULE_NOT_SUPPORTED},
	[TEGRABL_CLK_MPHY_TX_1MHZ_REF]		= {TEGRA194_CLK_MPHY_TX_1MHZ_REF, MODULE_NOT_SUPPORTED},
	[TEGRABL_CLK_MPHY_IOBIST]			= {MODULE_NOT_SUPPORTED_SKIPPED, MODULE_NOT_SUPPORTED},
	[TEGRABL_CLK_MPHY_L1_RX_ANA]		= {TEGRA194_CLK_MPHY_L1_RX_ANA, MODULE_NOT_SUPPORTED},
	[TEGRABL_CLK_MPHY_L0_RX_ANA]		= {TEGRA194_CLK_MPHY_L0_RX_ANA, MODULE_NOT_SUPPORTED},
	[TEGRABL_CLK_MPHY_L0_TX_LS_3XBIT]	= {TEGRA194_CLK_MPHY_L0_TX_LS_3XBIT, MODULE_NOT_SUPPORTED},
	[TEGRABL_CLK_MPHY_L0_TX_SYMB]		= {TEGRA194_CLK_MPHY_L0_TX_SYMB, MODULE_NOT_SUPPORTED},
	[TEGRABL_CLK_MPHY_L0_RX_LS_BIT]		= {TEGRA194_CLK_MPHY_L0_RX_LS_BIT, MODULE_NOT_SUPPORTED},
	[TEGRABL_CLK_MPHY_L0_RX_SYMB]		= {TEGRA194_CLK_MPHY_L0_RX_SYMB, MODULE_NOT_SUPPORTED},
	[TEGRABL_CLK_MPHY_FORCE_LS_MODE]	= {TEGRA194_CLK_MPHY_FORCE_LS_MODE, MODULE_NOT_SUPPORTED},
};

static uint32_t ufs_instances[TEGRABL_CLK_UFS_MAX_INSTANCES][2] = {
	[TEGRABL_CLK_UFSHC_RST]			= {MODULE_NOT_SUPPORTED, TEGRA194_RESET_UFSHC},
	[TEGRABL_CLK_UFSHC_AXI_M_RST]	= {MODULE_NOT_SUPPORTED, TEGRA194_RESET_UFSHC_AXI_M},
	[TEGRABL_CLK_UFSHC_LP_RST]		= {MODULE_NOT_SUPPORTED, TEGRA194_RESET_UFSHC_LP_SEQ},
	[TEGRABL_CLK_UFSDEV_REF]		= {TEGRA194_CLK_UFSDEV_REF, MODULE_NOT_SUPPORTED},
	[TEGRABL_CLK_UFSHC]				= {TEGRA194_CLK_UFSHC, MODULE_NOT_SUPPORTED},
};

static uint32_t eqos_instances[TEGRABL_CLK_EQOS_MAX_INSTANCES][2] = {
	[TEGRABL_CLK_EQOS_AXI]		= {TEGRA194_CLK_EQOS_AXI, MODULE_NOT_SUPPORTED},
	[TEGRABL_CLK_EQOS_PTP_REF]	= {TEGRA194_CLK_EQOS_PTP_REF, MODULE_NOT_SUPPORTED},
	[TEGRABL_CLK_EQOS_RX]		= {TEGRA194_CLK_EQOS_RX, MODULE_NOT_SUPPORTED},
	[TEGRABL_CLK_EQOS_TX]		= {TEGRA194_CLK_EQOS_TX, MODULE_NOT_SUPPORTED},
	[TEGRABL_CLK_EQOS_RST]		= {MODULE_NOT_SUPPORTED, TEGRA194_RESET_EQOS},
};

static uint32_t pex_usb_instances[TEGRABL_MODULE_PEX_USB_UPHY][2] = {
	[TEGRABL_CLK_PEX_USB_UPHY_L11_RST]	= {MODULE_NOT_SUPPORTED, TEGRA194_RESET_PEX_USB_UPHY_L11},
	[TEGRABL_CLK_PEX_USB_UPHY_L10_RST]	= {MODULE_NOT_SUPPORTED, TEGRA194_RESET_PEX_USB_UPHY_L10},
	[TEGRABL_CLK_PEX_USB_UPHY_L9_RST]	= {MODULE_NOT_SUPPORTED, TEGRA194_RESET_PEX_USB_UPHY_L9},
	[TEGRABL_CLK_PEX_USB_UPHY_L8_RST]	= {MODULE_NOT_SUPPORTED, TEGRA194_RESET_PEX_USB_UPHY_L8},
	[TEGRABL_CLK_PEX_USB_UPHY_L7_RST]	= {MODULE_NOT_SUPPORTED, TEGRA194_RESET_PEX_USB_UPHY_L7},
	[TEGRABL_CLK_PEX_USB_UPHY_L6_RST]	= {MODULE_NOT_SUPPORTED, TEGRA194_RESET_PEX_USB_UPHY_L6},
	[TEGRABL_CLK_PEX_USB_UPHY_L5_RST]	= {MODULE_NOT_SUPPORTED, TEGRA194_RESET_PEX_USB_UPHY_L5},
	[TEGRABL_CLK_PEX_USB_UPHY_L4_RST]	= {MODULE_NOT_SUPPORTED, TEGRA194_RESET_PEX_USB_UPHY_L4},
	[TEGRABL_CLK_PEX_USB_UPHY_L3_RST]	= {MODULE_NOT_SUPPORTED, TEGRA194_RESET_PEX_USB_UPHY_L3},
	[TEGRABL_CLK_PEX_USB_UPHY_L2_RST]	= {MODULE_NOT_SUPPORTED, TEGRA194_RESET_PEX_USB_UPHY_L2},
	[TEGRABL_CLK_PEX_USB_UPHY_L1_RST]	= {MODULE_NOT_SUPPORTED, TEGRA194_RESET_PEX_USB_UPHY_L1},
	[TEGRABL_CLK_PEX_USB_UPHY_L0_RST]	= {MODULE_NOT_SUPPORTED, TEGRA194_RESET_PEX_USB_UPHY_L0},
	[TEGRABL_CLK_PEX_USB_UPHY_PLL1_RST]	= {MODULE_NOT_SUPPORTED, TEGRA194_RESET_PEX_USB_UPHY_PLL1},
	[TEGRABL_CLK_PEX_USB_UPHY_PLL0_RST]	= {MODULE_NOT_SUPPORTED, TEGRA194_RESET_PEX_USB_UPHY_PLL0},
	[TEGRABL_CLK_PEX_USB_UPHY_PLL2_RST]	= {MODULE_NOT_SUPPORTED, TEGRA194_RESET_PEX_USB_UPHY_PLL2},
	[TEGRABL_CLK_PEX_USB_UPHY_PLL3_RST]	= {MODULE_NOT_SUPPORTED, TEGRA194_RESET_PEX_USB_UPHY_PLL3},
	[TEGRABL_CLK_PEX_USB_UPHY_RST]		= {MODULE_NOT_SUPPORTED, TEGRA194_RESET_PEX_USB_UPHY},
};

#if defined(CONFIG_ENABLE_QSPI)
static uint32_t qspi_module_div2_instances[QSPI_MAX_INSTANCES_1TO2][2] = {
	{TEGRA194_CLK_QSPI0_PM, MODULE_NOT_SUPPORTED},
	{TEGRA194_CLK_QSPI1_PM, MODULE_NOT_SUPPORTED},
};
#endif

#define TEGRABL_XUSB 0
#define TEGRABL_XUSB_DEV 1
#define TEGRABL_XUSB_HOST 2
#define TEGRABL_XUSB_SS 3
#define TEGRABL_XUSB_PADCTL 4
#define XUSB_MAX_INSTANCES 5
static uint32_t internal_xusb_index_map;

static uint32_t xusb_module_instances[XUSB_MAX_INSTANCES][2] = {
	{MODULE_NOT_SUPPORTED,  MODULE_NOT_SUPPORTED},
	{TEGRA194_CLK_XUSB_CORE_DEV,  MODULE_NOT_SUPPORTED},
	{TEGRA194_CLK_XUSB_CORE_HOST, MODULE_NOT_SUPPORTED},
	{TEGRA194_CLK_XUSB_CORE_SS,   MODULE_NOT_SUPPORTED},
	{MODULE_NOT_SUPPORTED,        TEGRA194_RESET_XUSB_PADCTL}
};

static enum {
	TEGRABL_RST_NVDISPLAY0_HEAD0,
	TEGRABL_RST_NVDISPLAY0_HEAD1,
	TEGRABL_RST_NVDISPLAY0_HEAD2,
	TEGRABL_RST_NVDISPLAY0_WGRP0,
	TEGRABL_RST_NVDISPLAY0_WGRP1,
	TEGRABL_RST_NVDISPLAY0_WGRP2,
	TEGRABL_RST_NVDISPLAY0_WGRP3,
	TEGRABL_RST_NVDISPLAY0_WGRP4,
	TEGRABL_RST_NVDISPLAY0_WGRP5,
	TEGRABL_RST_NVDISPLAY0_MISC,
	TEGRABL_NVDISP_SOR0,
	TEGRABL_NVDISP_SOR1,
	TEGRABL_NVDISP_SOR2,
	TEGRABL_NVDISP_SOR3,
	TEGRABL_NVDISP_P0,
	TEGRABL_NVDISP_P1,
	TEGRABL_NVDISP_P2,
	TEGRABL_NVDISP_P3,
	TEGRABL_NVDISP_HOST1X,
	TEGRABL_NVDISP_HUB,
	TEGRABL_NVDISP_DSC,
	TEGRABL_NVDISP_DISP,
	TEGRABL_NVDISP_SOR0_PAD_CLKOUT,
	TEGRABL_NVDISP_SOR1_PAD_CLKOUT,
	TEGRABL_NVDISP_SOR2_PAD_CLKOUT,
	TEGRABL_NVDISP_SOR3_PAD_CLKOUT,
	TEGRABL_NVDISP_SOR_SAFE,
	TEGRABL_NVDISP_SOR0_OUT,
	TEGRABL_NVDISP_SOR1_OUT,
	TEGRABL_NVDISP_SOR2_OUT,
	TEGRABL_NVDISP_SOR3_OUT,
	TEGRABL_NVDISP_DPAUX,
	TEGRABL_NVDISP_DPAUX1,
	TEGRABL_NVDISP_DPAUX2,
	TEGRABL_NVDISP_DPAUX3,
	NVDISP_MAX_INSTANCES
} index_nvdisp_map;

static uint32_t nvdisp_module_instance[NVDISP_MAX_INSTANCES][2] = {
	{MODULE_NOT_SUPPORTED,         TEGRA194_RESET_NVDISPLAY0_HEAD0},
	{MODULE_NOT_SUPPORTED,         TEGRA194_RESET_NVDISPLAY0_HEAD1},
	{MODULE_NOT_SUPPORTED,         TEGRA194_RESET_NVDISPLAY0_HEAD2},
	{MODULE_NOT_SUPPORTED,         TEGRA194_RESET_NVDISPLAY0_WGRP0},
	{MODULE_NOT_SUPPORTED,         TEGRA194_RESET_NVDISPLAY0_WGRP1},
	{MODULE_NOT_SUPPORTED,         TEGRA194_RESET_NVDISPLAY0_WGRP2},
	{MODULE_NOT_SUPPORTED,         TEGRA194_RESET_NVDISPLAY0_WGRP3},
	{MODULE_NOT_SUPPORTED,         TEGRA194_RESET_NVDISPLAY0_WGRP4},
	{MODULE_NOT_SUPPORTED,         TEGRA194_RESET_NVDISPLAY0_WGRP5},
	{MODULE_NOT_SUPPORTED,         TEGRA194_RESET_NVDISPLAY0_MISC},
	{TEGRA194_CLK_SOR0_REF,        TEGRA194_RESET_SOR0},
	{TEGRA194_CLK_SOR1_REF,        TEGRA194_RESET_SOR1},
	{TEGRA194_CLK_SOR2_REF,        TEGRA194_RESET_SOR2},
	{TEGRA194_CLK_SOR3_REF,        TEGRA194_RESET_SOR3},
	{TEGRA194_CLK_NVDISPLAY_P0,    MODULE_NOT_SUPPORTED},
	{TEGRA194_CLK_NVDISPLAY_P1,    MODULE_NOT_SUPPORTED},
	{TEGRA194_CLK_NVDISPLAY_P2,    MODULE_NOT_SUPPORTED},
	{TEGRA194_CLK_NVDISPLAY_P3,    MODULE_NOT_SUPPORTED},
	{TEGRA194_CLK_HOST1X,          TEGRA194_RESET_HOST1X},
	{TEGRA194_CLK_NVDISPLAYHUB,    MODULE_NOT_SUPPORTED},
	{MODULE_NOT_SUPPORTED,   MODULE_NOT_SUPPORTED},
	{TEGRA194_CLK_NVDISPLAY_DISP,  MODULE_NOT_SUPPORTED},
	{TEGRA194_CLK_SOR0_PAD_CLKOUT, MODULE_NOT_SUPPORTED},
	{TEGRA194_CLK_SOR1_PAD_CLKOUT, MODULE_NOT_SUPPORTED},
	{TEGRA194_CLK_SOR2_PAD_CLKOUT, MODULE_NOT_SUPPORTED},
	{TEGRA194_CLK_SOR3_PAD_CLKOUT, MODULE_NOT_SUPPORTED},
	{TEGRA194_CLK_SOR_SAFE,        MODULE_NOT_SUPPORTED},
	{TEGRA194_CLK_SOR0_OUT,        MODULE_NOT_SUPPORTED},
	{TEGRA194_CLK_SOR1_OUT,        MODULE_NOT_SUPPORTED},
	{TEGRA194_CLK_SOR2_OUT,        MODULE_NOT_SUPPORTED},
	{TEGRA194_CLK_SOR3_OUT,        MODULE_NOT_SUPPORTED},
	{TEGRA194_CLK_DPAUX,           TEGRA194_RESET_DPAUX},
	{TEGRA194_CLK_DPAUX1,          TEGRA194_RESET_DPAUX1},
	{TEGRA194_CLK_DPAUX2,          TEGRA194_RESET_DPAUX2},
	{TEGRA194_CLK_DPAUX3,          TEGRA194_RESET_DPAUX3},
	};

static uint32_t spi_module_instances[SPI_MAX_INSTANCES_1TO4][2] = {
	{TEGRA194_CLK_SPI1, TEGRA194_RESET_SPI1},
	{TEGRA194_CLK_SPI2, TEGRA194_RESET_SPI2},
	{TEGRA194_CLK_SPI3, TEGRA194_RESET_SPI3},
	{MODULE_NOT_SUPPORTED, TEGRA194_RESET_SPI4},
};

static uint32_t pwm_module_instances[PWM_MAX_INSTANCES][2] = {
	{TEGRA194_CLK_PWM1, TEGRA194_RESET_PWM1},
	{TEGRA194_CLK_PWM2, TEGRA194_RESET_PWM2},
	{TEGRA194_CLK_PWM3, TEGRA194_RESET_PWM3},
	{TEGRA194_CLK_PWM4, TEGRA194_RESET_PWM4},
	{TEGRA194_CLK_PWM5, TEGRA194_RESET_PWM5},
	{TEGRA194_CLK_PWM6, TEGRA194_RESET_PWM6},
	{TEGRA194_CLK_PWM7, TEGRA194_RESET_PWM7},
	{TEGRA194_CLK_PWM8, TEGRA194_RESET_PWM8},
};

static int32_t tegrabl_module_to_bpmp_id(
				tegrabl_module_t module_num,
				uint8_t instance,
				bool clk_or_rst)
{
	/* TODO - Complete below mapping */
	switch (module_num) {
	case (TEGRABL_MODULE_UART):
	{
		if (instance < UART_MAX_INSTANCES_A2G) {
			return uart_module_instances[instance][clk_or_rst];
		}
		break;
	}
	case (TEGRABL_MODULE_SDMMC):
	{
		if (instance < SDMMC_MAX_INSTANCES_1TO4) {
			return sdmmc_module_instances[instance][clk_or_rst];
		}
		break;
	}
	case (TEGRABL_MODULE_GPCDMA):
	{
		if (clk_or_rst == MOD_RST)
			return TEGRA194_RESET_GPCDMA;
		else if (clk_or_rst == MOD_CLK)
			return TEGRA194_CLK_GPCCLK;
		else
			return MODULE_NOT_SUPPORTED;
		break;
	}
	case (TEGRABL_MODULE_QSPI):
	{
		if (instance < QSPI_MAX_INSTANCES_1TO2) {
			return qspi_module_instances[instance][clk_or_rst];
		}
		break;
	}
	case (TEGRABL_MODULE_I2C):
	{
		if (instance < I2C_MAX_INSTANCES_1TO14) {
			return i2c_module_instances[instance][clk_or_rst];
		}
		break;
	}
	case (TEGRABL_MODULE_XUSBF):
	{
		internal_xusb_index_map = TEGRABL_XUSB;
		return xusb_module_instances[internal_xusb_index_map][clk_or_rst];
		break;
	}
	case (TEGRABL_MODULE_XUSB_DEV):
	{
		internal_xusb_index_map = TEGRABL_XUSB_DEV;
		return xusb_module_instances[internal_xusb_index_map][clk_or_rst];
		break;
	}
	case (TEGRABL_MODULE_XUSB_HOST):
	{
		internal_xusb_index_map = TEGRABL_XUSB_HOST;
		return xusb_module_instances[internal_xusb_index_map][clk_or_rst];
		break;
	}
	case (TEGRABL_MODULE_XUSB_SS):
	{
		internal_xusb_index_map = TEGRABL_XUSB_SS;
		return xusb_module_instances[internal_xusb_index_map][clk_or_rst];
		break;
	}
	case (TEGRABL_MODULE_XUSB_PADCTL):
	{
		internal_xusb_index_map = TEGRABL_XUSB_PADCTL;
		return xusb_module_instances[internal_xusb_index_map][clk_or_rst];
		break;
	}
	case (TEGRABL_MODULE_SOR):
	{
		if (instance > 3) /* SOR0, SOR1, SOR2, SOR3 */ {
			return MODULE_NOT_SUPPORTED;
		}
		/* index w.r.t SOR0 */
		index_nvdisp_map = TEGRABL_NVDISP_SOR0 + instance;
		return nvdisp_module_instance[index_nvdisp_map][clk_or_rst];
		break;
	}
	case (TEGRABL_MODULE_SOR_OUT):
	{
		if (instance > 3) /* SOR0_OUT, SOR1_OUT, SOR2_OUT, SOR3_OUT */ {
			return MODULE_NOT_SUPPORTED;
		}
		/* index w.r.t SOR0_OUT */
		index_nvdisp_map = TEGRABL_NVDISP_SOR0_OUT + instance;
		return nvdisp_module_instance[index_nvdisp_map][clk_or_rst];
		break;
	}
	case (TEGRABL_MODULE_SOR_PAD_CLKOUT):
	{
		if (instance > 3) /* SOR0_PAD_CLKOUT, SOR1_PAD_CLKOUT, SOR2_PAD_CLKOUT, SOR3_PAD_CLKOUT */ {
			return MODULE_NOT_SUPPORTED;
		}
		/* index w.r.t SOR0_PAD_CLKOUT */
		index_nvdisp_map = TEGRABL_NVDISP_SOR0_PAD_CLKOUT + instance;
		return nvdisp_module_instance[index_nvdisp_map][clk_or_rst];
		break;
	}
	case (TEGRABL_MODULE_SOR_SAFE):
	{
		index_nvdisp_map = TEGRABL_NVDISP_SOR_SAFE;
		return nvdisp_module_instance[index_nvdisp_map][clk_or_rst];
		break;
	}
	case (TEGRABL_MODULE_DPAUX):
	{
		index_nvdisp_map = TEGRABL_NVDISP_DPAUX;
		return nvdisp_module_instance[index_nvdisp_map][clk_or_rst];
		break;
	}
	case (TEGRABL_MODULE_DPAUX1):
	{
		index_nvdisp_map = TEGRABL_NVDISP_DPAUX1;
		return nvdisp_module_instance[index_nvdisp_map][clk_or_rst];
		break;
	}
	case (TEGRABL_MODULE_DPAUX2):
	{
		index_nvdisp_map = TEGRABL_NVDISP_DPAUX2;
		return nvdisp_module_instance[index_nvdisp_map][clk_or_rst];
		break;
	}
	case (TEGRABL_MODULE_DPAUX3):
	{
		index_nvdisp_map = TEGRABL_NVDISP_DPAUX3;
		return nvdisp_module_instance[index_nvdisp_map][clk_or_rst];
		break;
	}
	case (TEGRABL_MODULE_NVDISPLAYHUB):
	{
		index_nvdisp_map = TEGRABL_NVDISP_HUB;
		return nvdisp_module_instance[index_nvdisp_map][clk_or_rst];
		break;
	}
	case (TEGRABL_MODULE_NVDISPLAY_DSC):
	{
		index_nvdisp_map = TEGRABL_NVDISP_DSC;
		return nvdisp_module_instance[index_nvdisp_map][clk_or_rst];
		break;
	}
	case (TEGRABL_MODULE_NVDISPLAY_DISP):
	{
		index_nvdisp_map = TEGRABL_NVDISP_DISP;
		return nvdisp_module_instance[index_nvdisp_map][clk_or_rst];
		break;
	}
	case (TEGRABL_MODULE_NVDISPLAY_P):
	{
		if (instance > 3) /* P0 to P3 */ {
			return MODULE_NOT_SUPPORTED;
		}
		/* index w.r.t P0 */
		index_nvdisp_map = TEGRABL_NVDISP_P0 + instance;
		return nvdisp_module_instance[index_nvdisp_map][clk_or_rst];
		break;
	}
	case (TEGRABL_MODULE_HOST1X):
	{
		index_nvdisp_map = TEGRABL_NVDISP_HOST1X;
		return nvdisp_module_instance[index_nvdisp_map][clk_or_rst];
		break;
	}
	case (TEGRABL_MODULE_NVDISPLAY0_HEAD):
	{
		if (instance > 2) /* HEAD0 to HEAD2 */ {
			return MODULE_NOT_SUPPORTED;
		}
		/* index w.r.t HEAD0 */
		index_nvdisp_map =
					(TEGRABL_RST_NVDISPLAY0_HEAD0 + instance);
		return nvdisp_module_instance[index_nvdisp_map][clk_or_rst];
		break;
	}
	case (TEGRABL_MODULE_NVDISPLAY0_WGRP):
	{
		if (instance > 5) /* WGRP0 to WGRP5 */ {
			return MODULE_NOT_SUPPORTED;
		}
		/* index w.r.t WGRP0 */
		index_nvdisp_map =
					(TEGRABL_RST_NVDISPLAY0_WGRP0 + instance);
		return nvdisp_module_instance[index_nvdisp_map][clk_or_rst];
		break;
	}
	case (TEGRABL_MODULE_NVDISPLAY0_MISC):
	{
		index_nvdisp_map = TEGRABL_RST_NVDISPLAY0_MISC;
		return nvdisp_module_instance[index_nvdisp_map][clk_or_rst];
		break;
	}
	case TEGRABL_MODULE_SE:
	{
		switch (clk_or_rst) {
		case MOD_RST:
			return TEGRA194_RESET_SE;
			break;
		case MOD_CLK:
			return TEGRA194_CLK_SE;
			break;
		}
		break;
	}
	case TEGRABL_MODULE_SPI:
	{
		if (instance < SPI_MAX_INSTANCES_1TO4) {
			return spi_module_instances[instance][clk_or_rst];
		}
		break;
	}
	case TEGRABL_MODULE_AUD_MCLK:
	{
		switch (clk_or_rst) {
		case MOD_RST:
			return MODULE_NOT_SUPPORTED;
			break;
		case MOD_CLK:
			return TEGRA194_CLK_AUD_MCLK;
			break;
		}
	}
	case TEGRABL_MODULE_SATA:
	{
		switch (clk_or_rst) {
		case MOD_RST:
			return TEGRA194_RESET_SATA;
			break;
		case MOD_CLK:
			return TEGRA194_CLK_SATA;
			break;
		}
	}
	case TEGRABL_MODULE_SATACOLD:
	{
		switch (clk_or_rst) {
		case MOD_RST:
			return TEGRA194_RESET_SATACOLD;
			break;
		}
	}
	case TEGRABL_MODULE_SATA_OOB:
	{
		switch (clk_or_rst) {
		case MOD_CLK:
			return TEGRA194_CLK_SATA_OOB;
			break;
		}
	}
	case TEGRABL_MODULE_PCIE:
	{
		switch (clk_or_rst) {
		case MOD_RST:
			return TEGRA194_RESET_PCIE;
			break;
		}
	}
	case TEGRABL_MODULE_PCIEXCLK:
	{
		switch (clk_or_rst) {
		case MOD_RST:
			return TEGRA194_RESET_PCIEXCLK;
			break;
		}
	}
	case TEGRABL_MODULE_AFI:
	{
		switch (clk_or_rst) {
		case MOD_RST:
			return TEGRA194_RESET_AFI;
			break;
		}
	}
	case TEGRABL_MODULE_MPHY:
	{
		if (instance < TEGRABL_CLK_MPHY_MAX_INSTANCES) {
			return mphy_instances[instance][clk_or_rst];
		}
		break;
	}
	case TEGRABL_MODULE_UFS:
	{
		if (instance < TEGRABL_CLK_UFS_MAX_INSTANCES) {
			return ufs_instances[instance][clk_or_rst];
		}
		break;
	}
	case TEGRABL_MODULE_UFSDEV_REF:
	{
		if (clk_or_rst == MOD_CLK) {
			return TEGRA194_CLK_UFSDEV_REF;
		}
		break;
	}
	case TEGRABL_MODULE_UFSHC_CG_SYS:
	{
		if (clk_or_rst == MOD_CLK) {
			return TEGRA194_CLK_UFSHC;
		}
		break;
	}
	case TEGRABL_MODULE_EQOS:
	{
		if (instance < TEGRABL_CLK_EQOS_MAX_INSTANCES) {
			return eqos_instances[instance][clk_or_rst];
		}
		break;
	}
	case TEGRABL_MODULE_AXI_CBB:
	{
		if (clk_or_rst == MOD_CLK) {
			return TEGRA194_CLK_AXI_CBB;
		}
		break;
	}
	case TEGRABL_MODULE_PEX_USB_UPHY:
	{
		if (instance < TEGRABL_CLK_PEX_USB_UPHY_MAX_INSTANCES) {
			return pex_usb_instances[instance][clk_or_rst];
		}
		break;
	}
	case TEGRABL_MODULE_PWM:
	{
		if (instance < PWM_MAX_INSTANCES) {
			return pwm_module_instances[instance][clk_or_rst];
		}
		break;
	}
	default:
		break;
	}
	return MODULE_NOT_SUPPORTED;
}

static tegrabl_clk_src_id_t src_clk_bpmp_to_tegrabl(uint32_t src)
{
	/* TODO - Complete below mapping */
	switch (src) {
	case TEGRA194_CLK_PLLP_OUT0:
		return TEGRABL_CLK_SRC_PLLP_OUT0;
		break;
	case TEGRA194_CLK_CLK_M:
		return TEGRABL_CLK_SRC_CLK_M;
		break;
	case TEGRA194_CLK_PLLC4_OUT1:
	case TEGRA194_CLK_PLLC4_OUT2:
	case TEGRA194_CLK_PLLC4_VCO_DIV2:
		return TEGRABL_CLK_SRC_PLLC4_MUXED;
		break;
	case TEGRA194_CLK_CLK_32K:
		return TEGRABL_CLK_SRC_CLK_S;
		break;
	case TEGRA194_CLK_PLLD:
		return TEGRABL_CLK_SRC_PLLD_OUT1;
		break;
	case TEGRA194_CLK_PLLD2:
		return TEGRABL_CLK_SRC_PLLD2_OUT0;
		break;
	case TEGRA194_CLK_PLLD3:
		return TEGRABL_CLK_SRC_PLLD3_OUT0;
		break;
	case TEGRA194_CLK_PLLDP:
		return TEGRABL_CLK_SRC_PLLDP;
		break;
	case TEGRA194_CLK_NVDISPLAY_P0:
		return TEGRABL_CLK_SRC_NVDISPLAY_P0_CLK;
		break;
	case TEGRA194_CLK_NVDISPLAY_P1:
		return TEGRABL_CLK_SRC_NVDISPLAY_P1_CLK;
		break;
	case TEGRA194_CLK_NVDISPLAY_P2:
		return TEGRABL_CLK_SRC_NVDISPLAY_P2_CLK;
		break;
	case TEGRA194_CLK_SOR0_REF:
		return TEGRABL_CLK_SRC_SOR0;
		break;
	case TEGRA194_CLK_SOR1_REF:
		return TEGRABL_CLK_SRC_SOR1;
		break;
	case TEGRA194_CLK_SOR2_REF:
		return TEGRABL_CLK_SRC_SOR2;
		break;
	case TEGRA194_CLK_SOR3_REF:
		return TEGRABL_CLK_SRC_SOR3;
		break;
	case TEGRA194_CLK_SOR_SAFE:
		return TEGRABL_CLK_SRC_SOR_SAFE_CLK;
		break;
	case TEGRA194_CLK_SOR0_PAD_CLKOUT:
		return TEGRABL_CLK_SRC_SOR0_PAD_CLKOUT;
		break;
	case TEGRA194_CLK_SOR1_PAD_CLKOUT:
		return TEGRABL_CLK_SRC_SOR1_PAD_CLKOUT;
		break;
	case TEGRA194_CLK_SOR2_PAD_CLKOUT:
		return TEGRABL_CLK_SRC_SOR2_PAD_CLKOUT;
		break;
	case TEGRA194_CLK_SOR3_PAD_CLKOUT:
		return TEGRABL_CLK_SRC_SOR3_PAD_CLKOUT;
		break;
	case TEGRA194_CLK_PLLDISPHUB_DIV:
		return TEGRABL_CLK_SRC_PLLDISPHUB_DIV;
		break;
	case TEGRA194_CLK_PLLDISPHUB:
		return TEGRABL_CLK_SRC_PLLDISPHUB;
		break;
	default:
		return TEGRABL_CLK_SRC_INVALID;
	}
}

static uint32_t src_clk_tegrabl_to_bpmp(tegrabl_clk_src_id_t src)
{
	/* TODO - Complete below mapping */
	switch (src) {
	case TEGRABL_CLK_SRC_PLLP_OUT0:
		return TEGRA194_CLK_PLLP_OUT0;
		break;
	case TEGRABL_CLK_SRC_CLK_M:
		return TEGRA194_CLK_CLK_M;
		break;
	case TEGRABL_CLK_SRC_CLK_S:
		return TEGRA194_CLK_CLK_32K;
		break;
	case TEGRABL_CLK_SRC_PLLD_OUT1:
		return TEGRA194_CLK_PLLD;
		break;
	case TEGRABL_CLK_SRC_PLLD2_OUT0:
		return TEGRA194_CLK_PLLD2;
		break;
	case TEGRABL_CLK_SRC_PLLD3_OUT0:
		return TEGRA194_CLK_PLLD3;
		break;
	case TEGRABL_CLK_SRC_PLLDP:
		return TEGRA194_CLK_PLLDP;
		break;
	case TEGRABL_CLK_SRC_NVDISPLAY_P0_CLK:
		return TEGRA194_CLK_NVDISPLAY_P0;
		break;
	case TEGRABL_CLK_SRC_NVDISPLAY_P1_CLK:
		return TEGRA194_CLK_NVDISPLAY_P1;
		break;
	case TEGRABL_CLK_SRC_NVDISPLAY_P2_CLK:
		return TEGRA194_CLK_NVDISPLAY_P2;
		break;
	case TEGRABL_CLK_SRC_SOR0:
		return TEGRA194_CLK_SOR0_REF;
		break;
	case TEGRABL_CLK_SRC_SOR1:
		return TEGRA194_CLK_SOR1_REF;
		break;
	case TEGRABL_CLK_SRC_SOR2:
		return TEGRA194_CLK_SOR2_REF;
		break;
	case TEGRABL_CLK_SRC_SOR3:
		return TEGRA194_CLK_SOR3_REF;
		break;
	case TEGRABL_CLK_SRC_SOR_SAFE_CLK:
		return TEGRA194_CLK_SOR_SAFE;
		break;
	case TEGRABL_CLK_SRC_SOR0_PAD_CLKOUT:
		return TEGRA194_CLK_SOR0_PAD_CLKOUT;
		break;
	case TEGRABL_CLK_SRC_SOR1_PAD_CLKOUT:
		return TEGRA194_CLK_SOR1_PAD_CLKOUT;
		break;
	case TEGRABL_CLK_SRC_SOR2_PAD_CLKOUT:
		return TEGRA194_CLK_SOR2_PAD_CLKOUT;
		break;
	case TEGRABL_CLK_SRC_SOR3_PAD_CLKOUT:
		return TEGRA194_CLK_SOR3_PAD_CLKOUT;
		break;
	case TEGRABL_CLK_SRC_PLLDISPHUB_DIV:
		return TEGRA194_CLK_PLLDISPHUB_DIV;
		break;
	case TEGRABL_CLK_SRC_PLLDISPHUB:
		return TEGRA194_CLK_PLLDISPHUB;
		break;
	case TEGRABL_CLK_SRC_PLLC4_OUT0_LJ:
	case TEGRABL_CLK_SRC_PLLC4_OUT1_LJ:
	case TEGRABL_CLK_SRC_PLLC4_OUT2_LJ:
		return TEGRA194_CLK_PLLC4;
		break;
	default:
		return TEGRA194_MAX_CLK_ID;
	}
}

static tegrabl_error_t internal_tegrabl_car_set_clk_src(
		uint32_t clk_id,
		uint32_t clk_src)
{
	struct mrq_clk_request req_clk_set_src;
	struct mrq_clk_response resp_clk_set_src;

	if ((clk_id == MODULE_NOT_SUPPORTED) ||
		(clk_src == TEGRA194_MAX_CLK_ID)) {
			pr_error("%s coudn't set %d (bpmpid) as parent for %d, returning\n",
					 __func__, clk_src, clk_id);
		return TEGRABL_ERR_NOT_SUPPORTED;
	}

	req_clk_set_src.clk_set_parent.parent_id = clk_src;
	req_clk_set_src.cmd_and_id = BPMP_CLK_CMD(CMD_CLK_SET_PARENT, clk_id);

	pr_trace("(%s,%d) bpmp_src: %d\n", __func__, __LINE__, clk_src);

	/* TX */
	if (TEGRABL_NO_ERROR != tegrabl_ccplex_bpmp_xfer(
					&req_clk_set_src, &resp_clk_set_src,
					sizeof(struct mrq_clk_request),
					sizeof(struct mrq_clk_response),
					MRQ_CLK)) {
		pr_error("Error in tx-rx: %s,%d\n", __func__, __LINE__);
		return TEGRABL_ERR_INVALID;
	}

	return TEGRABL_NO_ERROR;
}

static tegrabl_error_t internal_tegrabl_car_get_clk_rate(
		uint32_t clk_id,
		uint32_t *rate_khz)
{
	struct mrq_clk_request req_clk_get_rate;
	struct mrq_clk_response resp_clk_get_rate;

	if (clk_id == TEGRA194_MAX_CLK_ID) {
		return TEGRABL_ERR_NOT_SUPPORTED;
	}

	req_clk_get_rate.cmd_and_id = BPMP_CLK_CMD(CMD_CLK_GET_RATE, clk_id);

	/* TX */
	if (TEGRABL_NO_ERROR != tegrabl_ccplex_bpmp_xfer(
					&req_clk_get_rate, &resp_clk_get_rate,
					sizeof(struct mrq_clk_request),
					sizeof(struct mrq_clk_response),
					MRQ_CLK)) {
		pr_error("Error in tx-rx: %s,%d\n", __func__, __LINE__);
		return TEGRABL_ERR_INVALID;
	}

	/* RX */
	*rate_khz = (resp_clk_get_rate.clk_get_rate.rate)/HZ_1K;
	pr_trace("Received data (from BPMP) %d\n", *rate_khz);

	return TEGRABL_NO_ERROR;
}

static tegrabl_error_t internal_tegrabl_car_set_clk_rate(
		uint32_t clk_id,
		uint32_t rate_khz,
		uint32_t *rate_set_khz)
{
	struct mrq_clk_request req_clk_set_rate;
	struct mrq_clk_response resp_clk_set_rate;

	if (clk_id == MODULE_NOT_SUPPORTED) {
		return TEGRABL_ERR_NOT_SUPPORTED;
	}

	req_clk_set_rate.cmd_and_id = BPMP_CLK_CMD(CMD_CLK_SET_RATE, clk_id);
	req_clk_set_rate.clk_set_rate.rate = rate_khz*HZ_1K;

	/* TX */
	if (TEGRABL_NO_ERROR != tegrabl_ccplex_bpmp_xfer(
					&req_clk_set_rate, &resp_clk_set_rate,
					sizeof(struct mrq_clk_request),
					sizeof(struct mrq_clk_response),
					MRQ_CLK)) {
		pr_error("Error in tx-rx: %s,%d\n", __func__, __LINE__);
		return TEGRABL_ERR_INVALID;
	}

	/* RX */
	*rate_set_khz = (resp_clk_set_rate.clk_set_rate.rate)/HZ_1K;

	pr_trace("(%s,%d) Enabled rate %d for %d\n", __func__, __LINE__,
			 *rate_set_khz, clk_id);

	return TEGRABL_NO_ERROR;
}

static tegrabl_error_t internal_tegrabl_car_clk_enable(uint32_t clk_id)
{
	struct mrq_clk_request req_clk_enable;
	struct mrq_clk_response resp_clk_enable;

	if (clk_id == MODULE_NOT_SUPPORTED) {
		return TEGRABL_ERR_NOT_SUPPORTED;
	} else if (clk_id == MODULE_NOT_SUPPORTED_SKIPPED) {
		pr_trace("(%s,%d) operation skipped\n", __func__, __LINE__);
		return TEGRABL_NO_ERROR;
	}

	req_clk_enable.cmd_and_id = BPMP_CLK_CMD(CMD_CLK_ENABLE, clk_id);

	/* TX */
	if (TEGRABL_NO_ERROR != tegrabl_ccplex_bpmp_xfer(
					&req_clk_enable, &resp_clk_enable,
					sizeof(struct mrq_clk_request),
					sizeof(struct mrq_clk_response),
					MRQ_CLK)) {
		pr_error("Error in tx-rx: %s,%d\n", __func__, __LINE__);
		return TEGRABL_ERR_INVALID;
	}

	pr_trace("(%s,%d) Enabled - %d\n", __func__, __LINE__, clk_id);

		return TEGRABL_NO_ERROR;
}

static bool internal_tegrabl_car_clk_is_enabled(uint32_t clk_id)
{
	struct mrq_clk_request req_clk_is_enabled;
	struct mrq_clk_response resp_clk_is_enabled;

	if (clk_id == MODULE_NOT_SUPPORTED) {
		return false;
	}

	req_clk_is_enabled.cmd_and_id = BPMP_CLK_CMD(CMD_CLK_IS_ENABLED, clk_id);

	/* TX */
	if (TEGRABL_NO_ERROR != tegrabl_ccplex_bpmp_xfer(
					&req_clk_is_enabled, &resp_clk_is_enabled,
					sizeof(struct mrq_clk_request),
					sizeof(struct mrq_clk_response),
					MRQ_CLK)) {
		pr_error("Error in tx-rx: %s,%d\n", __func__, __LINE__);
		return false;
	}

	pr_trace("(%s,%d) clk(%d) state = %d\n", __func__, __LINE__, clk_id,
			 resp_clk_is_enabled.clk_is_enabled.state);

	return (bool)resp_clk_is_enabled.clk_is_enabled.state;
}

bool tegrabl_car_clk_is_enabled(tegrabl_module_t module, uint8_t instance)
{
	uint32_t bpmp_id;

	bpmp_id = tegrabl_module_to_bpmp_id(module, instance, MOD_CLK);
	if (bpmp_id == MODULE_NOT_SUPPORTED) {
		return false;
	}

	return internal_tegrabl_car_clk_is_enabled(bpmp_id);
}


static tegrabl_error_t internal_tegrabl_car_clk_disable(uint32_t clk_id)
{
	struct mrq_clk_request req_clk_disable;
	struct mrq_clk_response resp_clk_disable;

	if (!internal_tegrabl_car_clk_is_enabled(clk_id)) {
		pr_trace("clock (id - %d) not enabled. skipping disable request\n",
				 clk_id);
		return TEGRABL_NO_ERROR;
	}

	if (clk_id == MODULE_NOT_SUPPORTED) {
		return TEGRABL_ERR_NOT_SUPPORTED;
	} else if (clk_id == MODULE_NOT_SUPPORTED_SKIPPED) {
		pr_trace("(%s,%d) operation skipped\n", __func__, __LINE__);
		return TEGRABL_NO_ERROR;
	}

	req_clk_disable.cmd_and_id = BPMP_CLK_CMD(CMD_CLK_DISABLE, clk_id);

	/* TX */
	if (TEGRABL_NO_ERROR != tegrabl_ccplex_bpmp_xfer(
					&req_clk_disable, &resp_clk_disable,
					sizeof(struct mrq_clk_request),
					sizeof(struct mrq_clk_response),
					MRQ_CLK)) {
		pr_error("Error in tx-rx: %s,%d\n", __func__, __LINE__);
		return TEGRABL_ERR_INVALID;
	}

	pr_trace("(%s,%d) Disabled - %d\n", __func__, __LINE__, clk_id);

		return TEGRABL_NO_ERROR;
}

static tegrabl_error_t internal_tegrabl_car_rst(uint32_t rst_id, uint32_t flag)
{
	struct mrq_reset_request req_rst;
	uint32_t resp_rst;

	if (rst_id == MODULE_NOT_SUPPORTED) {
		return TEGRABL_ERR_NOT_SUPPORTED;
	} else if (rst_id == MODULE_NOT_SUPPORTED_SKIPPED) {
		pr_trace("(%s,%d) operation skipped\n", __func__, __LINE__);
		return TEGRABL_NO_ERROR;
	}

	pr_trace("(%s,%d) reset operation on %d\n", __func__, __LINE__, rst_id);
	req_rst.cmd = flag;
	req_rst.reset_id = rst_id;

	/* TX */
	if (TEGRABL_NO_ERROR != tegrabl_ccplex_bpmp_xfer(
					&req_rst, &resp_rst,
					sizeof(req_rst),
					sizeof(resp_rst),
					MRQ_RESET)) {
		pr_error("Error in tx-rx: %s,%d\n", __func__, __LINE__);
	}

	return TEGRABL_NO_ERROR;
}

/**
 * ------------------------NOTES------------------------
 * Please read below before using these APIs.
 * For using APIs that query clock state, namely get_clk_rate()
 * and get_clk_src(), it is necessary that clk_enable has been
 * called for the module before regardless of whether the clock is
 * enabled by default on POR. This is how the driver keeps initializes
 * the module clock states.
 */

/**
 * @brief - Gets the current clock source of the module
 *
 * @module - Module ID of the module
 * @instance - Instance of the module
 * @return - Enum of clock source if module is found and has a valid clk source
 * configured. TEGRABL_CLK_SRC_INVAID otherwise.
 */
tegrabl_clk_src_id_t tegrabl_car_get_clk_src(
		tegrabl_module_t module,
		uint8_t instance)
{
	struct mrq_clk_request req_clk_get_src;
	struct mrq_clk_response resp_clk_get_src;
	int32_t clk_id;

	pr_trace("(%s,%d) %d, %d\n", __func__, __LINE__,
			 module, instance);

	clk_id =  tegrabl_module_to_bpmp_id(module, instance, MOD_CLK);
	if (clk_id == MODULE_NOT_SUPPORTED) {
		return TEGRABL_ERR_NOT_SUPPORTED;
	}

	req_clk_get_src.cmd_and_id = BPMP_CLK_CMD(CMD_CLK_GET_PARENT, clk_id);

	/* TX */
	if (TEGRABL_NO_ERROR != tegrabl_ccplex_bpmp_xfer(
					&req_clk_get_src, &resp_clk_get_src,
					sizeof(struct mrq_clk_request),
					sizeof(struct mrq_clk_response),
					MRQ_CLK)) {
		pr_error("Error in tx-rx: %s,%d\n", __func__, __LINE__);
		return TEGRABL_CLK_SRC_INVALID;
	}

	/* RX */
	pr_trace("Received parent_id (from BPMP): %d\n",
			 resp_clk_get_src.clk_get_parent.parent_id);

	return src_clk_bpmp_to_tegrabl(resp_clk_get_src.clk_get_parent.parent_id);
}

/**
 * @brief - Sets the clock source of the module to
 * the source specified.
 * NOTE: If the module clock is disabled when this function is called,
 * the new settings will take effect only after enabling the clock.
 *
 * @module - Module ID of the module
 * @instance - Instance of the module
 * @clk_src - Specified source
 * @return - TEGRABL_NO_ERROR if success, error-reason otherwise.
 */
tegrabl_error_t tegrabl_car_set_clk_src(
		tegrabl_module_t module,
		uint8_t instance,
		tegrabl_clk_src_id_t clk_src)
{
	pr_trace("(%s,%d) %d, %d\n", __func__, __LINE__,
			 module, instance);

	return internal_tegrabl_car_set_clk_src(
					tegrabl_module_to_bpmp_id(module, instance, MOD_CLK),
					src_clk_tegrabl_to_bpmp(clk_src));
}

/**
 * @brief - Gets the current clock rate of the module
 *
 * @module - Module ID of the module
 * @instance - Instance of the module
 * @rate_khz - Address to store the current clock rate
 * @return - TEGRABL_NO_ERROR if success, error-reason otherwise.
 */
tegrabl_error_t tegrabl_car_get_clk_rate(
		tegrabl_module_t module,
		uint8_t instance,
		uint32_t *rate_khz)
{
	pr_trace("(%s,%d) %d, %d\n", __func__, __LINE__,
			 module, instance);

	return internal_tegrabl_car_get_clk_rate(
			tegrabl_module_to_bpmp_id(module, instance, MOD_CLK),
			rate_khz);
}

/**
 * @brief - Get current frequency of the specified
 * clock source.
 *
 * @src_id - enum of the clock source
 * @rate_khz - Address to store the frequency of the clock source
 * @return - TEGRABL_NO_ERROR if success, error-reason otherwise.
 */
tegrabl_error_t tegrabl_car_get_clk_src_rate(
		tegrabl_clk_src_id_t src_id,
		uint32_t *rate_khz)
{
	pr_trace("(%s,%d) %d\n", __func__, __LINE__, src_id);

	if ((src_id == TEGRABL_CLK_SRC_PLLC4_MUXED) && (pllc4_muxed_rate != 0)) {
		*rate_khz = pllc4_muxed_rate;
		return TEGRABL_NO_ERROR;
	}

	return internal_tegrabl_car_get_clk_rate(
			src_clk_tegrabl_to_bpmp(src_id),
			rate_khz);
}

/**
 * @brief - Set frequency for the specified
 * clock source.
 *
 * @src_id - enum of the clock source
 * @rate_khz - the frequency of the clock source
 * @rate_set_khz - Rate set
 * @return - TEGRABL_NO_ERROR if success, error-reason otherwise.
 */
tegrabl_error_t tegrabl_car_set_clk_src_rate(
		tegrabl_clk_src_id_t src_id,
		uint32_t rate_khz,
		uint32_t *rate_set_khz)
{
	pr_trace("(%s,%d) %d\n", __func__, __LINE__, src_id);

	if (src_id == TEGRABL_CLK_SRC_PLLC4_MUXED) {
		/* MUX SRC rate cannot be edited directly - Exception
		 *  Save the requested rate for future use */
		pllc4_muxed_rate = rate_khz;
		return TEGRABL_NO_ERROR;
	}

	return internal_tegrabl_car_set_clk_rate(
			src_clk_tegrabl_to_bpmp(src_id),
			rate_khz,
			rate_set_khz);
}
/**
 * @brief - Attempts to set the current clock rate of
 * the module to the value specified and returns the actual rate set.
 * NOTE: If the module clock is disabled when this function is called,
 * it will also enable the clock.
 *
 * @module - Module ID of the module
 * @instance - Instance of the module
 * @rate_khz - Rate requested
 * @rate_set_khz - Rate set
 * @return - TEGRABL_NO_ERROR if success, error-reason otherwise.
 */
tegrabl_error_t tegrabl_car_set_clk_rate(
		tegrabl_module_t module,
		uint8_t instance,
		uint32_t rate_khz,
		uint32_t *rate_set_khz)
{
	pr_trace("(%s,%d) %d, %d, %d\n", __func__, __LINE__,
			 module, instance, rate_khz);

	/* TODO - Add a condition to check if already enabled */
	tegrabl_car_clk_enable(module, instance, NULL);

	return internal_tegrabl_car_set_clk_rate(
			tegrabl_module_to_bpmp_id(module, instance, MOD_CLK),
			rate_khz,
			rate_set_khz);
}

/**
 * @brief - Configures the essential PLLs, Oscillator,
 * and other essential clocks.
 */
void tegrabl_car_clock_init(void)
{
	/* Since BPMP takes care of initializing clk, hence this init is empty */
	return;
}

/**
 * @brief - Returns the oscillator frequency in KHz
 *
 * @freq_khz - Pointer to store the freq in kHz
 * @return - TEGRABL_NO_ERROR if success, error-reason otherwise.
 */
tegrabl_error_t tegrabl_car_get_osc_freq_khz(uint32_t *freq_khz)
{
	pr_trace("(%s,%d)\n", __func__, __LINE__);

	return internal_tegrabl_car_get_clk_rate(
			TEGRA194_CLK_OSC,
			freq_khz);
}

/**
 * @brief - Initializes the pll specified by pll_id.
 * Does nothing if pll already initialized
 *
 * @pll_id - ID of the pll to be initialized
 * @rate_khz - Rate to which the PLL is to be initialized
 * @priv_data - Any PLL specific initialization data to send
 * @return - TEGRABL_NO_ERROR if success, error-reason otherwise.
 */
tegrabl_error_t tegrabl_car_init_pll_with_rate(
		tegrabl_clk_pll_id_t pll_id, uint32_t rate_khz,
		void *priv_data)
{
	uint32_t rate_set_khz; /* filler - to avoid NULL exception */
	uint32_t clk_id = tegrabl_pllid_to_bpmp_pllid[pll_id];
	bool state;

	TEGRABL_UNUSED(priv_data);

	pr_trace("(%s,%d) %d, %d\n", __func__, __LINE__, pll_id, rate_khz);
	/* Check if requested PLL is supported (at present) */
	if (clk_id == TEGRA194_MAX_CLK_ID)
		return TEGRABL_ERR_NOT_SUPPORTED;

	state = internal_tegrabl_car_clk_is_enabled(clk_id);
	/* Check if already initialized by BR/BL. If so, do nothing */
	if (state) {
		pr_trace("(%s) Requested PLL(%d) already enabled. skipping init\n",
				 __func__, pll_id);
		return TEGRABL_NO_ERROR;
	}

	/* Set rate */
	if (TEGRABL_NO_ERROR != internal_tegrabl_car_set_clk_rate(
			clk_id,
			rate_khz,
			&rate_set_khz)) {
		return TEGRABL_ERR_INVALID;
	}

	/* Enable PLL */
	if (TEGRABL_NO_ERROR != internal_tegrabl_car_clk_enable(clk_id)) {
		return TEGRABL_ERR_INVALID;
	}

	return TEGRABL_NO_ERROR;
}

static inline uint32_t div_round_off(uint32_t n, uint32_t d)
{
	if (((n) % (d)) >= ((d)/2))
		return (n) / (d) + 1;
	else
		return (n) / (d);
}

/**
 * @brief Configures the clock source and divider if needed
 * and enables clock for the module specified.
 *
 * @module - Module ID of the module
 * @instance - Instance of the module
 * @priv_data - module specific private data pointer to module specific clock
 * init data
 * @return - TEGRABL_NO_ERROR if success, error-reason otherwise.
 */
tegrabl_error_t tegrabl_car_clk_enable(tegrabl_module_t module,
					uint8_t instance,
					void *priv_data)
{
	tegrabl_error_t err = TEGRABL_NO_ERROR;

#if defined(CONFIG_ENABLE_QSPI)
	/* Do QSPI specific init based on priv_data */
	if ((module == TEGRABL_MODULE_QSPI) && (priv_data != NULL)) {
		uint32_t src, rate_khz, rate_set_khz, c4_mux_sel, c4_vco_khz;
		struct qspi_clk_data *clk_data;

		clk_data = (struct qspi_clk_data *)priv_data;

		/* Map TEGRABL_CLK_SRC ids to bpmp-abi clk ids */
		switch (clk_data->clk_src) {
		case TEGRABL_CLK_SRC_PLLC4_MUXED:
			/* Fetch exact parent info in case of C4_MUXED */
			/* 1. Obtain pllc4 vco freq */
			internal_tegrabl_car_get_clk_rate(TEGRA194_CLK_PLLC4,
											  &c4_vco_khz);
			/* 2. Calculate divisor */
			tegrabl_car_get_clk_src_rate(TEGRABL_CLK_SRC_PLLC4_MUXED,
										 &rate_set_khz);
			c4_mux_sel = div_round_off(c4_vco_khz, rate_set_khz);
			/* 3. Obtain parent info */
			pr_trace("c4 vco-div (mux selection) = %d\n", c4_mux_sel);
			switch (c4_mux_sel) {
			case (5):
				src = TEGRA194_CLK_PLLC4_OUT2;
				break;
			case (2):
				src = TEGRA194_CLK_PLLC4_VCO_DIV2;
				break;
			case (3):
			default:
				src = TEGRA194_CLK_PLLC4_OUT1;
			}
			break;
		case TEGRABL_CLK_SRC_CLK_M:
			src = TEGRA194_CLK_CLK_M;
			break;
		case TEGRABL_CLK_SRC_PLLC_OUT0:
			src = TEGRA194_CLK_PLLC;
			break;
		case TEGRABL_CLK_SRC_PLLP_OUT0:
		default:
			src = TEGRA194_CLK_PLLP_OUT0;
			break;
		}

		/* Set parent */
		err = internal_tegrabl_car_set_clk_src(qspi_module_instances[instance][MOD_CLK], src);
		if (err != TEGRABL_NO_ERROR) {
			goto fail;
		}

		/* Get parent rate */
		err = internal_tegrabl_car_get_clk_rate(src, &rate_set_khz);
		if (err != TEGRABL_NO_ERROR) {
			goto fail;
		}

		/* Derive clk rate from src rate and divisor */
		/* clk_rate = (pllp_freq * 2) / (N + 2) */
		rate_khz = (rate_set_khz << 1) / (clk_data->clk_divisor + 2);
		/* Round down rate to the nearest 1000 */
		rate_khz = ROUND_DOWN(rate_khz, 1000);

		err = tegrabl_qspi_sdr_enable(instance);
		if (err != TEGRABL_NO_ERROR) {
			goto fail;
		}

		pr_info("QSPI source rate = %d Khz\n", rate_set_khz);
		pr_info("Requested rate for QSPI clock = %d Khz\n", rate_khz);
		err = internal_tegrabl_car_set_clk_rate
				(qspi_module_instances[instance][MOD_CLK], rate_khz, &rate_set_khz);
		if (err != TEGRABL_NO_ERROR) {
			goto fail;
		}
		pr_info("BPMP-set rate for QSPI clk = %d Khz\n", rate_set_khz);

		/* Enable QSPI clk */
		err = internal_tegrabl_car_clk_enable(qspi_module_instances[instance][MOD_CLK]);
		if (err != TEGRABL_NO_ERROR) {
			goto fail;
		}

		return TEGRABL_NO_ERROR;
	}
#else
	TEGRABL_UNUSED(priv_data);
#endif /* CONFIG_ENABLE_QSPI */

	pr_trace("(%s,%d) %d, %d\n", __func__, __LINE__, module, instance);

	err = internal_tegrabl_car_clk_enable(tegrabl_module_to_bpmp_id(module, instance, MOD_CLK));

#if defined(CONFIG_ENABLE_QSPI)
fail:
#endif
	if (err != TEGRABL_NO_ERROR) {
		err = TEGRABL_ERROR_HIGHEST_MODULE(err);
	}

	return err;
}

/**
 * @brief  Disables clock for the module specified
 *
 * @module  Module ID of the module
 * @instance  Instance of the module
 * @return - TEGRABL_NO_ERROR if success, error-reason otherwise.
 */
tegrabl_error_t tegrabl_car_clk_disable(tegrabl_module_t module,
					uint8_t instance)
{
	pr_trace("(%s,%d) %d, %d\n", __func__, __LINE__,
			 module, instance);

	return internal_tegrabl_car_clk_disable(
			tegrabl_module_to_bpmp_id(module, instance, MOD_CLK));
}

/**
 * @brief Power downs plle.
 */
void tegrabl_car_disable_plle(void)
{
	pr_trace("(%s,%d)\n", __func__, __LINE__);

	internal_tegrabl_car_clk_disable(TEGRA194_CLK_PLLE);
}

/**
 * @brief - Configures PLLM0 for WB0 Override
 * @return - TEGRABL_NO_ERROR if success, error-reason otherwise.
 */
tegrabl_error_t tegrabl_car_pllm_wb0_override(void)
{
	/* Stub - Functionality not needed */
	return 0;
}

/**
 * @brief - Configures CAR dividers for slave TSC
 *
 * Configuration is done for both OSC and PLL paths.
 * If OSC >= 38400, Osc is chosen as source
 * else PLLP is chosen as source.
 *
 * @return - TEGRABL_NO_ERROR if success, error-reason otherwise.
 */
tegrabl_error_t tegrabl_car_setup_tsc_dividers(void)
{
	/* Stub - Functionality not needed */
	return TEGRABL_NO_ERROR;
}

/**
 * @brief - Set/Clear fuse register visibility
 *
 * @param visibility if true, it will make all reg visible otherwise invisible.
 *
 * @return existing visibility before programming the value
 */
bool tegrabl_set_fuse_reg_visibility(bool visibility)
{
	/* Stub to return TRUE
	 * BPMP fw keeps this enabled */
	TEGRABL_UNUSED(visibility);
	return true;
}

/**
 * @brief Puts the module in reset
 *
 * @module - Module ID of the module
 * @instance - Instance of the module
 * @return - TEGRABL_NO_ERROR if success, error-reason otherwise.
 */
tegrabl_error_t tegrabl_car_rst_set(tegrabl_module_t module,
				uint8_t instance)
{
	pr_trace("(%s,%d) %d, %d\n", __func__, __LINE__,
			 module, instance);

	/* Handle XUSB RST exceptions - These resets are controlled by PG sequence */
	switch (module) {
	case TEGRABL_MODULE_XUSBF:
	case TEGRABL_MODULE_XUSB_DEV:
	case TEGRABL_MODULE_XUSB_HOST:
	case TEGRABL_MODULE_XUSB_SS:
		return TEGRABL_NO_ERROR;
		break;
	default:
		/* Do nothing */
		break;
	}

	return internal_tegrabl_car_rst(
				tegrabl_module_to_bpmp_id(module, instance, MOD_RST),
				CMD_RESET_ASSERT);
}

/**
 * @brief  Releases the module from reset
 *
 * @module - Module ID of the module
 * @instance - Instance of the module
 * @return - TEGRABL_NO_ERROR if success, error-reason otherwise.
 */
tegrabl_error_t tegrabl_car_rst_clear(tegrabl_module_t module,
				uint8_t instance)
{
	pr_trace("(%s,%d) %d, %d\n", __func__, __LINE__,
			 module, instance);

	return internal_tegrabl_car_rst(
				tegrabl_module_to_bpmp_id(module, instance, MOD_RST),
				CMD_RESET_DEASSERT);
}

tegrabl_error_t tegrabl_car_clk_get_reset_state(tegrabl_module_t module, uint8_t instance, bool *state)
{
	TEGRABL_UNUSED(module);
	TEGRABL_UNUSED(instance);
	TEGRABL_UNUSED(state);
	/*BPMP IVC interface does not support reading reset status */
	return TEGRABL_ERROR(TEGRABL_ERR_NOT_SUPPORTED, 0);
}

/**
 * @brief - Returns the enum of oscillator frequency
 * @return - Enum value of current oscillator frequency
 */
tegrabl_clk_osc_freq_t tegrabl_get_osc_freq(void)
{
	tegrabl_clk_osc_freq_t return_freq = TEGRABL_CLK_OSC_FREQ_38_4;

	if (tegrabl_is_fpga()) {
		return_freq = TEGRABL_CLK_OSC_FREQ_19_2;
	}
	return return_freq;
}

void tegrabl_usbf_program_tracking_clock(bool is_enable)
{
	uint32_t dummy;
	if (is_enable) {
		/* Set tracking clock to 9600 MHz */
		(void)internal_tegrabl_car_clk_enable(TEGRA194_CLK_USB2_TRK);
		(void)internal_tegrabl_car_set_clk_src(TEGRA194_CLK_USB2_TRK, TEGRA194_CLK_OSC);
		(void)internal_tegrabl_car_set_clk_rate(TEGRA194_CLK_USB2_TRK, 9600, &dummy);
	} else {
		(void)internal_tegrabl_car_clk_disable(TEGRA194_CLK_USB2_TRK);
	}

	return;
}

tegrabl_error_t tegrabl_usbf_clock_init(void)
{
	uint32_t dummy;
	uint8_t index;
	tegrabl_error_t err = TEGRABL_NO_ERROR;

	uint32_t usbf_clk[3][3] = {
		/* clk_id, src, rate */
		{TEGRA194_CLK_XUSB_CORE_DEV, TEGRA194_CLK_PLLP_OUT0,         102000},
		{TEGRA194_CLK_XUSB_SS,       TEGRA194_CLK_UTMIPLL_CLKOUT480, 120000},
		{TEGRA194_CLK_XUSB_FS,       TEGRA194_CLK_UTMIPLL_CLKOUT48,   48000},
	};

	/* Array index for above (usbf_clk) */
	const uint8_t clk_id = 0;
	const uint8_t src_id = 1;
	const uint8_t rate = 2;

	struct mrq_pg_request xusb_pg_request = {
		.cmd = CMD_PG_SET_STATE,
		.id = TEGRA194_POWER_DOMAIN_XUSBA,
		.set_state = {
			.state = PG_STATE_ON,
		}
	};

	pr_trace("Programming XUSB clks\n");
	for (index = 0UL; index < 3UL; index++) {
		err = internal_tegrabl_car_set_clk_src(usbf_clk[index][clk_id], usbf_clk[index][src_id]);
		if ((err != TEGRABL_NO_ERROR) && (err != TEGRABL_ERR_NOT_SUPPORTED)) {
			goto fail;
		}
		err = internal_tegrabl_car_set_clk_rate(usbf_clk[index][clk_id], usbf_clk[index][rate], &dummy);
		if ((err != TEGRABL_NO_ERROR) && (err != TEGRABL_ERR_NOT_SUPPORTED)) {
			goto fail;
		}
	}

	/* unPowerGate XUSB */
	while (xusb_pg_request.id <= TEGRA194_POWER_DOMAIN_XUSBC) {
		err = tegrabl_ccplex_bpmp_xfer(&xusb_pg_request, NULL, sizeof(xusb_pg_request), 0, MRQ_PG);
		if (err != TEGRABL_NO_ERROR) {
			pr_error("PG_STATE_ON for %d failed. Skipping others.\n", xusb_pg_request.id);
			TEGRABL_SET_HIGHEST_MODULE(err);
			goto fail;
		} else {
			pr_trace("un-Powergated %d\n", xusb_pg_request.id);
		}
		++(xusb_pg_request.id);
	}
	err = TEGRABL_NO_ERROR;

fail:
	return err;
}

tegrabl_error_t tegrabl_usb_powergate(void)
{
	tegrabl_error_t err = TEGRABL_NO_ERROR;

	struct mrq_pg_request xusb_pg_request = {
		.cmd = CMD_PG_SET_STATE,
		.id = TEGRA194_POWER_DOMAIN_XUSBA,
		.set_state = {
			.state = PG_STATE_OFF,
		}
	};

	/* PowerGate XUSBA and XUSBC partitions */
	while (xusb_pg_request.id <= TEGRA194_POWER_DOMAIN_XUSBC) {
		err = tegrabl_ccplex_bpmp_xfer(&xusb_pg_request, NULL, sizeof(xusb_pg_request), 0, MRQ_PG);
		if (err != TEGRABL_NO_ERROR) {
			pr_error("PG_STATE_OFF for %d failed. Skipping others.\n", xusb_pg_request.id);
			TEGRABL_SET_HIGHEST_MODULE(err);
			goto fail;
		} else {
			pr_trace("Powergated %d\n", xusb_pg_request.id);
		}
		++(xusb_pg_request.id);
	}
	err = TEGRABL_NO_ERROR;

fail:
	return err;
}

tegrabl_error_t tegrabl_usb_host_clock_init(void)
{
	/* bpmp-fw takes care of clock initialization */
	return TEGRABL_NO_ERROR;
}

#if defined(CONFIG_ENABLE_QSPI)
static tegrabl_error_t tegrabl_qspi_clk_div_mode(uint8_t instance, bool val)
{
	tegrabl_error_t err = TEGRABL_NO_ERROR;
	uint32_t clk_rate;
	uint32_t qspi_clk_id;
	uint32_t div_clk_id;
	uint32_t dummy;
	/* Unknown value for the first call */
	static uint32_t prev_div2_sel = 0xa5a5a5a5U;

	pr_trace("QSPI clk div mode - input val = %u\n", val);

	if (instance >= ARRAY_SIZE(qspi_module_div2_instances)) {
		err = TEGRABL_ERROR(TEGRABL_ERR_NOT_SUPPORTED, 1);
		goto done;
	}

	qspi_clk_id = tegrabl_module_to_bpmp_id(TEGRABL_MODULE_QSPI, instance, MOD_CLK);
	div_clk_id = qspi_module_div2_instances[instance][MOD_CLK];

	/* Rate of QSPI */
	err = internal_tegrabl_car_get_clk_rate(qspi_clk_id, &clk_rate);
	if (err != TEGRABL_NO_ERROR) {
		goto done;
	}

	/* Obtain actual value via ivc interface, for the first time */
	if (prev_div2_sel == 0xa5a5a5a5U) {
		/* Rate of QSPI_PM */
		err = internal_tegrabl_car_get_clk_rate(div_clk_id, &prev_div2_sel);
		if (err != TEGRABL_NO_ERROR) {
			goto done;
		}

		/* QSPI and QSPI_PM would have different rates from bpmp-fw in DDR mode */
		if (clk_rate != prev_div2_sel) {
			/* DDR */
			prev_div2_sel = 1U;
		} else {
			prev_div2_sel = 0U;
		}
	}

	if (prev_div2_sel == (uint32_t)val) {
		/* Skip - no change in state */
		err = TEGRABL_NO_ERROR;
		goto no_err;
	}

	if (val) {
		/* ENABLE QSPI QDDR READ */
		err = internal_tegrabl_car_set_clk_rate(div_clk_id, 0x0U, &dummy);
		if (err != TEGRABL_NO_ERROR) {
			goto done;
		}
		/* Double frequency */
		clk_rate = clk_rate << 1U;
	} else {
		err = internal_tegrabl_car_set_clk_rate(div_clk_id, 0xFFFFFFFFU, &dummy);
		if (err != TEGRABL_NO_ERROR) {
			goto done;
		}
		/* Halve frequency */
		clk_rate = clk_rate >> 1U;
	}

	err = internal_tegrabl_car_set_clk_rate(qspi_clk_id, clk_rate, &dummy);
	if (err != TEGRABL_NO_ERROR) {
		goto done;
	}

no_err:
	/* Store the current state for next calls */
	prev_div2_sel = (uint32_t)val;

done:
	return err;
}

tegrabl_error_t tegrabl_qspi_ddr_enable(uint8_t instance)
{
	return tegrabl_qspi_clk_div_mode(instance, true);
}

tegrabl_error_t tegrabl_qspi_sdr_enable(uint8_t instance)
{
	return tegrabl_qspi_clk_div_mode(instance, false);
}
#endif /* CONFIG_ENABLE_QSPI */

void tegrabl_clk_uphy_lane_select_pll(uint32_t lane, uint32_t pll_sel)
{
	/* Do nothing. Users of this interface not allowed to actually program */
	TEGRABL_UNUSED(lane);
	TEGRABL_UNUSED(pll_sel);
	return;
}

static tegrabl_error_t internal_tegrabl_clk_pll_hw_sequencer_state(
										tegrabl_clk_pll_id_t pll_id,
										bool state)
{
	tegrabl_error_t error = TEGRABL_NO_ERROR;
	uint32_t bpmp_clk_id = MODULE_NOT_SUPPORTED;

	switch (pll_id) {
	case TEGRABL_CLK_PLL_ID_UTMI_PLL:
		bpmp_clk_id = TEGRA194_CLK_UTMIPLL;
		break;
	case TEGRABL_CLK_PLL_ID_PLLE:
		bpmp_clk_id = TEGRA194_CLK_PLLE_HPS;
		break;
	default:
		pr_warn("HPS for pll_id %u not supported\n", pll_id);
		error = TEGRABL_ERROR(TEGRABL_ERR_NOT_SUPPORTED, 0);
		break;
	}

	if (error != TEGRABL_NO_ERROR) {
		goto done;
	}

	if (state) {
		error = internal_tegrabl_car_clk_enable(bpmp_clk_id);
	} else {
		error = internal_tegrabl_car_clk_disable(bpmp_clk_id);
	}
done:
	return error;
}

tegrabl_error_t tegrabl_clk_pll_hw_sequencer_enable(
							bool spread_enable,
							tegrabl_clk_pll_id_t pll_id)
{
	TEGRABL_UNUSED(spread_enable);
	return internal_tegrabl_clk_pll_hw_sequencer_state(pll_id, true);
}

tegrabl_error_t tegrabl_clk_pll_hw_sequencer_disable(tegrabl_clk_pll_id_t pll_id)
{
	return internal_tegrabl_clk_pll_hw_sequencer_state(pll_id, false);
}
