/*
 * Copyright (c) 2015-2018, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#define MODULE TEGRABL_ERR_GPCDMA

#include "build_config.h"
#include <inttypes.h>
#include <string.h>
#include <stdbool.h>
#include <tegrabl_addressmap.h>
#include <tegrabl_timer.h>
#include <tegrabl_io.h>
#include <tegrabl_debug.h>
#include <tegrabl_error.h>
#include <tegrabl_gpcdma.h>
#include <tegrabl_clock.h>
#include <tegrabl_dmamap.h>
#include <tegrabl_gpcdma_soc_common.h>
#include <tegrabl_gpcdma_err_aux.h>

#define DMA_CHANNEL_OFFSET				(0x10000U)

/* DMA channel registers */
#define DMA_CH_CSR					(0x0U)
#define DMA_CH_CSR_WEIGHT_SHIFT				(10U)
#define DMA_CH_CSR_XFER_MODE_SHIFT			(21U)
#define DMA_CH_CSR_XFER_MODE_CLEAR_MASK			(7U)
#define DMA_CH_CSR_DMA_MODE_IO2MEM_FC			(1U)
#define DMA_CH_CSR_DMA_MODE_MEM2IO_FC			(3U)
#define DMA_CH_CSR_REQ_SEL_SHIFT			(16U)
#define DMA_CH_CSR_FC_1_MMIO				(1UL << 24)
#define DMA_CH_CSR_FC_4_MMIO				(3UL << 24)
#define DMA_CH_CSR_IRQ_MASK_ENABLE			(1UL << 15)
#define DMA_CH_CSR_RUN_ONCE				(1UL << 27)
#define DMA_CH_CSR_ENABLE				(1UL << 31)

#define DMA_CH_STAT					(0x4U)
#define DMA_CH_STAT_BUSY				(1UL << 31)

#define DMA_CH_SRC_PTR					(0xCU)

#define DMA_CH_DST_PTR					(0x10U)

#define DMA_CH_HI_ADR_PTR				(0x14U)
#define DMA_CH_HI_ADR_PTR_SRC_SHIFT			(0U)
#define DMA_CH_HI_ADR_PTR_SRC_MASK			(0xFFU)
#define DMA_CH_HI_ADR_PTR_DST_SHIFT			(16U)
#define DMA_CH_HI_ADR_PTR_DST_MASK			(0xFFU)

#define DMA_CH_MC_SEQ					(0x18U)
#define DMA_CH_MC_SEQ_REQ_CNT_SHIFT			(25U)
#define DMA_CH_MC_SEQ_REQ_CNT_CLEAR			(0x3FU)
#define DMA_CH_MC_SEQ_REQ_CNT_VAL			(0x10U)
#define DMA_CH_MC_SEQ_BURST_SHIFT			(23U)
#define DMA_CH_MC_SEQ_BURST_MASK			(0x3U)
#define DMA_CH_MC_SEQ_BURST_2_WORDS			(0x0U)
#define DMA_CH_MC_SEQ_BURST_16_WORDS			(0x3U)
#define DMA_CH_MC_SEQ_STREAMID0_1			(0x1U)

#define DMA_CH_MMIO_SEQ					(0x1CU)
#define DMA_CH_MMIO_SEQ_BUS_WIDTH_SHIFT			(28U)
#define DMA_CH_MMIO_SEQ_BUS_WIDTH_CLEAR			(7U)
#define DMA_CH_MMIO_SEQ_MMIO_BURST_SHIFT		(23U)
#define DMA_CH_MMIO_SEQ_MMIO_BURST_CLEAR		(0xFU)
#define DMA_CH_MMIO_SEQ_MMIO_BURST_1_WORD		(0U)
#define DMA_CH_MMIO_SEQ_MMIO_BURST_2_WORD		(1U)
#define DMA_CH_MMIO_SEQ_MMIO_BURST_4_WORD		(3U)
#define DMA_CH_MMIO_SEQ_MMIO_BURST_8_WORD		(7U)
#define DMA_CH_MMIO_SEQ_MMIO_BURST_16_WORD		(15U)

#define DMA_CH_MMIO_WCOUNT				(0x20U)

#define DMA_CH_FIXED_PATTERN				(0x34U)

