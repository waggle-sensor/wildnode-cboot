/*
 * Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited
 */

#include <debug.h>
#include <err.h>
#include <assert.h>
#include <arch.h>
#include <arch/ops.h>
#include <arch/arm64.h>
#include <sys/types.h>
#include <platform.h>
#include <platform/interrupts.h>
#include <platform/iomap.h>
#include <kernel/thread.h>
#include <kernel/debug.h>
#include <common.h>
#include <tegrabl_addressmap.h>

#define COMPATIBLE_VGIC2 "arm,cortex-a15-gic"

//GIC Block offset
#define DISTRIBUTOR		0x1000
#define CPU_INTERFACE	0x2000

// GICD Offsets
#define GICD_CTLR		0x000
#define GICD_TYPER		0x004
#define GICD_ISENABLE	0x100
#define GICD_ICENABLE	0x180
#define GICD_ISACTIVE	0x300
#define GICD_ITARGETS	0x800
#define GICD_ICFG		0xC00

// GICC Offsets
#define GICC_CTLR		0x000
#define GICC_PMR		0x004
#define GICC_IAR		0x00C
#define GICC_EOIR		0x010

#define GICD_READ(x)	readl(gicd_base + x)
#define GICD_WRITE(x,y)	writel(y, gicd_base + x)

#define GICC_READ(x)	readl(gicc_base + x)
#define GICC_WRITE(x,y)	writel(y, gicc_base + x)

#define TEGRA_MAX_INT	256

#define T186_GICC_BASE	((T186_GICD_BASE) + 0x1000)

static uintptr_t gicd_base;
static uintptr_t gicc_base;

struct ihandler {
    int_handler func;
    void *arg;
};

static struct ihandler handler[TEGRA_MAX_INT];

handler_return_t platform_irq(struct arm64_iframe_short *frame)
{
    handler_return_t ret = INT_NO_RESCHEDULE;
    uint32_t iar;
    uint32_t irq_num;
    iar = GICC_READ(GICC_IAR);
    irq_num = iar & 0x3FF;

    if (irq_num >= TEGRA_MAX_INT) {
        // spurious
        return INT_NO_RESCHEDULE;
    }

    inc_critical_section();

    THREAD_STATS_INC(interrupts);
    KEVLOG_IRQ_ENTER(irq_num);

    if (handler[irq_num].func)
        ret = handler[irq_num].func(handler[irq_num].arg);

    GICC_WRITE(GICC_EOIR,(iar & 0x1FFF));

    KEVLOG_IRQ_EXIT(irq_num);

    if (ret != INT_NO_RESCHEDULE)
        thread_preempt();

    dec_critical_section();

    return ret;
}

void platform_fiq(struct arm64_iframe_short *frame)
{
    PANIC_UNIMPLEMENTED;
}

void platform_init_interrupts(void)
{
    uint32_t spi_lines;
    uint32_t cpu_num;
    uint32_t i;

	gicd_base = T186_GICD_BASE;
	gicc_base = T186_GICC_BASE;

    // Get number of Shared peripheral interupt lines
    spi_lines = GICD_READ(GICD_TYPER);
    spi_lines = (spi_lines & 0x1F) * 32;

    // Clear all interrupt enables (including SGIs & PPIs)
    for (i = 0; i < ((spi_lines / 32) + 1); i++)
        GICD_WRITE(GICD_ICENABLE + (i * 4),0xFFFFFFFF);

	cpu_num = arm64_get_cpu_idx();
	dprintf(SPEW, "GIC-SPI Target CPU: %u\n", cpu_num);

    cpu_num = 1 << cpu_num;
    cpu_num = ((cpu_num << 24) | (cpu_num << 16) | (cpu_num << 8) | cpu_num);
    for (i = 0; i < (((spi_lines / 32) + 1) * 8); i++)
        GICD_WRITE(GICD_ITARGETS + (i * 4),cpu_num);

    //Enable GIC distributor
    i = GICD_READ(GICD_CTLR);
    i |= 0x1;
    GICD_WRITE(GICD_CTLR,i);

    //Enable GIC CPU-Interface
    i = GICC_READ(GICC_CTLR);
    i |= 0x1;
    GICC_WRITE(GICC_CTLR,i);

    //Set lowest priority
    GICC_WRITE(GICC_PMR,0xFF);
	dprintf(INFO,"Interrupts Init done\n");
}

void register_int_handler(unsigned int vector, int_handler func, void *arg)
{
    ASSERT(vector < TEGRA_MAX_INT);

    enter_critical_section();
    handler[vector].func = func;
    handler[vector].arg = arg;
    exit_critical_section();
}

status_t mask_interrupt(unsigned int vector)
{
    enter_critical_section();

    GICD_WRITE(GICD_ICENABLE + ((vector / 32) * 4), 1 << (vector % 32));

    exit_critical_section();

    return NO_ERROR;
}

status_t unmask_interrupt(unsigned int vector)
{
    enter_critical_section();

    GICD_WRITE(GICD_ISENABLE + ((vector / 32) * 4), 1 << (vector % 32));

    exit_critical_section();

    return NO_ERROR;
}

