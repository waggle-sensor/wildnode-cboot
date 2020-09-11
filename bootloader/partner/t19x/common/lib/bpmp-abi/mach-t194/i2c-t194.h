/*
 * Copyright (c) 2017-2018, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#ifndef BPMP_ABI_MACH_T194_I2C_T194_H
#define BPMP_ABI_MACH_T194_I2C_T194_H

/**
 * @file
 * @defgroup bpmp_i2c_defines I2C Defines
 * @{
 *   @defgroup bpmp_i2c_fw_defines I2C Firewall Defines
 *   @defgroup bpmp_i2c_ids I2C controller ID's
 * @}
 */

/**
 * @addtogroup bpmp_i2c_fw_defines
 * @{
 */
#define I2C_FW_NOMATCH 0
#define I2C_FW_ALLOW 1
#define I2C_FW_DENY 2

#define I2C_FW_IS 1
#define I2C_FW_IS_NOT 2
#define I2C_FW_CONDITION_OFF 3
/** @} */

/**
 * @addtogroup bpmp_i2c_ids
 * @{
 */
#define TEGRA194_I2C1 1
#define TEGRA194_I2C2 2
#define TEGRA194_I2C3 3
#define TEGRA194_I2C4 4
#define TEGRA194_I2C5 5
#define TEGRA194_I2C6 6
#define TEGRA194_I2C7 7
#define TEGRA194_I2C8 8
#define TEGRA194_I2C9 9
#define TEGRA194_I2C10 10
#define TEGRA194_I2C_MAX 11
/** @} */

#endif
