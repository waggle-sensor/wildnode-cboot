/*
 * Copyright (c) 2018-2019, NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include "lwip/apps/tftp_client.h"
#include "lwip/ip_addr.h"
#include <string.h>

#if LWIP_UDP

#include "lwip/udp.h"

#define TFTP_MAX_PAYLOAD_SIZE_BYTES    512U
#define TFTP_HEADER_LENGTH             4U
#define TFTP_SERVER_PORT               69U
#define TFTP_CLIENT_PORT               50033U
#define TFTP_ACK_RESEND_TIMEOUT        5000U
#define TFTP_MAX_ACK_RETRIES           5U

#define TFTP_READ                      1U
#define TFTP_WRITE                     2U
#define TFTP_DATA                      3U
#define TFTP_ACK                       4U
#define TFTP_ERROR                     5U

#define TFTP_CLIENT_DEBUG              0U
#define PROGRESS_BAR                   1U

#define PROGRESS_BAR_INTERVAL_BLKS     250U
#define PROGRESS_BAR_INTERVAL_BYTES    (PROGRESS_BAR_INTERVAL_BLKS * TFTP_MAX_PAYLOAD_SIZE_BYTES)
#define MIN_CONSOLE_ROW_SIZE           80U

#define PBUF_TAKE_ERR_MSG(str)         "Failed to copy " "" str "" " to pbuf buffer"

struct tftp_client_priv {
    struct udp_pcb *pcb;
    ip_addr_t tftp_server_ip;
    void *dst_mem_addr;
    u32_t dst_size;
    u16_t last_rcvd_blk;
    u16_t exptd_blk;
    u32_t tot_data_cnt_bytes;
    u16_t temp_conn_port;
    time_t last_ack_time_ms;
    bool is_file_rcvd;
    err_t err;
};

struct tftp_client_priv tftp_client = {0};
static char *prefix_str = "TFTP Client:";
static u32_t bar_cnt;

#if TFTP_CLIENT_DEBUG
static u32_t transfer_start_ms;
static u32_t transfer_end_ms;
static u32_t speed_kibi_byte_per_sec;
#endif

static int
send_ack(u16_t blk_num, u16_t port)
{
    struct pbuf *p = NULL;
    u16_t *payload;
    err_t ret = ERR_OK;

    p = pbuf_alloc(PBUF_TRANSPORT, TFTP_HEADER_LENGTH, PBUF_RAM);
    if (p == NULL) {
        ret = ERR_MEM;
        LWIP_DEBUGF(TFTP_DEBUG | LWIP_DBG_STATE, ("%s Failed to allocate memory\n", prefix_str));
        goto fail;
    }
    payload = (u16_t *)p->payload;
    payload[0] = lwip_htons(TFTP_ACK);
    payload[1] = lwip_htons(blk_num);

    ret = udp_sendto(tftp_client.pcb, p, &tftp_client.tftp_server_ip, port);
    if (ret != ERR_OK) {
        LWIP_DEBUGF(TFTP_DEBUG | LWIP_DBG_STATE,
                    ("%s Failed to send ACK: err: %d\n", prefix_str, (signed)ret));
        goto fail;
    }

    tftp_client.last_ack_time_ms = tegrabl_get_timestamp_ms();

#if TFTP_CLIENT_DEBUG && !PROGRESS_BAR
    LWIP_DEBUGF(TFTP_DEBUG | LWIP_DBG_STATE, ("Sent ACK: %u\n", blk_num));
#endif

fail:
    if (p != NULL) {
        pbuf_free(p);
    }
    return ret;
}

static void
recv(void *a, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
    u16_t *sbuf = NULL;
    u8_t *data_ptr = NULL;
    u8_t *ram_addr = NULL;
    u16_t opcode = 0;
    u16_t data_len_bytes = 0;
    u16_t blk_num = 0;
    err_t ret = ERR_OK;
#if PROGRESS_BAR
    bool old_setting;
#endif

    char *err_msg[8] = {
        [0] = "Not defined",
        [1] = "File not found",
        [2] = "Access Violation",
        [3] = "Disk full or quota exceeded",
        [4] = "Illegal operation",
        [5] = "Unknown port number",
        [6] = "File already exists",
        [7] = "No such user"
    };

    sbuf = (u16_t *)p->payload;
    opcode = lwip_ntohs(sbuf[0]);

    switch (opcode) {

    case TFTP_DATA:
        if ((u16_t)p->len == 0U) {
            goto fail;
        }

        blk_num = lwip_ntohs(sbuf[1]);
        if (blk_num != tftp_client.exptd_blk) {
#if TFTP_CLIENT_DEBUG && !PROGRESS_BAR
            LWIP_DEBUGF(TFTP_DEBUG | LWIP_DBG_STATE,
                        ("Rcvd blk no: %u  !=  expected blk no: %u\n", blk_num, tftp_client.exptd_blk));
#endif
            goto fail;
        }

#if TFTP_CLIENT_DEBUG && !PROGRESS_BAR
        LWIP_DEBUGF(TFTP_DEBUG | LWIP_DBG_STATE, ("blk no: %u\n", blk_num));
#endif

        tftp_client.exptd_blk++;
        tftp_client.last_rcvd_blk = blk_num;

        /* Copy data to RAM */
        data_len_bytes = (u16_t)p->len - TFTP_HEADER_LENGTH;
        ram_addr = (u8_t *)tftp_client.dst_mem_addr + tftp_client.tot_data_cnt_bytes;
        data_ptr = (u8_t *)p->payload + TFTP_HEADER_LENGTH;
        memcpy((void *)ram_addr, (void *)data_ptr, data_len_bytes);

        tftp_client.tot_data_cnt_bytes = tftp_client.tot_data_cnt_bytes + data_len_bytes;
        if (tftp_client.tot_data_cnt_bytes > tftp_client.dst_size) {
            LWIP_DEBUGF(TFTP_DEBUG | LWIP_DBG_STATE,
                        ("%s Destination size is smaller than the rcvd file size\n", prefix_str));
            goto fail;
        }

        if (data_len_bytes < TFTP_MAX_PAYLOAD_SIZE_BYTES) {
            tftp_client.is_file_rcvd = true;
        }

