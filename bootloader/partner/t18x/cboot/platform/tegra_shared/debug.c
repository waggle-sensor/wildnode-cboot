/*
 * Copyright (c) 2013-2016, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited
 */

#include <reg.h>
#include <debug.h>
#include <uart.h>
#include <boot_arg.h>
#include <platform_c.h>
#include <printf.h>
#include <tegrabl_timer.h>
#include <tegrabl_debug.h>

#if defined(CONFIG_DEBUG_TIMESTAMP)
#define TIME_STAMP_LENGTH	12
#endif

//static uint32_t is_dcc_debug;

void tegra_debug_init(void)
{
#if 0
	uint32_t size = sizeof(uint64_t);
	uint64_t uart_base_address;


	/* Queries uart base initialized by iram bl */
	boot_arg_query_info(BOOT_ARG_UART_ADDR,
						&uart_base_address, &size);

	tegra_uart_init(uart_base_address);

	/* Print boot_arg according to the flag enabled */
	/* within boot_arg_print_info */
	boot_arg_print_info();

	/* If Uart Base is not provided print on debugger */
	if (!uart_base_address)
		is_dcc_debug = 1;

	dprintf(INFO,"\nDebug Init done\n");
#endif

}

void platform_dputc(char c)
{
#if defined(CONFIG_DEBUG_TIMESTAMP)
	uint8_t i = 0;
	lk_time_t time_lapse = 0;
	char time_stamp[TIME_STAMP_LENGTH];
	static char t = 0;

	/* if last char was new line print time stamp*/
	if (t == '\n') {
		time_lapse = tegrabl_get_timestamp_ms();

		sprintf(time_stamp, "[%04lu.%03lu] ",
				time_lapse / 1000, time_lapse % 1000);

		for (i = 0; i < (TIME_STAMP_LENGTH - 1) ; i++) {
			tegrabl_putc(time_stamp[i]);
		}
	}
	/* cache the printed character for next timestamp */
	t = c;
#endif

	if (c == '\n')
		tegrabl_putc('\r');
	tegrabl_putc(c);
}

int platform_dgetc(char *c, bool wait)
{
	int _c;

	_c = tegrabl_getc();
	if (_c < 0)
		return -1;

	*c = _c;
	return 0;
}

void platform_halt(void)
{
	dprintf(ALWAYS, "HALT: spinning forever...\n");
	for(;;);
}

