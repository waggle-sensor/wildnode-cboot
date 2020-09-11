/*
 * Copyright (c) 2016-2019, NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */
#define MODULE TEGRABL_ERR_NVDISP

#include <stdint.h>
#include <stdbool.h>
#include <tegrabl_ar_macro.h>
#include <ardisplay.h>
#include <ardisplay_a.h>
#include <tegrabl_error.h>
#include <tegrabl_debug.h>
#include <tegrabl_io.h>
#include <tegrabl_drf.h>
#include <tegrabl_addressmap.h>
#include <tegrabl_clock.h>
#include <tegrabl_timer.h>
#include <tegrabl_malloc.h>
#include <tegrabl_surface.h>
#include <tegrabl_nvdisp.h>
#include <tegrabl_nvdisp_local.h>
#include <tegrabl_nvdisp_win.h>
#include <tegrabl_nvdisp_cmu.h>
#include <tegrabl_module.h>
#include <tegrabl_display_dtb.h>
#include <tegrabl_bpmp_fw_interface.h>

#define NO_COLOR 0
#define KHZ 1000
#define TEGRABL_DP_CLK 270000 /* 270MHz */

/* Because of the upward page alignment done in kernel we are seeing linear
 * mapping failure for the lut_mem memory region passed by BL in cmdline.
 * For proper linear mapping in kernel, start address and size should be
 * page aligned for smmu mapping. In kernel, page alignment is always done to
 * next page boundary.
 * changing the alignment from actual value of 256 to 4k as required in kernel.
*/
#define CMU_ALLIGNMENT_SIZE 4096
#define CP_ALLIGNMENT_SIZE 4096

static int32_t sor_instance;

uint32_t nvdisp_base_addr[] = {
	NV_ADDRESS_MAP_DISPLAY_BASE,
	NV_ADDRESS_MAP_DISPLAYB_BASE,
	NV_ADDRESS_MAP_DISPLAYC_BASE,
};

static tegrabl_error_t nvdisp_out_init(struct tegrabl_nvdisp *nvdisp,
									   struct tegrabl_display_pdata *pdata)
{
	tegrabl_error_t err = TEGRABL_NO_ERROR;

	pr_debug("%s: entry\n", __func__);
	switch (nvdisp->type) {
	case DISPLAY_OUT_HDMI:
		nvdisp->out_ops = &hdmi_ops;
		break;
#if defined(CONFIG_ENABLE_DP)
	case DISPLAY_OUT_DP:
		nvdisp->out_ops = &dp_ops;
		break;
#endif
#if defined(CONFIG_ENABLE_DSI)
	case DISPLAY_OUT_DSI:
		nvdisp->out_ops = &dsi_ops;
		break;
#endif
	default:
		nvdisp->out_ops = NULL;
		break;
	}

	if (nvdisp->out_ops && nvdisp->out_ops->init) {
		err = nvdisp->out_ops->init(nvdisp, pdata);
		if (err != TEGRABL_NO_ERROR) {
			nvdisp->out_ops = NULL;
			TEGRABL_SET_HIGHEST_MODULE(err);
		}
	}
	pr_debug("%s: exit\n", __func__);
	return err;
}

static tegrabl_error_t nvdisp_set_color_control(struct tegrabl_nvdisp *nvdisp)
{
	tegrabl_error_t err = TEGRABL_NO_ERROR;
	uint32_t val = nvdisp_readl(nvdisp, DISP_DISP_COLOR_CONTROL);

	switch (nvdisp->depth) {
	case 0:
		val = NV_FLD_SET_DRF_DEF(DC, DISP_DISP_COLOR_CONTROL, BASE_COLOR_SIZE,
								 BASE666, val);
		break;

	case 8:
		val = NV_FLD_SET_DRF_DEF(DC, DISP_DISP_COLOR_CONTROL, BASE_COLOR_SIZE,
								 BASE888, val);
		break;

	case 10:
		val = NV_FLD_SET_DRF_DEF(DC, DISP_DISP_COLOR_CONTROL, BASE_COLOR_SIZE,
								 BASE101010, val);
		break;

	case 12:
		val = NV_FLD_SET_DRF_DEF(DC, DISP_DISP_COLOR_CONTROL, BASE_COLOR_SIZE,
								 BASE121212, val);
		break;

	default:
		val = NV_FLD_SET_DRF_DEF(DC, DISP_DISP_COLOR_CONTROL, BASE_COLOR_SIZE,
								 BASE888, val);
		break;
	}

	switch (nvdisp->dither) {
	case NVDISP_UNDEFINED_DITHER:
	case NVDISP_DISABLE_DITHER:
		val = NV_FLD_SET_DRF_DEF(DC, DISP_DISP_COLOR_CONTROL, DITHER_CONTROL,
								 DISABLE, val);
		break;
	case NVDISP_ORDERED_DITHER:
		val = NV_FLD_SET_DRF_DEF(DC, DISP_DISP_COLOR_CONTROL, DITHER_CONTROL,
								 ORDERED, val);
		break;
	case NVDISP_TEMPORAL_DITHER:
		val = NV_FLD_SET_DRF_DEF(DC, DISP_DISP_COLOR_CONTROL, DITHER_CONTROL,
								 TEMPORAL, val);
		break;
	case NVDISP_ERRDIFF_DITHER:
		val = NV_FLD_SET_DRF_DEF(DC, DISP_DISP_COLOR_CONTROL, DITHER_CONTROL,
								 ERR_ACC, val);
		break;
	default:
		err = TEGRABL_ERROR(TEGRABL_ERR_NOT_SUPPORTED, 0);
		goto fail;
	}

	nvdisp_writel(nvdisp, DISP_DISP_COLOR_CONTROL, val);

fail:
	return err;
}