#if TFTP_CLIENT_DEBUG && !PROGRESS_BAR
        LWIP_DEBUGF(TFTP_DEBUG | LWIP_DBG_STATE,
                    ("Total data rcvd: %u bytes, Curr pkt data len: %u bytes\n",
                        tftp_client.tot_data_cnt_bytes, data_len_bytes));
#endif

#if PROGRESS_BAR
        old_setting = tegrabl_enable_timestamp(false);
        if ((tftp_client.tot_data_cnt_bytes % PROGRESS_BAR_INTERVAL_BYTES) == 0) {
            tegrabl_printf("#");
            bar_cnt++;
            /* Enter a newline if bar crosses the minimum row size */
            if ((bar_cnt % MIN_CONSOLE_ROW_SIZE) == 0) {
                tegrabl_printf("\n");
            }
        }
        if (tftp_client.is_file_rcvd) {
            tegrabl_printf("\n");
        }
        (void)tegrabl_enable_timestamp(old_setting);
#endif

        if (tftp_client.is_file_rcvd) {
            LWIP_DEBUGF(TFTP_DEBUG | LWIP_DBG_STATE, ("%s Last packet received\n", prefix_str));
        }

#if TFTP_CLIENT_DEBUG
        if (tftp_client.is_file_rcvd) {
            transfer_end_ms = tegrabl_get_timestamp_ms() - transfer_start_ms;
            speed_kibi_byte_per_sec = ((tftp_client.tot_data_cnt_bytes/transfer_end_ms) * 1000) / 1024;
            LWIP_DEBUGF(TFTP_DEBUG | LWIP_DBG_STATE,
                        ("Total data: %u bytes, transfer rate: %u KiB/s\n",
                            tftp_client.tot_data_cnt_bytes, speed_kibi_byte_per_sec));
        }
#endif

        /* Send acknowledgement for successful packet reception */
        ret = send_ack(blk_num, port);
        if (ret != ERR_OK) {
            goto fail;
        }
        tftp_client.temp_conn_port = port;

        break;

    case TFTP_ERROR:
        LWIP_DEBUGF(TFTP_DEBUG | LWIP_DBG_STATE,
                    ("%s Error received: code: %u, msg: %s\n",
                        prefix_str, lwip_ntohs(sbuf[1]), err_msg[lwip_ntohs(sbuf[1])]));
        tftp_client.err = ERR_ARG;
        break;

    default:
        LWIP_DEBUGF(TFTP_DEBUG | LWIP_DBG_STATE, ("%s Invalid opcode: %u\n", prefix_str, opcode));
        tftp_client.err = ERR_ARG;
        break;
    }