#define MAX_TRANSFER_SIZE				(1U*1024U*1024U*1024U)	/* 1GB */

struct s_dma_plat_data {
	tegrabl_dmatype_t dma_type;
	uint8_t max_channel_num;
	uintptr_t base_addr;
	tegrabl_module_t dma_module_id;
	bool skip_reset;
};

static struct s_dma_plat_data g_dma_gpc_plat = {
	.dma_type = DMA_GPC,
	.max_channel_num = 32,
	.base_addr = NV_ADDRESS_MAP_GPCDMA_BASE,
	.dma_module_id = TEGRABL_MODULE_GPCDMA,
#if defined(CONFIG_SKIP_GPCDMA_RESET)
	.skip_reset = true,
#else
	.skip_reset = false,
#endif
};
static struct s_dma_plat_data g_dma_bpmp_plat = {
	.dma_type = DMA_BPMP,
	.max_channel_num = 4,
	.base_addr = NV_ADDRESS_MAP_BPMP_DMA_BASE,
	.dma_module_id = TEGRABL_MODULE_BPMPDMA,
	.skip_reset = false,
};
static struct s_dma_plat_data g_dma_spe_plat = {
	.dma_type = DMA_SPE,
	.max_channel_num = 8,
	.base_addr = NV_ADDRESS_MAP_AON_DMA_BASE,
	.dma_module_id = TEGRABL_MODULE_SPEDMA,
	.skip_reset = false,
};

/* Global strcture to maintain DMA information */
struct s_dma_privdata {
	struct s_dma_plat_data dma_plat_data;
	bool init_done;
};

static struct s_dma_privdata g_dma_data[DMA_MAX_NUM];

tegrabl_gpcdma_handle_t tegrabl_dma_request(tegrabl_dmatype_t dma_type)
{
	tegrabl_error_t err = TEGRABL_NO_ERROR;
	tegrabl_gpcdma_handle_t ret;
	bool default_flag = false;

	if (dma_type >= DMA_MAX_NUM) {
		ret = NULL;
		goto done;
	}

	if (g_dma_data[dma_type].init_done == true) {
		ret = (tegrabl_gpcdma_handle_t)(&g_dma_data[dma_type]);
		goto done;
	}

	switch (dma_type) {
	case DMA_GPC:
		g_dma_data[dma_type].dma_plat_data = g_dma_gpc_plat;
		break;
	case DMA_BPMP:
		g_dma_data[dma_type].dma_plat_data = g_dma_bpmp_plat;
		break;
	case DMA_SPE:
		g_dma_data[dma_type].dma_plat_data = g_dma_spe_plat;
		break;
	default:
		ret = NULL;
		default_flag = true;
		break;
	}

	if (default_flag) {
		goto done;
	}

	if (g_dma_data[dma_type].dma_plat_data.skip_reset == false) {
		/* assert reset of DMA engine */
		err = tegrabl_car_rst_set(g_dma_data[dma_type].dma_plat_data.dma_module_id, 0);
		if (err != TEGRABL_NO_ERROR) {
			ret = NULL;
			goto done;
		}

		tegrabl_udelay(2);

		/* de-assert reset of DMA engine */
		err = tegrabl_car_rst_clear(g_dma_data[dma_type].dma_plat_data.dma_module_id, 0);
		if (err != TEGRABL_NO_ERROR) {
			ret = NULL;
			goto done;
		}
	}

	g_dma_data[dma_type].init_done = true;
	ret = (tegrabl_gpcdma_handle_t)(&g_dma_data[dma_type]);

done:
	return ret;
}

static void tegrabl_unmap_buffers(struct tegrabl_dma_xfer_params *params,
								  tegrabl_module_t dma_module_id)
{
	if ((params->dir == DMA_IO_TO_MEM) ||
		(params->dir == DMA_MEM_TO_MEM) ||
		(params->dir == DMA_PATTERN_FILL)) {
		tegrabl_dma_unmap_buffer(dma_module_id, 0, (void *)params->dst,
								 params->size, TEGRABL_DMA_FROM_DEVICE);
		pr_trace("dst unmapped buffer = 0x%x\n", (unsigned int)params->dst);
	}

	if ((params->dir == DMA_MEM_TO_IO) || (params->dir == DMA_MEM_TO_MEM)) {
		tegrabl_dma_unmap_buffer(dma_module_id, 0, (void *)params->src,
								 params->size, TEGRABL_DMA_TO_DEVICE);
		pr_trace("src unmapped buffer = 0x%x\n", (unsigned int)params->src);
	}
}