static tegrabl_error_t nvdisp_get_h_ref_to_sync(struct nvdisp_mode *mode)
{
	int32_t a, b;
	tegrabl_error_t err = TEGRABL_NO_ERROR;

	/* Constraint 5: H_REF_TO_SYNC >= 0 */
	a = 0;

	/* Constraint 6: H_FRONT_PORT >= (H_REF_TO_SYNC + 1) */
	b = mode->h_front_porch - 1;

	/* Constraint 1: H_REF_TO_SYNC + H_SYNC_WIDTH + H_BACK_PORCH > 11 */
	if ((a + mode->h_sync_width + mode->h_back_porch) <= 11) {
		a = 1 + 11 - mode->h_sync_width - mode->h_back_porch;
	}

	/* check Constraint 1 and 6 */
	if (a > b) {
		err = TEGRABL_ERROR(TEGRABL_ERR_INVALID, 0);
		goto fail;
	}

	/* Constraint 4: H_SYNC_WIDTH >= 1 */
	if (mode->h_sync_width < 1) {
		err = TEGRABL_ERROR(TEGRABL_ERR_INVALID, 1);
		goto fail;
	}

	/* Constraint 7: H_DISP_ACTIVE >= 16 */
	if (mode->h_active < 16) {
		err = TEGRABL_ERROR(TEGRABL_ERR_INVALID, 2);
		goto fail;
	}

	if ((b > a) && (a % 2))
		mode->h_ref_to_sync = a + 1;
	else
		mode->h_ref_to_sync = a;

fail:
	return err;
}

static tegrabl_error_t nvdisp_get_v_ref_to_sync(struct nvdisp_mode *mode)
{
	uint32_t a;
	tegrabl_error_t err = TEGRABL_NO_ERROR;

	/* Constraint 5: V_REF_TO_SYNC >= 1 */
	a = mode->v_front_porch - 1;

	/* Constraint 2: V_REF_TO_SYNC + V_SYNC_WIDTH + V_BACK_PORCH > 1 */
	if ((a + mode->v_sync_width + mode->v_back_porch) <= 1) {
		a = 1 + 1 - mode->v_sync_width - mode->v_back_porch;
	}

	/* Constraint 6 */
	if (mode->v_front_porch < (a + 1)) {
		a = mode->v_front_porch - 1;
	}

	/* Constraint 4: V_SYNC_WIDTH >= 1 */
	if (mode->v_sync_width < 1) {
		err = TEGRABL_ERROR(TEGRABL_ERR_INVALID, 3);
		goto fail;
	}

	/* Constraint 7: V_DISP_ACTIVE >= 16 */
	if (mode->v_active < 16) {
		err = TEGRABL_ERROR(TEGRABL_ERR_INVALID, 4);
		goto fail;
	}

	mode->v_ref_to_sync = a;

fail:
	return err;
}

