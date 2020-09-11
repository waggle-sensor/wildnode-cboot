/*
 * Copyright (c) 2017-2018, NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#define _MK_ENUM_CONST(_constant_) (_constant_ ## UL)

#define PLLP_FIXED_FREQ_KHZ_13000            13000
#define PLLP_FIXED_FREQ_KHZ_216000          216000
#define PLLP_FIXED_FREQ_KHZ_408000          408000
#define PLLP_FIXED_FREQ_KHZ_432000          432000

/**
 * @brief enum for plls that might be used. Not all of them might be
 * supported. Add new plls to this list and update the clock driver add
 * support for the new pll.
 */
typedef uint32_t tegrabl_clk_pll_id_t;
#define TEGRABL_CLK_PLL_ID_PLLP 0U					/* 0x0 */
#define TEGRABL_CLK_PLL_ID_PLLC4 1U					/* 0x1 */
#define TEGRABL_CLK_PLL_ID_PLLD 2U					/* 0x2 */
#define TEGRABL_CLK_PLL_ID_PLLD2 3U					/* 0x3 */
	/* TEGRABL_CLK_PLL_ID_PLLD3 at 14 */
#define TEGRABL_CLK_PLL_ID_PLLDP 4U					/* 0x4 */
#define TEGRABL_CLK_PLL_ID_PLLE 5U					/* 0x5 */
#define TEGRABL_CLK_PLL_ID_PLLM 6U					/* 0x6 */
#define TEGRABL_CLK_PLL_ID_SATA_PLL 7U					/* 0x7 */
#define TEGRABL_CLK_PLL_ID_UTMI_PLL 8U					/* 0x8 */
#define TEGRABL_CLK_PLL_ID_XUSB_PLL 9U					/* 0x9 */
#define TEGRABL_CLK_PLL_ID_AON_PLL 10U					/* 0xA */
#define TEGRABL_CLK_PLL_ID_PLLDISPHUB 11U				/* 0xB */
#define TEGRABL_CLK_PLL_ID_PLL_NUM 12U					/* 0xC */
#define TEGRABL_CLK_PLL_ID_PLLMSB 13U					/* 0xD */
#define TEGRABL_CLK_PLL_ID_PLLD3 14U					/* 0xE */
	/* PLLM at 6; PLLMSB at 13 */
#define TEGRABL_CLK_PLL_ID_PLLMSC 15U					/* 0xF */
#define TEGRABL_CLK_PLL_ID_PLLMSD 16U					/* 0x10 */
#define TEGRABL_CLK_PLL_ID_PLLMB 17U					/* 0x11 */
#define TEGRABL_CLK_PLL_ID_PLLMSBB 18U					/* 0x12 */
#define TEGRABL_CLK_PLL_ID_PLLMSCB 19U					/* 0x13 */
#define TEGRABL_CLK_PLL_ID_PLLMSDB 20U					/* 0x14 */
#define TEGRABL_CLK_PLL_ID_PLLC 21U					/* 0x15 */
#define TEGRABL_CLK_PLL_ID_PLLC2 22U					/* 0x16 */
#define TEGRABL_CLK_PLL_ID_PLLC3 23U					/* 0x17 */
#define TEGRABL_CLK_PLL_ID_PLLNVHS 24U					/* 0x18 */
#define TEGRABL_CLK_PLL_ID_HSIO_UPHY_PLL0 25U				/* 0x19 */
#define TEGRABL_CLK_PLL_ID_HSIO_UPHY_PLL1 26U				/* 0x1A */
#define TEGRABL_CLK_PLL_ID_HSIO_UPHY_PLL2 27U				/* 0x1B */
#define TEGRABL_CLK_PLL_ID_HSIO_UPHY_PLL3 28U				/* 0x1C */
#define TEGRABL_CLK_PLL_ID_PLLREFE 29U					/* 0x1D */
#define TEGRABL_CLK_PLL_ID_MAX 255U					/* 0xFF */
/**
 * @brief - enum for possible module clock divisors
 * @TEGRABL_CLK_DIV_TYPE_REGULAR - Divide by (N + 1)
 * @TEGRABL_CLK_DIV_TYPE_FRACTIONAL - Divide by (N/2 + 1)
 * where N is the divisor value written to the clock source register
 * one of the PLLs). Not all of them are supported.
 */