tegrabl_error_t tegrabl_dma_transfer(tegrabl_gpcdma_handle_t handle,
	uint8_t c_num, struct tegrabl_dma_xfer_params *params)
{
	uintptr_t cb = 0;
	uint32_t val = 0;
	uint32_t val1;
	dma_addr_t src_dma_addr = 0;
	dma_addr_t dst_dma_addr = 0;
	struct s_dma_privdata *dma_data = (struct s_dma_privdata *)handle;
	struct gpcdma_soc_info *ggpcdma_info;
	tegrabl_module_t dma_module_id;
	tegrabl_error_t err;

	if ((handle == NULL) || (params == NULL)) {
		err = TEGRABL_ERROR(TEGRABL_ERR_BAD_PARAMETER, AUX_INFO_DMA_TRANSFER_1);
		TEGRABL_SET_ERROR_STRING(err, "handle: %p, params: %p", handle, params);
		return err;
	}

	if (dma_data->init_done != true) {
		err = TEGRABL_ERROR(TEGRABL_ERR_NOT_INITIALIZED, AUX_INFO_DMA_TRANSFER_1);
		TEGRABL_SET_ERROR_STRING(err, "DMA");
		return err;
	}

	if (c_num >= dma_data->dma_plat_data.max_channel_num) {
		err = TEGRABL_ERROR(TEGRABL_ERR_INVALID, AUX_INFO_DMA_TRANSFER_1);
		TEGRABL_SET_ERROR_STRING(err, "Channel %u", c_num);
		return err;
	}

	/* get the dma module id */
	dma_module_id = dma_data->dma_plat_data.dma_module_id;

	/* get channel base offset */
	cb = dma_data->dma_plat_data.base_addr +
										   (DMA_CHANNEL_OFFSET * (c_num + 1UL));

	/* make sure channel isn't busy */
	val = NV_READ32(cb + DMA_CH_STAT);
	if ((val & DMA_CH_STAT_BUSY) != 0UL) {
		err = TEGRABL_ERROR(TEGRABL_ERR_BUSY, AUX_INFO_DMA_TRANSFER_1);
		TEGRABL_SET_ERROR_STRING(err, "Channel %u", c_num);
		return err;
	}

	gpcdma_get_soc_info(&ggpcdma_info);
	NV_WRITE32_FENCE(cb + DMA_CH_CSR, 0x0);

	/* configure MC sequencer */
	val = NV_READ32(cb + DMA_CH_MC_SEQ);
	val1 = DMA_CH_MC_SEQ_REQ_CNT_CLEAR << DMA_CH_MC_SEQ_REQ_CNT_SHIFT;
	val &= ~val1;
	val |= (DMA_CH_MC_SEQ_REQ_CNT_VAL << DMA_CH_MC_SEQ_REQ_CNT_SHIFT);
	val1 = DMA_CH_MC_SEQ_BURST_MASK << DMA_CH_MC_SEQ_BURST_SHIFT;
	val &= ~val1;

	if ((params->dir == DMA_IO_TO_MEM) || (params->dir == DMA_MEM_TO_IO)) {
		if (ggpcdma_info[0].io_dma_mc_burst_size == 16U) {
			val |= (DMA_CH_MC_SEQ_BURST_16_WORDS << DMA_CH_MC_SEQ_BURST_SHIFT);
		} else if (ggpcdma_info[0].io_dma_mc_burst_size == 2U) {
			val |= (DMA_CH_MC_SEQ_BURST_2_WORDS << DMA_CH_MC_SEQ_BURST_SHIFT);
		}
	} else {
		val |= (DMA_CH_MC_SEQ_BURST_16_WORDS << DMA_CH_MC_SEQ_BURST_SHIFT);
	}
	NV_WRITE32_FENCE(cb + DMA_CH_MC_SEQ, val);

	/* configure MMIO sequencer if either end of DMA transaction is an MMIO */
	if ((params->dir == DMA_IO_TO_MEM) || (params->dir == DMA_MEM_TO_IO)) {
		val = NV_READ32(cb + DMA_CH_MMIO_SEQ);
		val1 = DMA_CH_MMIO_SEQ_BUS_WIDTH_CLEAR <<
												DMA_CH_MMIO_SEQ_BUS_WIDTH_SHIFT;
		val &= ~val1;
		val |= ((uint32_t)params->io_bus_width <<
			DMA_CH_MMIO_SEQ_BUS_WIDTH_SHIFT);
		val1 = DMA_CH_MMIO_SEQ_MMIO_BURST_CLEAR <<
											   DMA_CH_MMIO_SEQ_MMIO_BURST_SHIFT;
		val &= ~val1;
		if (ggpcdma_info[0].io_dma_mmio_burst_size == 16U) {
			val |= (DMA_CH_MMIO_SEQ_MMIO_BURST_16_WORD <<
				DMA_CH_MMIO_SEQ_MMIO_BURST_SHIFT);
		} else if (ggpcdma_info[0].io_dma_mmio_burst_size == 8U) {
			val |= (DMA_CH_MMIO_SEQ_MMIO_BURST_8_WORD <<
				DMA_CH_MMIO_SEQ_MMIO_BURST_SHIFT);
		}
		NV_WRITE32_FENCE(cb + DMA_CH_MMIO_SEQ, val);
	}

	if (params->dir != DMA_PATTERN_FILL) {
		params->pattern = 0;
	}
	NV_WRITE32_FENCE(cb + DMA_CH_FIXED_PATTERN, params->pattern);

	if ((params->dir == DMA_IO_TO_MEM) ||
		(params->dir == DMA_MEM_TO_MEM) ||
		(params->dir == DMA_PATTERN_FILL)) {
		dst_dma_addr = tegrabl_dma_map_buffer(dma_module_id, 0,
				(void *)params->dst, params->size, TEGRABL_DMA_FROM_DEVICE);
		pr_trace("dst mapped buffer = 0x%x\n", (unsigned int)params->dst);
	} else {
		dst_dma_addr = (uintptr_t)(params->dst);
	}

	if ((params->dir == DMA_MEM_TO_IO) || (params->dir == DMA_MEM_TO_MEM)) {
		src_dma_addr = tegrabl_dma_map_buffer(dma_module_id, 0,
				(void *)params->src, params->size, TEGRABL_DMA_TO_DEVICE);
		pr_trace("src unmapped buffer = 0x%x\n", (unsigned int)params->src);
	} else {
		src_dma_addr = (uintptr_t)(params->src);
	}

	/* populate src and dst address registers */
	NV_WRITE32_FENCE(cb + DMA_CH_SRC_PTR, (uint32_t)src_dma_addr);
	NV_WRITE32_FENCE(cb + DMA_CH_DST_PTR, (uint32_t)dst_dma_addr);
	val = (uint32_t)((src_dma_addr >> 32) & DMA_CH_HI_ADR_PTR_SRC_MASK);
	val |= (uint32_t)(((dst_dma_addr >> 32) & DMA_CH_HI_ADR_PTR_DST_MASK) <<
												   DMA_CH_HI_ADR_PTR_DST_SHIFT);
	NV_WRITE32_FENCE(cb + DMA_CH_HI_ADR_PTR, val);

	/* transfer size */
	if ((params->size > MAX_TRANSFER_SIZE) || ((params->size & 0x3UL) != 0UL)) {
		err = TEGRABL_ERROR(TEGRABL_ERR_BAD_PARAMETER, AUX_INFO_DMA_TRANSFER_2);
		TEGRABL_SET_ERROR_STRING(err, "transfer size %u", params->size);
		return err;
	}
	NV_WRITE32_FENCE(cb + DMA_CH_MMIO_WCOUNT, ((params->size >> 2) - 1U));

	/* populate value for CSR */
	val = DMA_CH_CSR_IRQ_MASK_ENABLE |
		  DMA_CH_CSR_RUN_ONCE |
		  (1UL << DMA_CH_CSR_WEIGHT_SHIFT);

	val1 = DMA_CH_CSR_XFER_MODE_CLEAR_MASK << DMA_CH_CSR_XFER_MODE_SHIFT;
	val &= ~val1;
	if (params->dir == DMA_IO_TO_MEM) {
		val |= (DMA_CH_CSR_DMA_MODE_IO2MEM_FC << DMA_CH_CSR_XFER_MODE_SHIFT);
	} else if (params->dir == DMA_MEM_TO_IO) {
		val |= (DMA_CH_CSR_DMA_MODE_MEM2IO_FC << DMA_CH_CSR_XFER_MODE_SHIFT);
	} else {
		val |= ((uint32_t)params->dir << DMA_CH_CSR_XFER_MODE_SHIFT);
	}

	if ((params->dir == DMA_IO_TO_MEM) || (params->dir == DMA_MEM_TO_IO)) {
		val |= (params->io << DMA_CH_CSR_REQ_SEL_SHIFT);
		val |= DMA_CH_CSR_FC_4_MMIO;
	}
	NV_WRITE32_FENCE(cb + DMA_CH_CSR, val);

	val = NV_READ32(cb + DMA_CH_CSR);
	val |= DMA_CH_CSR_ENABLE;
	NV_WRITE32_FENCE(cb + DMA_CH_CSR, val);

	if (params->is_async_xfer == true) {
		return TEGRABL_NO_ERROR;
	} else {
		val = DMA_CH_STAT_BUSY;
		while ((val & DMA_CH_STAT_BUSY) != 0UL) {
			val = NV_READ32(cb + DMA_CH_STAT);
		}
		tegrabl_unmap_buffers(params, dma_module_id);
	}

	return TEGRABL_NO_ERROR;
}