static tegrabl_error_t nvdisp_check_timing_constraints(struct nvdisp_mode *mode)
{
	tegrabl_error_t err = TEGRABL_NO_ERROR;

	/* Constraint 1: H_REF_TO_SYNC + H_SYNC_WIDTH + H_BACK_PORCH > 20 */
	if (mode->h_ref_to_sync + mode->h_sync_width + mode->h_back_porch <= 20) {
		pr_debug("display timings constraint 1 failed\n");
		err = TEGRABL_ERROR(TEGRABL_ERR_INVALID, 5);
		goto fail;
	}

	/* Constraint 2: V_REF_TO_SYNC + V_SYNC_WIDTH + V_BACK_PORCH > 1 */
	if (mode->v_ref_to_sync + mode->v_sync_width + mode->v_back_porch <= 1) {
		pr_debug("display timings constraint 2 failed\n");
		err = TEGRABL_ERROR(TEGRABL_ERR_INVALID, 6);
		goto fail;
	}

	/* Constraint 3: V_FRONT_PORCH + V_SYNC_WIDTH + V_BACK_PORCH > 1
	 * (vertical blank) */
	if (mode->v_front_porch + mode->v_sync_width + mode->v_back_porch <= 1) {
		pr_debug("display timings constraint 3 failed\n");
		err = TEGRABL_ERROR(TEGRABL_ERR_INVALID, 7);
		goto fail;
	}

	/* Constraint 4: V_SYNC_WIDTH >= 1, H_SYNC_WIDTH >= 1 */
	if (mode->v_sync_width < 1 || mode->h_sync_width < 1) {
		pr_debug("display timings constraint 4 failed\n");
		err = TEGRABL_ERROR(TEGRABL_ERR_INVALID, 8);
		goto fail;
	}

	/* Constraint 5: V_REF_TO_SYNC >= 1, H_REF_TO_SYNC >= 0 */
	if (mode->v_ref_to_sync < 1) {
		pr_debug("display timings constraint 5 failed\n");
		err = TEGRABL_ERROR(TEGRABL_ERR_INVALID, 9);
		goto fail;
	}

	/* Constraint 6: V_FRONT_PORT >= (V_REF_TO_SYNC + 1),
	 * H_FRONT_PORT >= (H_REF_TO_SYNC + 1). */
	if (mode->v_front_porch < mode->v_ref_to_sync + 1 ||
		mode->h_front_porch < mode->h_ref_to_sync + 1) {
		pr_debug("display timings constraint 6 failed\n");
		err = TEGRABL_ERROR(TEGRABL_ERR_INVALID, 10);
		goto fail;
	}

	/* Constraint 7: H_DISP_ACTIVE >= 16, V_DISP_ACTIVE >= 16 */
	if (mode->h_active < 16 || mode->v_active < 16) {
		pr_debug("display timings constraint 7 failed\n");
		err = TEGRABL_ERROR(TEGRABL_ERR_INVALID, 11);
		goto fail;
	}

fail:
	return err;
}

static tegrabl_error_t nvdisp_program_mode(struct tegrabl_nvdisp *nvdisp,
										   struct nvdisp_mode *mode)
{
	uint64_t val;
	tegrabl_error_t err = TEGRABL_NO_ERROR;

	pr_debug("%s: entry\n", __func__);
	pr_debug("h_back_porch = %d, v_back_porch = %d\n", mode->h_back_porch,
			 mode->v_back_porch);
	pr_debug("h_front_porch = %d, v_front_porch = %d\n", mode->h_front_porch,
			 mode->v_front_porch);
	pr_debug("h_active = %d, v_active = %d\n", mode->h_active, mode->v_active);
	pr_debug("h_sync_width = %d, v_sync_width = %d\n", mode->h_sync_width,
			 mode->v_sync_width);

	err = nvdisp_check_timing_constraints(mode);
	if (err != TEGRABL_NO_ERROR) {
		goto fail;
	}

	val = NV_DRF_NUM(DC, DISP_SYNC_WIDTH, H_SYNC_WIDTH, mode->h_sync_width) |
		NV_DRF_NUM(DC, DISP_SYNC_WIDTH, V_SYNC_WIDTH, mode->v_sync_width);
	nvdisp_writel(nvdisp, DISP_SYNC_WIDTH, val);

	val = NV_DRF_NUM(DC, DISP_BACK_PORCH, H_BACK_PORCH, mode->h_back_porch) |
		NV_DRF_NUM(DC, DISP_BACK_PORCH, V_BACK_PORCH, mode->v_back_porch);
	nvdisp_writel(nvdisp, DISP_BACK_PORCH, val);

	val = NV_DRF_NUM(DC, DISP_FRONT_PORCH, H_FRONT_PORCH, mode->h_front_porch)
		| NV_DRF_NUM(DC, DISP_FRONT_PORCH, V_FRONT_PORCH, mode->v_front_porch);
	nvdisp_writel(nvdisp, DISP_FRONT_PORCH, val);

