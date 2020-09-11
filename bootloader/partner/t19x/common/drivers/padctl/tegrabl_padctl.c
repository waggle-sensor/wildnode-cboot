/*
 * Copyright (c) 2018-2019, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#include <tegrabl_drf.h>
#include <tegrabl_io.h>
#include <tegrabl_debug.h>
#include <tegrabl_padctl.h>
#include <tegrabl_ar_macro.h>
#include <arpadctl_EDP.h>
#include <address_map_new.h>

static uint32_t pad_dp_aux[] = {
	PADCTL_EDP_DP_AUX_CH0_HPD_0,
	PADCTL_EDP_DP_AUX_CH1_HPD_0,
	PADCTL_EDP_DP_AUX_CH2_HPD_0,
	PADCTL_EDP_DP_AUX_CH3_HPD_0,
};

#define padctl_read(reg) \
	NV_READ32(NV_ADDRESS_MAP_PADCTL_A16_BASE + reg)

#define padctl_writel(reg, value) \
	NV_WRITE32(NV_ADDRESS_MAP_PADCTL_A16_BASE + reg, value)

void tegrabl_padctl_config_to_gpio(uint32_t pin_num)
{
	uint32_t val;

	/*we have 4 pins (M,0) (M,1) (M,2) (M,3) corresponding to pin num 96 to 99 in T194
	 * GPIO(M,0) = 96 -> dp_aux_ch0_hpd_pm0
	 * GPIO(M,1) = 97 -> dp_aux_ch0_hpd_pm1
	 * GPIO(M,2) = 98 -> dp_aux_ch0_hpd_pm2
	 * GPIO(M,3) = 99 -> dp_aux_ch0_hpd_pm3
	 */
	if ((pin_num - 96) > (ARRAY_SIZE(pad_dp_aux) - 1)) {
		pr_warn("%s: array size of pad_dp_aux exceeded\n", __func__);
		return;
	}
	val = padctl_read(pad_dp_aux[pin_num - 96]);
	val = NV_FLD_SET_DRF_DEF(PADCTL_EDP, DP_AUX_CH0_HPD, GPIO_SF_SEL, GPIO, val);
	val = NV_FLD_SET_DRF_DEF(PADCTL_EDP, DP_AUX_CH0_HPD, E_LPDR, ENABLE, val);
	val = NV_FLD_SET_DRF_DEF(PADCTL_EDP, DP_AUX_CH0_HPD, PUPD, NONE, val);

	padctl_writel(pad_dp_aux[pin_num - 96], val);
}

