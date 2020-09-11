/*
 * Copyright (c) 2014-2017, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited
 */

#ifndef __PLATFORM_C_H
#define __PLATFORM_C_H

#include <sys/types.h>
#include <err.h>
#include <platform_reg.h>
#include <reg.h>


/**
* @brief Initialize interrupts.
*/
void platform_init_interrupts(void);

/**
* @brief Initialize timers.
*/
void platform_init_timer(void);

/**
* @brief Initialize debug.
*/
void tegra_debug_init(void);

/**
* @brief Deinitialize timers.
*/
void platform_uninit_timer(void);

/**
* @brief Initialize device tree.
*/
void platform_init_dt(void);

/**
* @brief Initialize clocks.
*/
void platform_init_clocks(void);

/**
* @brief Waits in micro seconds.
*
* @param usecs Specifies time in micro seconds.
*/
void udelay(uint32_t usecs);

/**
* @brief Waits in milli seconds.
*
* @param msecs Specifies time in milli seconds.
*/
void mdelay(uint32_t msecs);

/**
 * @brief Initializes pinmuxes.
 *
 * @return NO_ERROR on success otherwise ERR_* type.
 */
status_t platform_pinmux_init(void);

/**
* @brief Gets time stamp in milli seconds. This function uses timerus
*        and can be called at any stage in the code as compared to
*        current_time().
*
* @return returns time stamp in lk_time_t.
*/
lk_time_t get_time_stamp_ms(void);

/**
* @brief Gets time stamp in micro seconds. This function uses timerus
*        and can be called at any stage in the code as compared to
*        current_time_hires().
*
* @return returns time stamp in lk_bigtime_t.
*/
lk_bigtime_t get_time_stamp_us(void);

#endif