	val = NV_DRF_NUM(DC, DISP_DISP_ACTIVE, H_DISP_ACTIVE, mode->h_active) |
			NV_DRF_NUM(DC, DISP_DISP_ACTIVE, V_DISP_ACTIVE, mode->v_active);
	nvdisp_writel(nvdisp, DISP_DISP_ACTIVE, val);

fail:
	pr_debug("%s: exit\n", __func__);
	return err;
}

void nvdisp_clk_disable(struct tegrabl_nvdisp *nvdisp)
{
	/*DUMMY FUNC*/
}

static tegrabl_error_t nvdisp_set_control(struct tegrabl_nvdisp *nvdisp)
{
	uint32_t protocol;
	bool use_sor = false;
	tegrabl_error_t err = TEGRABL_NO_ERROR;

	if (nvdisp->type == DISPLAY_OUT_HDMI) {
		protocol = NV_DRF_DEF(DC, DISP_CORE_SOR_SET_CONTROL, PROTOCOL, SINGLE_TMDS_A);
		use_sor = true;
	} else if (nvdisp->type == DISPLAY_OUT_DP) {
		protocol = NV_DRF_DEF(DC, DISP_CORE_SOR_SET_CONTROL, PROTOCOL, DP_A);
		use_sor = true;
#if defined(IS_T186)
	} else if (nvdisp->type == DISPLAY_OUT_DSI) {
		protocol = NV_DRF_DEF(DC, DISP_CORE_DSI_SET_CONTROL, PROTOCOL, DSI);
		nvdisp_writel(nvdisp, DISP_CORE_DSI_SET_CONTROL, protocol);
#endif
	} else {
		pr_error("%s: unsupported out_type=%d\n", __func__, nvdisp->type);
		err = TEGRABL_ERROR(TEGRABL_ERR_INVALID, 13);
		goto fail;
	}

	if (use_sor) {
		switch (sor_instance) {
		case 0:
			nvdisp_writel(nvdisp, DISP_CORE_SOR_SET_CONTROL, protocol);
			break;
		case 1:
			nvdisp_writel(nvdisp, DISP_CORE_SOR1_SET_CONTROL, protocol);
			break;
#if !defined(IS_T186)
		case 2:
			nvdisp_writel(nvdisp, DISP_CORE_SOR2_SET_CONTROL, protocol);
			break;
		case 3:
			nvdisp_writel(nvdisp, DISP_CORE_SOR3_SET_CONTROL, protocol);
			break;
#endif
		default:
			pr_error("%s: invalid sor_num:%d\n", __func__, sor_instance);
			err = TEGRABL_ERROR(TEGRABL_ERR_INVALID, 14);
			goto fail;
		}
	}

fail:
	return err;
}