tegrabl_error_t tegrabl_dma_transfer_status(tegrabl_gpcdma_handle_t handle,
	uint8_t c_num, struct tegrabl_dma_xfer_params *params)
{
	uintptr_t cb = 0;
	struct s_dma_privdata *dma_data;
	tegrabl_module_t dma_module_id;
	tegrabl_error_t err;

	if ((handle == NULL) || (params == NULL)) {
		err = TEGRABL_ERROR(TEGRABL_ERR_BAD_PARAMETER, AUX_INFO_DMA_TRANSFER_STATUS);
		TEGRABL_SET_ERROR_STRING(err, "handle: %p, params: %p", handle, params);
		return err;
	}
	dma_data = (struct s_dma_privdata *)handle;

	if (c_num >= dma_data->dma_plat_data.max_channel_num) {
		err = TEGRABL_ERROR(TEGRABL_ERR_INVALID, AUX_INFO_DMA_TRANSFER_STATUS);
		TEGRABL_SET_ERROR_STRING(err, "Channel %u", c_num);
		return err;
	}

	/* get the dma module id */
	dma_module_id = dma_data->dma_plat_data.dma_module_id;

	cb = dma_data->dma_plat_data.base_addr +
										   (DMA_CHANNEL_OFFSET * (c_num + 1UL));
	if ((NV_READ32(cb + DMA_CH_STAT) & DMA_CH_STAT_BUSY) != 0UL) {
		err = TEGRABL_ERROR(TEGRABL_ERR_BUSY, AUX_INFO_DMA_TRANSFER_STATUS);
		pr_debug("DMA channel %u is busy\n", c_num);
		return err;
	} else {
		tegrabl_unmap_buffers(params, dma_module_id);
		return TEGRABL_NO_ERROR;
	}
}

