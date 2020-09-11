/*
 * Copyright (c) 2018-2019, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited
 */

#define MODULE TEGRABL_ERR_SHELL

#include <string.h>
#include <ctype.h>
#include <var_cmd.h>
#include <tegrabl_malloc.h>
#include <tegrabl_debug.h>
#include <tegrabl_error.h>
#include <tegrabl_utils.h>
#include <tegrabl_cbo.h>

void update_boot_cfg_var(int argc, const cmd_args *argv)
{
	uint32_t i = 0, j = 0;
	uint32_t count;
	const char **boot_order = NULL;
	uint8_t ip[4];
	char *tok = NULL;
	unsigned long num = 0;

	if (!strcmp(argv[1].str, "boot-order")) {
		count = argc - 2; /* first is cmd, second is var name */
		boot_order = tegrabl_calloc(sizeof(char *), count);
		if (boot_order == NULL) {
			pr_error("Memory allocation failed for boot_order\n");
			goto fail;
		}
		for (i = 2; count > 0; count--, i++) {
			boot_order[j] = argv[i].str;
			j++;
		}
		tegrabl_set_boot_order(argc - 2, boot_order);
		tegrabl_free(boot_order);
	} else if (!strcmp(argv[1].str, "dhcp-enabled")) {
		tegrabl_set_ip_info(argv[1].str, ip, argv[2].b);
	} else if (!strcmp(argv[1].str, "boot_pt_guid")) {
		tegrabl_set_boot_pt_guid(argv[1].str, argv[2].str);
	} else { /* else its ip */
		tok = strtok(argv[2].str, ".");
		while (tok != NULL) {
			num = tegrabl_utils_strtoul(tok, NULL, 10);
			if (num > 0xFFUL) {
				pr_info("Invalid ip address.\n");
				goto fail;
			}
			ip[i++] = (uint8_t)num;
			tok = strtok(NULL, ".");
		}

		tegrabl_set_ip_info(argv[1].str, ip, argv[2].b);
	}

fail:
	return;
}

void update_var(int count, const cmd_args *argv)
{
	/* Dummy for now */
	pr_info("invalid variable\n");
	return;
}

void clear_boot_cfg_var(const char *var)
{
	if (!strcmp(var, "boot-order")) {
		tegrabl_clear_boot_order();
	} else if (!strcmp(var, "boot_pt_guid")) {
		tegrabl_clear_boot_pt_guid();
	} else {
		tegrabl_clear_ip_info(var);
	}
}

void clear_var(const char *var)
{
	/* Dummy for now */
	pr_info("invalid variable\n");
	return;
}

void print_boot_cfg_var(const char *var)
{
	if (!strcmp(var, "boot-order")) {
		tegrabl_print_boot_order();
	} else if (!strcmp(var, "boot_pt_guid")) {
		tegrabl_print_boot_pt_guid();
	} else {
		tegrabl_print_ip_info(var);
	}
}

void print_var(const char *var)
{
	/* Dummy for now */
	pr_info("invalid variable\n");
	return;
}