static tegrabl_error_t nvdisp_hw_init(struct tegrabl_nvdisp *nvdisp)
{
	uint32_t val;
	struct nvdisp_cmu *cmu;
	tegrabl_error_t err = TEGRABL_NO_ERROR;

	pr_debug("%s: entry\n", __func__);

	val = NV_DRF_DEF(DC, CMD_STATE_ACCESS, READ_MUX, ACTIVE) |
		NV_DRF_DEF(DC, CMD_STATE_ACCESS, WRITE_MUX, ACTIVE);
	nvdisp_writel(nvdisp, CMD_STATE_ACCESS, val);

	val = NV_DRF_NUM(DC, CMD_GENERAL_INCR_SYNCPT_CNTRL,
		GENERAL_INCR_SYNCPT_NO_STALL, 1);
	nvdisp_writel(nvdisp, CMD_GENERAL_INCR_SYNCPT_CNTRL, val);

#if defined(IS_T186)
	val = NV_DRF_NUM(DC, CMD_INT_TYPE, DSC_TO_UF_INT_TYPE, 1) |
		NV_DRF_NUM(DC, CMD_INT_TYPE, DSC_BBUF_UF_INT_TYPE, 1) |
		NV_DRF_NUM(DC, CMD_INT_TYPE, DSC_RBUF_UF_INT_TYPE, 1) |
		NV_DRF_NUM(DC, CMD_INT_TYPE, DSC_OBUF_UF_INT_TYPE, 1);
	nvdisp_writel(nvdisp, CMD_INT_TYPE, val);

	val = NV_DRF_NUM(DC, CMD_INT_TYPE, DSC_TO_UF_INT_TYPE, 1) |
		NV_DRF_NUM(DC, CMD_INT_TYPE, DSC_BBUF_UF_INT_TYPE, 1) |
		NV_DRF_NUM(DC, CMD_INT_TYPE, DSC_RBUF_UF_INT_TYPE, 1) |
		NV_DRF_NUM(DC, CMD_INT_TYPE, DSC_OBUF_UF_INT_TYPE, 1);
	nvdisp_writel(nvdisp, CMD_INT_TYPE, val);
#endif

	val = NV_DRF_NUM(DC, CMD_INT_POLARITY, FRAME_END_INT_POLARITY, 1) |
		NV_DRF_NUM(DC, CMD_INT_POLARITY, V_PULSE3_INT_POLARITY, 1) |
		NV_DRF_NUM(DC, CMD_INT_POLARITY, V_BLANK_INT_POLARITY, 1) |
		NV_DRF_NUM(DC, CMD_INT_POLARITY, V_PULSE2_INT_POLARITY, 1) |
		NV_DRF_NUM(DC, CMD_INT_POLARITY, REGION_CRC_INT_POLARITY, 1) |
		NV_DRF_NUM(DC, CMD_INT_POLARITY, REG_TMOUT_INT_POLARITY, 1) |
		NV_DRF_NUM(DC, CMD_INT_POLARITY, MSF_INT_POLARITY, 1) |
		NV_DRF_NUM(DC, CMD_INT_POLARITY, HEAD_UF_INT_POLARITY, 1) |
		NV_DRF_NUM(DC, CMD_INT_POLARITY, SD3_BUCKET_WALK_DONE_INT_POLARITY, 1);

#if defined(IS_T186)
	val |= NV_DRF_NUM(DC, CMD_INT_POLARITY, DSC_OBUF_UF_INT_POLARITY, 1) |
		NV_DRF_NUM(DC, CMD_INT_POLARITY, DSC_RBUF_UF_INT_POLARITY, 1) |
		NV_DRF_NUM(DC, CMD_INT_POLARITY, DSC_BBUF_UF_INT_POLARITY, 1) |
		NV_DRF_NUM(DC, CMD_INT_POLARITY, DSC_TO_UF_INT_POLARITY, 1);
#endif
	nvdisp_writel(nvdisp, CMD_INT_POLARITY, val);

	/* enable interrupts for vblank, frame_end and underflows */
	val = NV_DRF_DEF(DC, CMD_INT_ENABLE, FRAME_END_INT_ENABLE, ENABLE) |
		NV_DRF_DEF(DC, CMD_INT_ENABLE, SD3_BUCKET_WALK_DONE_INT_ENABLE,
				   ENABLE) |
		NV_DRF_DEF(DC, CMD_INT_ENABLE, V_BLANK_INT_ENABLE, ENABLE) |
		NV_DRF_DEF(DC, CMD_INT_ENABLE, HEAD_UF_INT_ENABLE, ENABLE);

	/* for panels with one-shot mode enable tearing effect interrupt */
	if (nvdisp->flags & NVDISP_OUT_ONE_SHOT_MODE)
		val |= NV_DRF_DEF(DC, CMD_INT_ENABLE, MSF_INT_ENABLE, ENABLE);

	nvdisp_writel(nvdisp, CMD_INT_ENABLE, val);

	val = NV_DRF_DEF(DC, CMD_INT_MASK, HEAD_UF_INT_MASK, NOTMASKED);
	nvdisp_writel(nvdisp, CMD_INT_MASK, val);

	nvdisp_writel(nvdisp, DISP_BLEND_BACKGROUND_COLOR, NO_COLOR);

	cmu = tegrabl_memalign(CMU_ALLIGNMENT_SIZE, sizeof(struct nvdisp_cmu));
	if (!cmu) {
		pr_debug("%s, failed to allocate memory\n", __func__);
		err = TEGRABL_ERROR(TEGRABL_ERR_NO_MEMORY, 0);
		goto fail;
	}

	nvdisp_cmu_init_defaults(cmu);
	nvdisp_set_color_control(nvdisp);
	nvdisp_cmu_set(nvdisp, cmu);

	nvdisp_program_mode(nvdisp, nvdisp->mode);

	err = nvdisp_set_control(nvdisp);
	if (err != TEGRABL_NO_ERROR) {
		goto fail;
	}

	pr_debug("%s: exit\n", __func__);

fail:
	return err;
}

static void nvdisp_set_head_clk(uint32_t module, uint32_t instance, uint32_t clk_src, uint32_t rate)
{
	uint32_t rate_khz;

	tegrabl_car_rst_set(module, instance);
	tegrabl_car_set_clk_src(module, instance, clk_src);
	if (rate != 0) {
		tegrabl_car_set_clk_rate(module, instance, rate, &rate_khz);
	}
	tegrabl_udelay(5);

	tegrabl_car_clk_enable(module, instance, NULL);
	tegrabl_car_rst_clear(module, instance);
	tegrabl_udelay(5);
}