fail:
    return;
}

err_t
tftp_client_init(const u8_t * const tftp_server_ip)
{
    err_t ret = ERR_OK;

    if (tftp_server_ip == NULL) {
        ret = ERR_ARG;
        LWIP_DEBUGF(TFTP_DEBUG | LWIP_DBG_STATE, ("%s Invalid TFTP server IP addr passed\n", prefix_str));
        goto done;
    }

    LWIP_DEBUGF(TFTP_DEBUG | LWIP_DBG_STATE, ("%s Init\n", prefix_str));

    tftp_client.pcb = udp_new_ip_type(IPADDR_TYPE_ANY);
    if (tftp_client.pcb == NULL) {
        ret = ERR_MEM;
        LWIP_DEBUGF(TFTP_DEBUG | LWIP_DBG_STATE, ("%s Failed to allocate UDP PCB\n", prefix_str));
        goto done;
    }

    ret = udp_bind(tftp_client.pcb, IP_ANY_TYPE, TFTP_CLIENT_PORT);
    if (ret != ERR_OK) {
        LWIP_DEBUGF(TFTP_DEBUG | LWIP_DBG_STATE, ("%s Failed to bind port to UDP PCB\n", prefix_str));
        goto cleanup;
    }

    tftp_client.tftp_server_ip.addr = (tftp_server_ip[0] << 0U)  |
                                      (tftp_server_ip[1] << 8U)  |
                                      (tftp_server_ip[2] << 16U) |
                                      (tftp_server_ip[3] << 24U);
    LWIP_DEBUGF(TFTP_DEBUG | LWIP_DBG_STATE,
                ("%s Server IP: %u.%u.%u.%u\n",
                    prefix_str,
                    (tftp_client.tftp_server_ip.addr >>  0U) & 0xFF,
                    (tftp_client.tftp_server_ip.addr >>  8U) & 0xFF,
                    (tftp_client.tftp_server_ip.addr >> 16U) & 0xFF,
                    (tftp_client.tftp_server_ip.addr >> 24U) & 0xFF));

    udp_recv(tftp_client.pcb, recv, NULL);

    goto done;

cleanup:
    if (tftp_client.pcb == NULL) {
        udp_remove(tftp_client.pcb);
    }

done:
    return ret;
}

