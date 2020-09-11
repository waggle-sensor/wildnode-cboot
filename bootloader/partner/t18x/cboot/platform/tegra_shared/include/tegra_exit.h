/*
 * Copyright (c) 2016, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited
 */

#ifndef TEGRA_EXIT_H_INCLUDED
#define TEGRA_EXIT_H_INCLUDED

/**
 * @brief shutdown routine in bootloader. Please call this one when you need to
 *		  turn off the device, instead of calling PMIC/PMC routines directly
 *
 * @param start_node NO_ERROR on success
 */
status_t tegra_poweroff(void);

/**
 * @brief Reset routine in bootloader. Please call this one when you need to
 *		  reboot the device, instead of calling PMIC/PMC routines directly
 */
void tegra_reset(void);

#endif