void nvdisp_clk_setup(struct tegrabl_nvdisp *nvdisp)
{
	int32_t i;
	uint32_t rate_khz;
	uint32_t pclk = (nvdisp->mode->pclk / KHZ);
	uint32_t clk_src_rate = pclk;
	static uint32_t max_disp_clk;
	static bool is_initialized;
	static bool is_head_initialized[4];

	pr_debug("%s: entry\n", __func__);

	/*
	 * PLLDx has minimum VCO frequency 800Mhz and the maximum P divider
	 * is 31, thus the lowest possible output frequency of PLLDx is
	 * 806.4/31=26MHz. For low resolution, like 640x480, which PCLK is
	 * lower than 27MHz, requests 2x freq and then use peripheral clock
	 * divider to divide it down.
	 */
	if (clk_src_rate < 27000) {
		pr_debug("%s: clk_src_rate %d KHz < 27000 KHz, set to %d KHz\n", __func__, clk_src_rate,
				 clk_src_rate * 2);
		clk_src_rate *= 2;
	}

	if (nvdisp->parent_clk == TEGRABL_CLK_SRC_PLLD3_OUT0) {
		tegrabl_car_init_pll_with_rate(TEGRABL_CLK_PLL_ID_PLLD3, clk_src_rate, NULL);
	} else if (nvdisp->parent_clk == TEGRABL_CLK_SRC_PLLD2_OUT0) {
		tegrabl_car_init_pll_with_rate(TEGRABL_CLK_PLL_ID_PLLD2, clk_src_rate, NULL);
	} else if (nvdisp->parent_clk == TEGRABL_CLK_SRC_PLLD_OUT1) {
		tegrabl_car_init_pll_with_rate(TEGRABL_CLK_PLL_ID_PLLD, clk_src_rate, NULL);
	} else {
		pr_critical("%s: invalid clock source for nvdisp\n", __func__);
	}

	if (nvdisp->type == DISPLAY_OUT_DP) {
		tegrabl_car_init_pll_with_rate(TEGRABL_CLK_PLL_ID_PLLDP, TEGRABL_DP_CLK, NULL);
	}

	if (!is_initialized) {
		/* Set HOST1X clock */
		tegrabl_car_rst_set(nvdisp->module_host1x, 0);
		tegrabl_car_set_clk_src(nvdisp->module_host1x, 0, TEGRABL_CLK_SRC_PLLP_OUT0);
		tegrabl_udelay(5);
		tegrabl_car_clk_enable(nvdisp->module_host1x, 0, NULL);
		tegrabl_car_rst_clear(nvdisp->module_host1x, 0);
		tegrabl_udelay(5);

		tegrabl_car_rst_set(TEGRABL_MODULE_NVDISPLAY0_HEAD, 0);
	}
	tegrabl_car_rst_set(TEGRABL_MODULE_NVDISPLAY0_HEAD, nvdisp->instance);

	if (!is_initialized) {
		for (i = 0; i < N_WINDOWS; i++) {
			tegrabl_car_rst_set(TEGRABL_MODULE_NVDISPLAY0_WGRP, i);
		}

		tegrabl_car_rst_set(TEGRABL_MODULE_NVDISPLAY0_MISC, 0);
	}

	for (i = 0; i < NVDISP_MAX_HEADS; i++) {
		if (nvdisp->instance == i) {
			nvdisp_set_head_clk(nvdisp->module_nvdisp_p0, i, nvdisp->parent_clk, pclk);
			is_head_initialized[i] = true;
		} else if (is_head_initialized[i] != true) {
			nvdisp_set_head_clk(nvdisp->module_nvdisp_p0, i, TEGRABL_CLK_SRC_CLK_M, 0);
		}
	}

	if (!is_initialized) {
		/* Set NVDISP_HUB clock */
		tegrabl_car_rst_set(nvdisp->module_nvdisp_hub, 0);
		tegrabl_car_set_clk_src(nvdisp->module_nvdisp_hub, 0, TEGRABL_CLK_SRC_PLLDISPHUB);
		tegrabl_udelay(5);
		tegrabl_car_clk_enable(nvdisp->module_nvdisp_hub, 0, NULL);
		tegrabl_car_rst_clear(nvdisp->module_nvdisp_hub, 0);
		tegrabl_udelay(5);
	}

	if (pclk > max_disp_clk) {
		max_disp_clk = pclk;  /*disp clk should always be max of all the head clks*/
	}

	/* Set NVDISP_DISP clock */
	tegrabl_car_rst_set(nvdisp->module_nvdisp, 0);
	tegrabl_car_set_clk_src(nvdisp->module_nvdisp, 0, TEGRABL_CLK_SRC_NVDISPLAY_P0_CLK + nvdisp->instance);
	tegrabl_car_set_clk_rate(nvdisp->module_nvdisp, 0, max_disp_clk, &rate_khz);
	tegrabl_udelay(5);
	tegrabl_car_clk_enable(nvdisp->module_nvdisp, 0, NULL);
	tegrabl_car_rst_clear(nvdisp->module_nvdisp, 0);
	tegrabl_udelay(5);

	if (!is_initialized) {
	#if defined(IS_T186) /*DSC is deprecated in T194*/
		/* Set NVDISP_DSC clock */
		tegrabl_car_rst_set(nvdisp->module_nvdisp_dsc, 0);
		tegrabl_car_set_clk_src(nvdisp->module_nvdisp_dsc, 0, TEGRABL_CLK_SRC_NVDISPLAY_P0_CLK);
		tegrabl_udelay(5);
		tegrabl_car_clk_enable(nvdisp->module_nvdisp_dsc, 0, NULL);
		tegrabl_car_rst_clear(nvdisp->module_nvdisp_dsc, 0);
		tegrabl_udelay(5);
	#endif

		for (i = 0; i < N_WINDOWS; i++) {
			tegrabl_car_rst_clear(TEGRABL_MODULE_NVDISPLAY0_WGRP, i);
		}

		tegrabl_car_rst_clear(TEGRABL_MODULE_NVDISPLAY0_MISC, 0);
		tegrabl_car_rst_clear(TEGRABL_MODULE_NVDISPLAY0_HEAD, 0);
	}
	tegrabl_car_rst_clear(TEGRABL_MODULE_NVDISPLAY0_HEAD, nvdisp->instance);

	is_initialized = true;
}


