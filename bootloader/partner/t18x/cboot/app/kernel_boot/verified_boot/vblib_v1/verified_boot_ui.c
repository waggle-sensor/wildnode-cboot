/*
 * Copyright (c) 2015-2018, NVIDIA Corporation.	All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#include <verified_boot_ui.h>
#include <mincrypt/sha256.h>
#include <menu_data.h>
#include <menu.h>
#include <err.h>
#include <sys/types.h>
#include <string.h>
#include <tegrabl_utils.h>
#include <tegrabl_malloc.h>
#include <printf.h>
#include <debug.h>
#include <tegrabl_error.h>
#include <verified_boot.h>

#define FINGERPRINT_LEN_BYTES 19

static status_t verified_boot_draw_menu(boot_state_t bs)
{
	struct menu *menu_pause, *menu_continue;
	tegrabl_error_t ret = TEGRABL_NO_ERROR;

	switch (bs) {
	case VERIFIED_BOOT_YELLOW_STATE:
		menu_pause = &yellow_state_menu_pause;
		menu_continue = &yellow_state_menu_continue;
		break;
	case VERIFIED_BOOT_RED_STATE:
		menu_pause = &red_state_menu_pause;
		menu_continue = &red_state_menu_continue;
		break;
	case VERIFIED_BOOT_ORANGE_STATE:
		menu_pause = &orange_state_menu_pause;
		menu_continue = &orange_state_menu_continue;
		break;
	default:
		return TEGRABL_NO_ERROR;
	}

	ret = menu_draw(menu_pause);
	if (TEGRABL_ERROR_REASON(ret) == TEGRABL_ERR_TIMEOUT ||
		ret != TEGRABL_NO_ERROR)
		return ret;

	ret = menu_draw(menu_continue);

	return ret;
}

status_t verified_boot_red_state_ui(void)
{
	return verified_boot_draw_menu(VERIFIED_BOOT_RED_STATE);
}

/* NOTE: Contains hard codings. Is not meant for reuse.
 * Generates fingerprint in format AABB-CCDD-EEFF-GGHH as on Nexus UIs
 */
static status_t get_fingerprint(struct rsa_public_key *pub_key,
								char **out_fp)
{
	uint8_t digest[SHA256_DIGEST_SIZE];
	uint32_t num_chars = 8, idx;
	char *ptr, *end, *fingerprint;

	/* Over allocating for convenience */
	fingerprint = tegrabl_malloc(sizeof(char) * num_chars * 3);
	if (!fingerprint)
		return ERR_NO_MEMORY;

	sha256_hash(pub_key->n, VERITY_KEY_SIZE, digest);

	/* Truncate to 8 bytes hash string */
	ptr = fingerprint;
	end = fingerprint + num_chars * 3;
	idx = 0;

	while (idx < num_chars) {
		snprintf(ptr, end - ptr, "%02X%02X-",
				 digest[SHA256_DIGEST_SIZE - 1 - idx],
				 digest[SHA256_DIGEST_SIZE - 1 - idx - 1]);
		ptr += 5;
		idx += 2;
	};

	*(ptr - 1) = '\0';

	*out_fp = fingerprint;

	return NO_ERROR;
}

status_t verified_boot_yellow_state_ui(struct rsa_public_key *boot_pub_key,
									   struct rsa_public_key *dtb_pub_key)
{
	char *boot_fingerprint = NULL;
	char *dtb_fingerprint = NULL;
	char *footer_str = NULL;
	status_t ret = NO_ERROR;

	/* Over allocating for convenience
	 * fingerprint footer format:
	 * boot.img  : xxxx-xxxx-xxxx-xxxx
	 * kernel-dtb: xxxx-xxxx-xxxx-xxxx */
	footer_str = tegrabl_malloc(FINGERPRINT_LEN_BYTES * 2 + 32);
	if (!footer_str)
		return ERR_NO_MEMORY;

	ret = get_fingerprint(boot_pub_key, &boot_fingerprint);
	if (ret != NO_ERROR)
		pr_error("Could not get RSA finger print of boot.img\n");
	else {
		pr_info("Boot image public key fingerprint = %s\n", boot_fingerprint);
		sprintf(footer_str, "boot.img  : %s\n", boot_fingerprint);
	}

	ret = get_fingerprint(dtb_pub_key, &dtb_fingerprint);
	if (ret != NO_ERROR)
		pr_error("Could not get RSA finger print of kernel-dtb\n");
	else {
		pr_info("Kernel-dtb public key fingerprint = %s\n", dtb_fingerprint);
		sprintf(footer_str + strlen(footer_str), "kernel-dtb: %s\n",
				dtb_fingerprint);
	}

	yellow_state_menu_pause.menu_footer.ms.data = footer_str;
	yellow_state_menu_continue.menu_footer.ms.data = footer_str;

	ret = verified_boot_draw_menu(VERIFIED_BOOT_YELLOW_STATE);

	tegrabl_free(boot_fingerprint);
	tegrabl_free(dtb_fingerprint);
	tegrabl_free(footer_str);

	return ret;
}

status_t verified_boot_orange_state_ui(void)
{
	return verified_boot_draw_menu(VERIFIED_BOOT_ORANGE_STATE);
}