err_t
tftp_client_recv(char * const filename,
				 char * const filetype,
				 void * const dst_addr,
				 u32_t dst_size,
				 u32_t * const file_size)
{
    struct pbuf *p = NULL;
    u16_t opcode;
    u8_t offset = 0;
    u8_t null_byte = 0;
    u8_t write_len = 0;
    u8_t ack_retries;
    u32_t last_ack_retry_blk;
    time_t curr_time_ms;
    err_t ret = ERR_OK;

    if ((filename == NULL) || (filetype == NULL) || (dst_addr == NULL)) {
        ret = ERR_ARG;
        LWIP_DEBUGF(TFTP_DEBUG | LWIP_DBG_STATE, ("%s Invalid args\n", prefix_str));
        goto fail;
    }

    LWIP_DEBUGF(TFTP_DEBUG | LWIP_DBG_STATE, ("%s Send RRQ, file: %s\n", prefix_str, filename));

    tftp_client.dst_mem_addr = dst_addr;
    tftp_client.dst_size = dst_size;
    tftp_client.is_file_rcvd = false;
    tftp_client.last_rcvd_blk = 0;
    tftp_client.exptd_blk = 1;
    tftp_client.tot_data_cnt_bytes = 0;
    tftp_client.last_ack_time_ms = tegrabl_get_timestamp_ms();
    tftp_client.err = ERR_OK;
    bar_cnt = 0;

    p = pbuf_alloc(PBUF_TRANSPORT, (u16_t)(TFTP_HEADER_LENGTH + strlen(filename) + strlen(filetype)),
                   PBUF_RAM);
    if (p == NULL) {
        ret = ERR_MEM;
        LWIP_DEBUGF(TFTP_DEBUG | LWIP_DBG_STATE, ("%s Failed to allocate buffer\n", prefix_str));
        goto fail;
    }

    /* Create RRQ packet */
    opcode = lwip_ntohs(TFTP_READ);
    write_len = sizeof(opcode);
    ret = pbuf_take(p, &opcode, write_len);
    if (ret != ERR_OK) {
        LWIP_DEBUGF(TFTP_DEBUG | LWIP_DBG_STATE, ("%s %s\n", prefix_str, PBUF_TAKE_ERR_MSG("opcode")));
        goto fail;
    }
    offset = write_len;
    write_len = strlen(filename);
    ret = pbuf_take_at(p, filename, write_len, offset);
    if (ret != ERR_OK) {
        LWIP_DEBUGF(TFTP_DEBUG | LWIP_DBG_STATE, ("%s %s\n", prefix_str, PBUF_TAKE_ERR_MSG("filename")));
        goto fail;
    }
    offset = offset + write_len;
    write_len = sizeof(null_byte);
    ret = pbuf_take_at(p, &null_byte, write_len, offset);
    if (ret != ERR_OK) {
        LWIP_DEBUGF(TFTP_DEBUG | LWIP_DBG_STATE, ("%s %s\n", prefix_str, PBUF_TAKE_ERR_MSG("null byte")));
        goto fail;
    }
    offset = offset + write_len;
    write_len = strlen(filetype);
    ret = pbuf_take_at(p, filetype, write_len, offset);
    if (ret != ERR_OK) {
        LWIP_DEBUGF(TFTP_DEBUG | LWIP_DBG_STATE, ("%s %s\n", prefix_str, PBUF_TAKE_ERR_MSG("filetype")));
        goto fail;
    }
    offset = offset + write_len;
    write_len = sizeof(null_byte);
    ret = pbuf_take_at(p, &null_byte, write_len, offset);
    if (ret != ERR_OK) {
        LWIP_DEBUGF(TFTP_DEBUG | LWIP_DBG_STATE, ("%s %s\n", prefix_str, PBUF_TAKE_ERR_MSG("null byte")));
        goto fail;
    }

    /* Send RRQ packet to destination */
    ret = udp_sendto(tftp_client.pcb, p, &tftp_client.tftp_server_ip, TFTP_SERVER_PORT);
    if (ret != ERR_OK) {
        udp_remove(tftp_client.pcb);
        goto fail;
    }

    /* Wait till full file is received */
#if TFTP_CLIENT_DEBUG
    transfer_start_ms = tegrabl_get_timestamp_ms();
#endif
    ack_retries = 0;
    last_ack_retry_blk = 0;
    while ((tftp_client.is_file_rcvd == false) &&
           (tftp_client.err == ERR_OK)         &&
           (ack_retries < TFTP_MAX_ACK_RETRIES)) {

        curr_time_ms = tegrabl_get_timestamp_ms();

        if (curr_time_ms > (tftp_client.last_ack_time_ms + TFTP_ACK_RESEND_TIMEOUT)) {

            /* Exit if haven't received a single packet */
            if (tftp_client.last_rcvd_blk == 0) {
                ret = ERR_CONN;
                LWIP_DEBUGF(TFTP_DEBUG | LWIP_DBG_STATE, ("%s Connection failed\n", prefix_str));
                goto fail;
            }

#if TFTP_CLIENT_DEBUG && !PROGRESS_BAR
            LWIP_DEBUGF(TFTP_DEBUG | LWIP_DBG_STATE, ("Resend ACK: %u\n", tftp_client.last_rcvd_blk));
#endif
            ret = send_ack(tftp_client.last_rcvd_blk, tftp_client.temp_conn_port);
            if (ret != ERR_OK) {
                goto fail;
            }

            if (last_ack_retry_blk == tftp_client.last_rcvd_blk) {
                ack_retries++;
            } else {
                ack_retries = 0;
            }

            last_ack_retry_blk = tftp_client.last_rcvd_blk;
            tftp_client.exptd_blk = tftp_client.last_rcvd_blk + 1;
        }
        tegrabl_udelay(100);
    }

    if (ack_retries >= TFTP_MAX_ACK_RETRIES) {
        ret = ERR_TIMEOUT;
        goto fail;
    }
    if (tftp_client.err != ERR_OK) {
        ret = tftp_client.err;
    }

	if (file_size != NULL) {
		*file_size = tftp_client.tot_data_cnt_bytes;
	}

fail:
    if (p != NULL) {
        pbuf_free(p);
    }
    return ret;
}

void
tftp_client_deinit(void)
{
    if (tftp_client.pcb != NULL) {
        udp_remove(tftp_client.pcb);
        tftp_client.pcb = NULL;
    }
}

#endif /* LWIP_UDP */