tegrabl_error_t nvdisp_enable(struct tegrabl_nvdisp *nvdisp)
{
	tegrabl_error_t err = TEGRABL_NO_ERROR;
	uint32_t val;

	if (!nvdisp || !nvdisp->out_ops) {
		err = TEGRABL_ERROR(TEGRABL_ERR_INVALID, 12);
		goto fail;
	}

	nvdisp_clk_setup(nvdisp);

	/* Disable interrupts during initialization */
	nvdisp_writel(nvdisp, CMD_INT_MASK, 0);
	err = nvdisp_hw_init(nvdisp);
	if (err != TEGRABL_NO_ERROR) {
		nvdisp_writel(nvdisp, CMD_INT_MASK, 0);
		nvdisp_clk_disable(nvdisp);
		err = TEGRABL_ERROR(TEGRABL_ERR_INIT_FAILED, 0);
		goto fail;
	}

	if (nvdisp->out_ops && nvdisp->out_ops->enable) {
		err = nvdisp->out_ops->enable(nvdisp);
		if (err != TEGRABL_NO_ERROR) {
			pr_debug("%s: nvdisp enable failed\n", __func__);
			goto fail;
		}
	}

	val = NV_DRF_DEF(DC, CMD_STATE_CONTROL, GENERAL_ACT_REQ, ENABLE) |
			NV_DRF_DEF(DC, CMD_STATE_CONTROL, WIN_A_ACT_REQ, ENABLE);
	nvdisp_writel(nvdisp, CMD_STATE_CONTROL, val);

	val = NV_DRF_DEF(DC, CMD_STATE_CONTROL, GENERAL_UPDATE, ENABLE) |
			NV_DRF_DEF(DC, CMD_STATE_CONTROL, WIN_A_UPDATE, ENABLE);
	nvdisp_writel(nvdisp, CMD_STATE_CONTROL, val);

	nvdisp->flags &= ~NVDISP_OUT_INITIALIZED_MODE;
	pr_debug("%s: EXIT\n", __func__);

fail:
	return err;
}

tegrabl_error_t nvdisp_disable(struct tegrabl_nvdisp *nvdisp)
{
	uint32_t i;
	tegrabl_error_t err = TEGRABL_NO_ERROR;

	nvdisp_writel(nvdisp, CMD_INT_MASK, 0);

	for (i = 0; i < N_WINDOWS; i++) {
		struct nvdisp_win *w = &nvdisp->windows[i];

		/* disable windows */
		w->flags &= ~WIN_FLAG_ENABLED;
	}

	err = tegrabl_car_clk_disable(nvdisp->module_nvdisp, nvdisp->instance);
	if (err != TEGRABL_NO_ERROR) {
		pr_error("Failed to disable nvdisp clk\n");
	}

	return err;
}