void tegrabl_dma_transfer_abort(tegrabl_gpcdma_handle_t handle,
			uint8_t c_num)
{
	uintptr_t cb = 0;
	struct s_dma_privdata *dma_data;

	if (handle == NULL) {
		return;
	}

	dma_data = (struct s_dma_privdata *)handle;

	cb = dma_data->dma_plat_data.base_addr +
										   (DMA_CHANNEL_OFFSET * (c_num + 1UL));
	NV_WRITE32_FENCE(cb + DMA_CH_CSR, 0);
}

/* General pupose DMA will be used for utility APIs */
int tegrabl_dma_memcpy(void *priv, void *dest, const void *src, size_t size)
{
	tegrabl_gpcdma_handle_t handle;
	struct tegrabl_dma_xfer_params params;
	tegrabl_dmatype_t dma_type;
	int ret;

	dma_type = *((tegrabl_dmatype_t *)priv);
	/* Check for word-aligned bufs and word-multiple length */
	if ((dest == NULL) || (src == NULL) || ((size & 0x3U) != 0U) ||
		((((uintptr_t)dest) & (0x3UL)) != 0UL) || ((((uintptr_t)src) & (0x3UL)) != 0UL)) {
		return -1;
	}
	pr_trace("%s(%p,%p,%u)\n", __func__, dest, src, (uint32_t)size);

	handle = tegrabl_dma_request(dma_type);

	params.src = (uintptr_t)src;
	params.dst = (uintptr_t)dest;
	params.size = size;
	params.is_async_xfer = false;
	params.dir = DMA_MEM_TO_MEM;

	if (tegrabl_dma_transfer(handle, 0, &params) != TEGRABL_NO_ERROR) {
		ret = 1;
	} else {
		ret = 0;
	}

	return ret;
}