/* macro tegrabl clk div type */
typedef uint8_t tegrabl_clk_div_type_t;
#define TEGRABL_CLK_DIV_TYPE_INVALID 0x0U
#define TEGRABL_CLK_DIV_TYPE_REGULAR 0x1U
#define TEGRABL_CLK_DIV_TYPE_FRACTIONAL 0x2U

/**
 * @brief - enum for possible clock sources
 * Add new sources to this list and update tegrabl_clk_get_src_freq()
 * to take care of the newly added source (usually a derivative of one of
 * one of the PLLs). Not all of them are supported.
 */
/* macro tegrabl clk src id */
typedef uint8_t tegrabl_clk_src_id_t;
#define TEGRABL_CLK_SRC_INVALID 0x0U
#define TEGRABL_CLK_SRC_CLK_M 0x1U
#define TEGRABL_CLK_SRC_CLK_S 0x2U /* 0x2 */
#define TEGRABL_CLK_SRC_PLLP_OUT0 0x3U
#define TEGRABL_CLK_SRC_PLLM_OUT0 0x4U /* 0x4 */
#define TEGRABL_CLK_SRC_PLLC_OUT0 0x5U
#define TEGRABL_CLK_SRC_PLLC4_MUXED 0x6U /* 0x6 */
#define TEGRABL_CLK_SRC_PLLC4_VCO 0x7U
#define TEGRABL_CLK_SRC_PLLC4_OUT0_LJ 0x8U /* 0x8 */
#define TEGRABL_CLK_SRC_PLLC4_OUT1 0x9U
#define TEGRABL_CLK_SRC_PLLC4_OUT1_LJ 0xaU /* 0xA */
#define TEGRABL_CLK_SRC_PLLC4_OUT2 0xbU
#define TEGRABL_CLK_SRC_PLLC4_OUT2_LJ 0xcU /* 0xC */
#define TEGRABL_CLK_SRC_PLLE 0xdU
#define TEGRABL_CLK_SRC_PLLAON_OUT 0xeU /* 0xE */
#define TEGRABL_CLK_SRC_PLLD_OUT1 0xfU
#define TEGRABL_CLK_SRC_PLLD2_OUT0 0x10U /* 0x10 */
#define TEGRABL_CLK_SRC_PLLD3_OUT0 0x11U
#define TEGRABL_CLK_SRC_PLLDP 0x12U /* 0x12 */
#define TEGRABL_CLK_SRC_NVDISPLAY_P0_CLK 0x13U
#define TEGRABL_CLK_SRC_NVDISPLAY_P1_CLK 0x14U /* 0x14 */
#define TEGRABL_CLK_SRC_NVDISPLAY_P2_CLK 0x15U
#define TEGRABL_CLK_SRC_SOR0 0x16U /* 0x16*/
#define TEGRABL_CLK_SRC_SOR1 0x17U
#define TEGRABL_CLK_SRC_SOR_SAFE_CLK 0x18U /* 0x18 */
#define TEGRABL_CLK_SRC_SOR0_PAD_CLKOUT 0x19U
#define TEGRABL_CLK_SRC_SOR1_PAD_CLKOUT 0x1aU /* 0x1A */
#define TEGRABL_CLK_SRC_DFLLDISP_DIV 0x1bU
#define TEGRABL_CLK_SRC_PLLDISPHUB_DIV 0x1cU /* 0x1C */
#define TEGRABL_CLK_SRC_PLLDISPHUB 0x1dU
#define TEGRABL_CLK_SRC_OSC_UNDIV 0x1eU /* 0x1E */
#define TEGRABL_CLK_SRC_PLLM_UD 0x1fU
#define TEGRABL_CLK_SRC_PLLMB_UD 0x20U /* 0x20 */
#define TEGRABL_CLK_SRC_PLLMB_OUT0 0x21U
#define TEGRABL_CLK_SRC_PLLREFE_VCOCLK 0x22U /* 0x22 */
#define TEGRABL_CLK_SRC_PLLC2_OUT0 0x23U
#define TEGRABL_CLK_SRC_PLLC3_OUT0 0x24U /* 0x24 */
#define TEGRABL_CLK_SRC_SOR2 0x25U
#define TEGRABL_CLK_SRC_SOR3 0x26U /* 0x26 */
#define TEGRABL_CLK_SRC_SOR2_PAD_CLKOUT 0x27U
#define TEGRABL_CLK_SRC_SOR3_PAD_CLKOUT 0x28U /* 0x28 */
#define TEGRABL_CLK_SRC_DUMMY 0x29U
#define TEGRABL_CLK_SRC_NUM 0xFFU