struct tegrabl_nvdisp *tegrabl_nvdisp_init(int32_t out_type, struct tegrabl_display_pdata *pdata)
{
	struct tegrabl_nvdisp *nvdisp = NULL;
	tegrabl_error_t err = TEGRABL_NO_ERROR;

	pr_debug("%s: entry\n", __func__);

	nvdisp = tegrabl_malloc(sizeof(struct tegrabl_nvdisp));
	if (!nvdisp) {
		pr_error("%s, memory alloc failed for nvdisp\n", __func__);
		goto fail;
	}

	nvdisp->base_addr = (uintptr_t) nvdisp_base_addr[pdata->nvdisp_instance];
	nvdisp->instance = pdata->nvdisp_instance;
	nvdisp->parent_clk = pdata->disp_clk_src;
	sor_instance = pdata->sor_dtb.sor_instance;

	nvdisp->module_host1x = TEGRABL_MODULE_HOST1X;
	nvdisp->module_nvdisp = TEGRABL_MODULE_NVDISPLAY_DISP;
	nvdisp->module_nvdisp_dsc = TEGRABL_MODULE_NVDISPLAY_DSC;
	nvdisp->module_nvdisp_hub = TEGRABL_MODULE_NVDISPLAYHUB;
	nvdisp->module_nvdisp_p0 = TEGRABL_MODULE_NVDISPLAY_P;
	nvdisp->type = out_type;
	nvdisp->mode = pdata->mode;
	nvdisp->depth = 8;
	nvdisp->dither = 0;
	nvdisp->flags = pdata->flags;

	nvdisp->n_windows = N_WINDOWS;

	err = nvdisp_out_init(nvdisp, pdata);
	if (err != TEGRABL_NO_ERROR) {
		pr_debug("failed to initialize DC out ops\n");
		goto fail;
	}

	err = nvdisp_enable(nvdisp);
	if (err != TEGRABL_NO_ERROR) {
		goto fail;
	} else {
		nvdisp->enabled = 1;
	}

	pr_debug("%s: exit\n", __func__);
	return nvdisp;

fail:
	if (nvdisp) {
		tegrabl_free(nvdisp);
	}

	return NULL;
}

void tegrabl_nvdisp_list_windows(struct tegrabl_nvdisp *nvdisp, uint32_t *count)
{
	TEGRABL_UNUSED(nvdisp);
	nvdisp_win_list(count);
}

tegrabl_error_t tegrabl_nvdisp_configure_window(struct tegrabl_nvdisp *nvdisp,
	uint32_t win_id, struct tegrabl_surface *surf)
{
	tegrabl_error_t err = TEGRABL_NO_ERROR;
	struct nvdisp_win *win = &nvdisp->windows[win_id];
	struct nvdisp_csc *csc = NULL;
	struct nvdisp_cp *cp = NULL;

	pr_debug("%s: entry\n", __func__);

	csc = tegrabl_malloc(sizeof(struct nvdisp_csc));
	if (!csc) {
		pr_debug("memory allocation failed\n");
		return TEGRABL_ERROR(TEGRABL_ERR_NO_MEMORY, 2);
	}
	cp = tegrabl_memalign(CP_ALLIGNMENT_SIZE, sizeof(struct nvdisp_cp));
	if (!cp) {
		pr_debug("memory allocation failed\n");
		tegrabl_free(csc);
		return TEGRABL_ERROR(TEGRABL_ERR_NO_MEMORY, 3);
	}

	win->surf = surf;
	win->pitch = surf->pitch;
	win->x = 0;
	win->y = 0;
	win->w = surf->width;
	win->h = surf->height;

	nvdisp->color_format = surf->pixel_format;

	nvdisp_win_csc_init_defaults(csc);
	nvdisp_win_csc_set(nvdisp, win_id, csc);
	nvdisp_win_cp_init_defaults(cp);
	nvdisp_win_cp_set(nvdisp, win_id, cp);
	nvdisp_win_set_owner(nvdisp, win_id);
	nvdisp_win_config(nvdisp, win_id);

	return err;
}

void tegrabl_nvdisp_win_set_surface(struct tegrabl_nvdisp *nvdisp,
	uint32_t win_id, uintptr_t surf_buf)
{
	pr_debug("%s: entry\n", __func__);

	nvdisp_win_set_buf(nvdisp, win_id, surf_buf);

	pr_debug("%s: exit\n", __func__);
}