/*
 * As the below SDRAM Init scrub API is using the GPCDMA for accessing the
 * SDRAM directy, data is not cached so no need to invoke DMA-mapping apis
 */
tegrabl_error_t tegrabl_init_scrub_dma(uint64_t dest, uint64_t src,
									   uint32_t pattern, uint32_t size,
									   tegrabl_dmatransferdir_t data_dir)
{
	tegrabl_gpcdma_handle_t handle;
	struct tegrabl_dma_xfer_params params;
	uintptr_t cb = 0;
	uint32_t val = 0;
	uint32_t val1;
	uint32_t c_num = 0;
	dma_addr_t src_dma_addr = 0;
	dma_addr_t dst_dma_addr = 0;
	struct s_dma_privdata *dma_data;
	tegrabl_error_t result = TEGRABL_NO_ERROR;

	handle = tegrabl_dma_request(DMA_GPC);

	params.size = size;
	params.is_async_xfer = false;
	params.pattern = pattern;
	params.dir = data_dir;

	dma_data = (struct s_dma_privdata *)handle;

	/* get channel base offset */
	cb = dma_data->dma_plat_data.base_addr +
											(DMA_CHANNEL_OFFSET * (c_num + 1U));

	/* make sure channel isn't busy */
	val = NV_READ32(cb + DMA_CH_STAT);
	if ((val & DMA_CH_STAT_BUSY) != 0UL) {
		result = TEGRABL_ERROR(TEGRABL_ERR_BUSY, AUX_INFO_INIT_SCRUB_DMA_1);
		TEGRABL_SET_ERROR_STRING(result, "Channel %u", c_num);
		goto fail;
	}

	val = NV_READ32(cb + DMA_CH_MC_SEQ);
	val1 = DMA_CH_MC_SEQ_REQ_CNT_CLEAR << DMA_CH_MC_SEQ_REQ_CNT_SHIFT;
	val &= ~val1;
	val |= (DMA_CH_MC_SEQ_REQ_CNT_VAL << DMA_CH_MC_SEQ_REQ_CNT_SHIFT);
	val1 = DMA_CH_MC_SEQ_BURST_MASK << DMA_CH_MC_SEQ_BURST_SHIFT;
	val &= ~val1;
	val |= (DMA_CH_MC_SEQ_BURST_16_WORDS << DMA_CH_MC_SEQ_BURST_SHIFT);
	NV_WRITE32_FENCE(cb + DMA_CH_MC_SEQ, val);

	NV_WRITE32_FENCE(cb + DMA_CH_FIXED_PATTERN, params.pattern);

	src_dma_addr = src;
	dst_dma_addr = dest;

	pr_trace("dst_dma_addr = 0x%016"PRIx64"\n", dst_dma_addr);
	pr_trace("src_dma_addr = 0x%016"PRIx64"\n", src_dma_addr);
	pr_trace("size = 0x%08x\n", params.size);

	/* populate src and dst address registers */
	NV_WRITE32_FENCE(cb + DMA_CH_SRC_PTR, (uint32_t)src_dma_addr);
	NV_WRITE32_FENCE(cb + DMA_CH_DST_PTR, (uint32_t)dst_dma_addr);
	val = (uint32_t)(src_dma_addr >> 32) & DMA_CH_HI_ADR_PTR_SRC_MASK;
	val |= (uint32_t)(((dst_dma_addr >> 32) & DMA_CH_HI_ADR_PTR_DST_MASK) <<
												   DMA_CH_HI_ADR_PTR_DST_SHIFT);
	NV_WRITE32_FENCE(cb + DMA_CH_HI_ADR_PTR, val);

	/* transfer size */
	if ((params.size > MAX_TRANSFER_SIZE) || ((params.size & 0x3U) != 0U)) {
		result = TEGRABL_ERROR(TEGRABL_ERR_BAD_PARAMETER, AUX_INFO_INIT_SCRUB_DMA_1);
		TEGRABL_SET_ERROR_STRING(result, "transfer size %u", params.size);
		goto fail;
	}
	NV_WRITE32_FENCE(cb + DMA_CH_MMIO_WCOUNT, ((params.size >> 2) - 1U));

	/* populate value for CSR */
	if ((params.dir != DMA_PATTERN_FILL) &&
			(params.dir != DMA_MEM_TO_MEM)) {
		result = TEGRABL_ERROR(TEGRABL_ERR_BAD_PARAMETER, AUX_INFO_INIT_SCRUB_DMA_2);
		TEGRABL_SET_ERROR_STRING(result, "transfer type %u", params.dir);
		goto fail;
	}
	val = NV_READ32(cb + DMA_CH_CSR);
	val1 = DMA_CH_CSR_XFER_MODE_CLEAR_MASK << DMA_CH_CSR_XFER_MODE_SHIFT;
	val &= ~val1;
	val |= (params.dir << DMA_CH_CSR_XFER_MODE_SHIFT);
	NV_WRITE32_FENCE(cb + DMA_CH_CSR, val);

	val = NV_READ32(cb + DMA_CH_CSR);
	val |= DMA_CH_CSR_ENABLE;
	NV_WRITE32_FENCE(cb + DMA_CH_CSR, val);

	pr_trace("Waiting for DMA ......%s\n", __func__);

	while ((NV_READ32(cb + DMA_CH_STAT) & DMA_CH_STAT_BUSY) != 0UL) {
		;
	}
	pr_trace("DMA Complete......%s\n\n", __func__);

fail:
	return result;
}

