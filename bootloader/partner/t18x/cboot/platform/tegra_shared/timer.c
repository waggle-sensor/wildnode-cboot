/*
 * Copyright (c) 2016 - 2017, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited
 */

#include <sys/types.h>
#include <err.h>
#include <reg.h>
#include <platform/timer.h>
#include <platform/interrupts.h>
#include <kernel/thread.h>
#include <common.h>
#include <platform_c.h>
#include <tegrabl_debug.h>
#include <tegrabl_addressmap.h>

#define TEGRA_TIMERUS_BASE		NV_ADDRESS_MAP_TSCUS_BASE
#define TIMERUS_CNTR_1US		0

#define TMR_BT			NV_ADDRESS_MAP_TOP_TKE_BASE
#define TMR_INDEX		1
#define INT_INDEX		1

/* Interrupt number (32 + IRQ index) */
#define INT_NUM			(32 + INT_INDEX)

#define TMR0_OFFSET		0x10000
#define TMR_OFFSET		(TMR0_OFFSET * (TMR_INDEX + 1))

#define TKECR			(16)
#define TKEIE			0x100
#define TKEIV			0x200
#define TKEIR			0x204

#define TMRCR			(TMR_OFFSET+0)
#define PTV				TMRCR
#define TMRSR			(TMR_OFFSET+4)
#define PCR				TMRSR
#define TMRCSSR			(TMR_OFFSET+8)

#define GET_IRQ_DTB		0

struct tegra_timer
{
	uintptr_t reg_base;
	uint32_t intr;
};

/* Need 2 timers. (1) ticker (2) delay */
struct tegra_timer tmrs[2];

static platform_timer_callback timer_callback;
static void *timer_arg;
static lk_time_t timer_interval;

static volatile uint32_t ticks;

static handler_return_t timer_irq(void *arg)
{
	/*  Clears the interrupt */
	writel((1 << 30), tmrs[0].reg_base + PCR);
	ticks += timer_interval;
	return timer_callback(timer_arg, ticks);
}

status_t platform_set_periodic_timer(platform_timer_callback callback, void *arg, lk_time_t interval)
{

	tmrs[0].reg_base = TMR_BT;
	tmrs[0].intr = INT_NUM;
	enter_critical_section();

	timer_callback = callback;
	timer_arg = arg;
	timer_interval = interval;

	/* 1. TKECR => BTKE+16 | Do not disable TSC, us and OSC counters */
	writel(0, tmrs[0].reg_base + TKECR);

	/* 2. TKEIE{i} => BTKE+0x100+4*{i} */
	writel(1 << (TMR_INDEX) , tmrs[0].reg_base + TKEIE + (4 * INT_INDEX));

	/* 3. TMRCSSR{t} => BT+P*{t}+8 */
	/* 00b is the 1 microsecond pulse,
	   01b is OSC (for SPE TKE) or clk_m (all others)
	   10b is any change in bit 0 of the local TSC counter */
	writel(0 , tmrs[0].reg_base + TMRCSSR);

	/* 4. TMRCR{t} => BT+P*{t}+0 */
	writel(((3 << 30)|(timer_interval*1000)) , tmrs[0].reg_base + TMRCR);

	register_int_handler(tmrs[0].intr, timer_irq, 0);
	unmask_interrupt(tmrs[0].intr);

	exit_critical_section();

	return 0;
}

/* returns time in milli seconds */
lk_time_t current_time(void)
{
	return ticks;
}

lk_bigtime_t get_time_stamp_us(void)
{
	return (lk_bigtime_t) readl(TEGRA_TIMERUS_BASE + TIMERUS_CNTR_1US);
}

lk_time_t get_time_stamp_ms(void)
{
	return (get_time_stamp_us() / 1000);
}

void platform_init_timer(void)
{
}

void platform_uninit_timer(void)
{
	mask_interrupt(tmrs[0].intr);
	writel(0, tmrs[0].reg_base + PTV);
}

void udelay(uint32_t usecs)
{
	lk_bigtime_t start = get_time_stamp_us();
	lk_bigtime_t current = 0x0;
	do {
		current = get_time_stamp_us();
		/* Check for rollover */
		if (current < start) {
			/* lk_bigtime_t is typedef as usigned long long */
			current = current + 0x100000000ULL;
		}
	} while((uint32_t)(current - start) < usecs);
}

void mdelay(uint32_t msecs)
{
	udelay(msecs*1000);
}

/* Return current time in micro seconds */
lk_bigtime_t current_time_hires(void)
{
	return ticks * 1000ULL;
}