/*
 * @brief - enum for possible set of oscillator frequencies
 * supported in the internal API + invalid (measured but not in any valid band)
 * + unknown (not measured at all)
 * Define tegrabl_clk_osc_freq here to have the correct collection of
 * oscillator frequencies.
 */
/* macro tegrabl clk osc freq */
typedef uint32_t tegrabl_clk_osc_freq_t;
	/* Specifies an oscillator frequency of 19.2MHz. */
#define TEGRABL_CLK_OSC_FREQ_19_2 _MK_ENUM_CONST(4)
	/* Specifies an oscillator frequency of 38.4MHz. */
#define TEGRABL_CLK_OSC_FREQ_38_4 _MK_ENUM_CONST(5)

/*
 * @brief - enum for possible set of mphy clocks
 */
/* macro tegrabl clk mphy instance */
typedef uint32_t tegrabl_clk_mphy_instance_t;
	/* RST only */
#define TEGRABL_CLK_MPHY_IOBIST_RST 0U
#define TEGRABL_CLK_MPHY_CLK_CTL_RST 1U
#define TEGRABL_CLK_MPHY_L1_RX_RST 2U
#define TEGRABL_CLK_MPHY_L1_TX_RST 3U
#define TEGRABL_CLK_MPHY_L0_RX_RST 4U
#define TEGRABL_CLK_MPHY_L0_TX_RST 5U
	/* Enable only */
#define TEGRABL_CLK_MPHY_CORE_PLL_FIXED 6U
#define TEGRABL_CLK_MPHY_TX_1MHZ_REF 7U
#define TEGRABL_CLK_MPHY_IOBIST 8U
#define TEGRABL_CLK_MPHY_L1_RX_ANA 9U
#define TEGRABL_CLK_MPHY_L0_RX_ANA 10U
#define TEGRABL_CLK_MPHY_L0_TX_LS_3XBIT 11U
#define TEGRABL_CLK_MPHY_L0_TX_SYMB 12U
#define TEGRABL_CLK_MPHY_L0_RX_LS_BIT 13U
#define TEGRABL_CLK_MPHY_L0_RX_SYMB 14U
#define TEGRABL_CLK_MPHY_FORCE_LS_MODE 15U
#define TEGRABL_CLK_MPHY_MAX_INSTANCES 16U


/*
 * @brief - enum for possible set of ufs clocks
 */
/* macro tegrabl clk ufs instance */
typedef uint32_t tegrabl_clk_ufs_instance_t;
	/* RST only */
#define TEGRABL_CLK_UFSHC_RST 0U
#define TEGRABL_CLK_UFSHC_AXI_M_RST 1U
#define TEGRABL_CLK_UFSHC_LP_RST 2U
	/* Enable only */
#define TEGRABL_CLK_UFSDEV_REF 3U
#define TEGRABL_CLK_UFSHC 4U
#define TEGRABL_CLK_UFS_MAX_INSTANCES 5U

/*
 * @brief - enum for possible set of pex_usb_uphy clocks
 */
typedef uint32_t tegrabl_clk_pex_usb_uphy_instance_t;
	/* RST only */
