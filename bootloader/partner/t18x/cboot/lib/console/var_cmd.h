/*
 * Copyright (c) 2018, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited
 */

#ifndef INCLUDED_VAR_CMD_H
#define INCLUDED_VAR_CMD_H

#include <lib/console.h>
#include <tegrabl_cbo.h>

void get_dtb_handle(void);

void update_boot_cfg_var(int count, const cmd_args *argv);

void update_var(int count, const cmd_args *argv);

void clear_boot_cfg_var(const char *var);

void clear_var(const char *var);

void print_boot_cfg_var(const char *var);

void print_var(const char *var);

#endif