int tegrabl_dma_memset(void *priv, void *s, uint32_t c, size_t size)
{
	tegrabl_gpcdma_handle_t handle;
	struct tegrabl_dma_xfer_params params;
	tegrabl_dmatype_t dma_type;
	int ret;

	dma_type = *((tegrabl_dmatype_t *)priv);
	/* Check for word-aligned buf and word-multiple size */
	if ((s == NULL) || ((size & 0x3U) != 0U) || ((((uintptr_t)s) & 0x3U) != 0U)) {
		return -1;
	}
	handle = tegrabl_dma_request(dma_type);

	pr_trace("%s(%p,%u,%u)\n", __func__, s, c, (uint32_t)size);

	params.src = 0;
	params.dst = (uintptr_t)s;
	params.size = size;
	params.is_async_xfer = false;
	c &= 0xffU;
	c |= c << 8;
	c |= c << 16;
	params.pattern = c;
	params.dir = DMA_PATTERN_FILL;

	if (tegrabl_dma_transfer(handle, 0, &params) != TEGRABL_NO_ERROR) {
		ret = 1;
	} else {
		ret = 0;
	}

	return ret;
}

static struct tegrabl_clib_dma clib_dma;

void tegrabl_dma_enable_clib_callbacks(tegrabl_dmatype_t dma_type,
									   size_t threshold)
{
	/* The gpcdma memcpy/memset APIs expect a pointer to dma_type.
	 * Cannot pass reference to local variables on stack for this as
	 * they would get dereferenced after the scope has finished */
	static tegrabl_dmatype_t s_dma_type;
	s_dma_type = dma_type;

	clib_dma.memcpy_priv = (void *)&s_dma_type;
	clib_dma.memset_priv = (void *)&s_dma_type;
	clib_dma.memcpy_threshold = threshold;
	clib_dma.memset_threshold = threshold;
	clib_dma.memcpy_callback = tegrabl_dma_memcpy;
	clib_dma.memset_callback = tegrabl_dma_memset;

	tegrabl_clib_dma_register(&clib_dma);
}
