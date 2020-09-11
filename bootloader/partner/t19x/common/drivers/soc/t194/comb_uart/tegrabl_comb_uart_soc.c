/*
 * Copyright (c) 2017-2018, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#include <tegrabl_comb_uart.h>
#include <tegrabl_comb_uart_soc_local.h>
#include <tegrabl_addressmap.h>
#include <tegrabl_ar_macro.h>
#include <arhsp_shrd_mbox.h>

static struct comb_uart_mailbox mailbox[] = {
	[COMB_UART_BPMP_MAILBOX] = {
		.tx_addr = NV_ADDRESS_MAP_AON_HSP_SM_6_7_BASE + HSP_SHRD_MBOX_MBOX_0_SHRD_MBOX_0,
		.rx_addr = NV_ADDRESS_MAP_BPMP_HSP_SM_0_1_BASE + HSP_SHRD_MBOX_MBOX_1_SHRD_MBOX_0
	},
	[COMB_UART_CPU_MAILBOX] = {
		.tx_addr = NV_ADDRESS_MAP_AON_HSP_SM_0_1_BASE + HSP_SHRD_MBOX_MBOX_1_SHRD_MBOX_0,
		.rx_addr = NV_ADDRESS_MAP_TOP0_HSP_SM_0_1_BASE + HSP_SHRD_MBOX_MBOX_0_SHRD_MBOX_0
	},
};

struct comb_uart_mailbox comb_uart_get_mailbox_info(uint32_t mailbox_type)
{
	return mailbox[mailbox_type];
}
