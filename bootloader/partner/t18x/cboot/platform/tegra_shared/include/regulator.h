/*
 * Copyright (c) 2014-2017, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#ifndef __REGULATOR_H_
#define __REGULATOR_H_

#include <common.h>
#include <err.h>

/* defines regulator volt type */
/* macro regulator volt type */
#define USER_DEFINED_VOLTS 0
#define STANDARD_VOLTS 1
typedef uint32_t regulator_volt_type;

status_t regulator_is_fixed(int32_t phandle, bool *is_fixed);

/**
 * @brief api to enable a regulator.
 *
 * @param phandle pointer to regulator phandle.
 *
 * @return NO_ERROR on success otherwise error.
 */
status_t regulator_enable(int32_t phandle);

/**
 * @brief api to disable a regulator.
 *
 * @param phandle pointer to regulator phandle.
 *
 * @return NO_ERROR on success otherwise error.
 */
status_t regulator_disable(int32_t phandle);

/**
 * @brief api to set regulator voltage.
 *
 * @param phandle pointer to regulator phandle.
 * @param volts   micro volts to be set. can be zero if regulator_volt_type
 *				  is set to STANDARD_VOLTS.
 * @param regulator_volt_type can be either STANDARD_VOLTS/USER_DEFINED_VOLTS.
 * NOTE: 		  if regulator_volt_type is set to STANDARD_VOLTS, volts
 *				  parameter value wouldn't be considered.
 *
 * NOTE: also this only sets the voltage of a regulator and doesn't
 *			necessarily enable the regulator if its disabled.
 *			its expected consumer would call regulator_enable(phandle)
 *			for enabling the regulator.
 *
 * @return NO_ERROR on success otherwise error.
 */
status_t regulator_set_voltage(int32_t phandle, uint32_t volts,
		regulator_volt_type volt_type);

#endif
