/*
 * Copyright (c) 2017, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#include <tegrabl_uphy.h>
#include <tegrabl_clock.h>

static struct tegrabl_uphy_lane uphy_lanes[] =  {
	{
		.module_id = TEGRABL_CLK_PEX_USB_UPHY_L0_RST,
		.base = NV_ADDRESS_MAP_UPHY_LANE0_BASE,
		.owner = TEGRABL_UPHY_UNASSIGNED,
	},
	{
		.module_id = TEGRABL_CLK_PEX_USB_UPHY_L1_RST,
		.base = NV_ADDRESS_MAP_UPHY_LANE1_BASE,
		.owner = TEGRABL_UPHY_UNASSIGNED,
	},
	{
		.module_id = TEGRABL_CLK_PEX_USB_UPHY_L2_RST,
		.base = NV_ADDRESS_MAP_UPHY_LANE2_BASE,
		.owner = TEGRABL_UPHY_UNASSIGNED,
	},
	{
		.module_id = TEGRABL_CLK_PEX_USB_UPHY_L3_RST,
		.base = NV_ADDRESS_MAP_UPHY_LANE3_BASE,
		.owner = TEGRABL_UPHY_UNASSIGNED,
	},
	{
		.module_id = TEGRABL_CLK_PEX_USB_UPHY_L4_RST,
		.base = NV_ADDRESS_MAP_UPHY_LANE4_BASE,
		.owner = TEGRABL_UPHY_UNASSIGNED,
	},
	{
		.module_id = TEGRABL_CLK_PEX_USB_UPHY_L5_RST,
		.base = NV_ADDRESS_MAP_UPHY_LANE5_BASE,
		.owner = TEGRABL_UPHY_UNASSIGNED,
	},
	{
		.module_id = TEGRABL_CLK_PEX_USB_UPHY_L6_RST,
		.base = NV_ADDRESS_MAP_UPHY_LANE6_BASE,
		.owner = TEGRABL_UPHY_UNASSIGNED,
	},
	{
		.module_id = TEGRABL_CLK_PEX_USB_UPHY_L7_RST,
		.base = NV_ADDRESS_MAP_UPHY_LANE7_BASE,
		.owner = TEGRABL_UPHY_UNASSIGNED,
	},
	{
		.module_id = TEGRABL_CLK_PEX_USB_UPHY_L8_RST,
		.base = NV_ADDRESS_MAP_UPHY_LANE8_BASE,
		.owner = TEGRABL_UPHY_UNASSIGNED,
	},
	{
		.module_id = TEGRABL_CLK_PEX_USB_UPHY_L9_RST,
		.base = NV_ADDRESS_MAP_UPHY_LANE9_BASE,
		.owner = TEGRABL_UPHY_UNASSIGNED,
	},
	{
		.module_id = TEGRABL_CLK_PEX_USB_UPHY_L10_RST,
		.base = NV_ADDRESS_MAP_UPHY_LANE10_BASE,
		.owner = TEGRABL_UPHY_UNASSIGNED,
	},
	{
		.module_id = TEGRABL_CLK_PEX_USB_UPHY_L11_RST,
		.base = NV_ADDRESS_MAP_UPHY_LANE11_BASE,
		.owner = TEGRABL_UPHY_UNASSIGNED,
	},
};

static struct tegrabl_uphy_pll uphy_pll[] = {
	{
		.module_id = TEGRABL_CLK_PEX_USB_UPHY_PLL0_RST,
		.base = NV_ADDRESS_MAP_UPHY_PLL0_BASE,
	},
	{
		.module_id = TEGRABL_CLK_PEX_USB_UPHY_PLL1_RST,
		.base = NV_ADDRESS_MAP_UPHY_PLL1_BASE,
	},
	{
		.module_id = TEGRABL_CLK_PEX_USB_UPHY_PLL2_RST,
		.base = NV_ADDRESS_MAP_UPHY_PLL2_BASE,
	},
	{
		.module_id = TEGRABL_CLK_PEX_USB_UPHY_PLL3_RST,
		.base = NV_ADDRESS_MAP_UPHY_PLL3_BASE,
	},
};