#define TEGRABL_CLK_PEX_USB_UPHY_L11_RST 0U
#define TEGRABL_CLK_PEX_USB_UPHY_L10_RST 1U
#define TEGRABL_CLK_PEX_USB_UPHY_L9_RST 2U
#define TEGRABL_CLK_PEX_USB_UPHY_L8_RST 3U
#define TEGRABL_CLK_PEX_USB_UPHY_L7_RST 4U
#define TEGRABL_CLK_PEX_USB_UPHY_L6_RST 5U
#define TEGRABL_CLK_PEX_USB_UPHY_L5_RST 6U
#define TEGRABL_CLK_PEX_USB_UPHY_L4_RST 7U
#define TEGRABL_CLK_PEX_USB_UPHY_L3_RST 8U
#define TEGRABL_CLK_PEX_USB_UPHY_L2_RST 9U
#define TEGRABL_CLK_PEX_USB_UPHY_L1_RST 10U
#define TEGRABL_CLK_PEX_USB_UPHY_L0_RST 11U
#define TEGRABL_CLK_PEX_USB_UPHY_PLL1_RST 12U
#define TEGRABL_CLK_PEX_USB_UPHY_PLL0_RST 13U
#define TEGRABL_CLK_PEX_USB_UPHY_PLL2_RST 14U
#define TEGRABL_CLK_PEX_USB_UPHY_PLL3_RST 15U
#define TEGRABL_CLK_PEX_USB_UPHY_RST 16U
#define TEGRABL_CLK_PEX_USB_UPHY_MAX_INSTANCES 17U

/*
 * @brief - enum for possible set of pex_usb_uphy_pll_mgmnt clocks
 */
typedef uint32_t tegrabl_clk_pex_usb_uphy_pll_mgmnt_instance_t;
#define TEGRABL_CLK_PEX_USB_PAD_PLL0_MGMT 0U
#define TEGRABL_CLK_PEX_USB_PAD_PLL1_MGMT 1U
#define TEGRABL_CLK_PEX_USB_PAD_PLL2_MGMT 2U
#define TEGRABL_CLK_PEX_USB_PAD_PLL3_MGMT 3U

/*
 * @brief - enum for possible set of eqos clocks
 */
/* macro tegrabl clk eqos instance */
typedef uint32_t tegrabl_clk_eqos_instance_t;
/* Enable only */
#define TEGRABL_CLK_EQOS_AXI			0
#define TEGRABL_CLK_EQOS_PTP_REF		1
#define TEGRABL_CLK_EQOS_RX				2
#define TEGRABL_CLK_EQOS_TX				3
#define TEGRABL_CLK_EQOS_RST			4
/* Reset only */
#define TEGRABL_CLK_EQOS_MAX_INSTANCES	5

/**
 * @brief Enables the HW power sequencer for selected PLL
 *
 * @spread_enable - Specify if spread needs to be enabled by State machine
 * @pll_id - PLL ID <PLLE or PLLNVHS>
 * @return - TEGRABL_NO_ERROR if success, error-reason otherwise.
 */
tegrabl_error_t tegrabl_clk_pll_hw_sequencer_enable(
						bool spread_enable,
						tegrabl_clk_pll_id_t pll_id);

/**
 * @brief Disables the HW power sequencer for selected PLL
 *
 * @pll_id - PLL ID <PLLE or PLLNVHS>
 * @return - TEGRABL_NO_ERROR if success, error-reason otherwise.
 */
tegrabl_error_t tegrabl_clk_pll_hw_sequencer_disable(
							tegrabl_clk_pll_id_t pll_id);

#if defined(CONFIG_ENABLE_QSPI)
/**
 * @brief Set Qspi_clk ddr divisor mode for read operation
 *
 * @param instance qspi instance
 *
 * @retval TEGRABL_NO_ERROR No err
 */
tegrabl_error_t tegrabl_qspi_ddr_enable(uint8_t instance);

/**
 * @brief Set Qspi_clk sdr divisor mode for read operation
 *
 * @param instance qspi instance
 *
 * @retval TEGRABL_NO_ERROR No err
 */
tegrabl_error_t tegrabl_qspi_sdr_enable(uint8_t instance);
#endif /* CONFIG_ENABLE_QSPI */

/**
 * @brief Set clocks for uphy lanes
 *
 * @param lane uphy lane
 *
 * @param pll_sel PLL to be selected
 *
 */
void tegrabl_clk_uphy_lane_select_pll(uint32_t lane, uint32_t pll_sel);

/**
 * @brief init usb host clocks
 *
 * @return TEGRABL_NO_ERROR if success, error-reason otherwise.
 */
tegrabl_error_t tegrabl_usb_host_clock_init(void);
