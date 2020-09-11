/*
 * Copyright (c) 2018-2019, NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#ifndef LWIP_HDR_APPS_TFTP_CLIENT_H
#define LWIP_HDR_APPS_TFTP_CLIENT_H

#include "lwip/apps/tftp_opts.h"
#include "lwip/err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize TFTP client
 * @param tftp_server_ip TFTP server IP address
 * @returns error
 */
err_t tftp_client_init(const u8_t * const tftp_server_ip);

/**
 * Request file transfer from TFTP server
 * @param filename file to be read
 * @param file_type type of file, ascii or binary
 * @param dst_addr memory address where received file is to be copied
 * @param dst_size size of the destination memory
 * @param file_size size of the received file
 * @returns error
 */
err_t tftp_client_recv(char * const filename,
					   char * const file_type,
					   void * const dst_addr,
					   u32_t dst_size,
					   u32_t * const file_size);

/**
 * De-initialize TFTP client
 */
void tftp_client_deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* LWIP_HDR_APPS_TFTP_CLIENT_H */
