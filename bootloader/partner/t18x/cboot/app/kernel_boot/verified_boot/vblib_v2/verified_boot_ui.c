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
#include <libavb/libavb.h>

#define MAX_FINGERPRINT_NUM   8
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
static status_t get_fingerprint(const uint8_t *pub_key,
								char **out_fp)
{
	uint8_t digest[SHA256_DIGEST_SIZE];
	uint32_t num_chars = 8, idx;
	char *ptr, *end, *fingerprint;

	/* Over allocating for convenience */
	fingerprint = tegrabl_malloc(sizeof(char) * num_chars * 3);
	if (!fingerprint)
		return ERR_NO_MEMORY;

	sha256_hash(pub_key, SHA256_DIGEST_SIZE, digest);

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

status_t verified_boot_yellow_state_ui(AvbSlotVerifyData *slot_data)
{
	char *fingerprint[MAX_FINGERPRINT_NUM] = {NULL};
	char *footer_str = NULL;
	status_t ret = NO_ERROR;
	uint8_t num_fp;
	uint8_t i;
	const uint8_t *pub_key = NULL;

	TEGRABL_ASSERT(slot_data);

	num_fp = slot_data->num_vbmeta_images;

	/* Over allocating for convenience
	 * fingerprint footer format:
	 * <partition>: xxxx-xxxx-xxxx-xxxx */
	footer_str = tegrabl_malloc(FINGERPRINT_LEN_BYTES * num_fp + 32);
	if (!footer_str) {
		return ERR_NO_MEMORY;
	}

	for (i = 0; i < num_fp; i++) {
		pub_key = slot_data->vbmeta_images[i].pub_key;
		if (!pub_key) {
			continue;
		}
		ret = get_fingerprint(pub_key, &fingerprint[i]);
		if (ret != NO_ERROR)
			pr_error("Could not get RSA finger print of boot.img\n");
		else {
			pr_info("%s: public key fingerprint = %s\n",
					slot_data->vbmeta_images[i].partition_name,
					fingerprint[i]);
			sprintf(footer_str, "%s : %s\n",
					slot_data->vbmeta_images[i].partition_name,
					fingerprint[i]);
		}
	}

	yellow_state_menu_pause.menu_footer.ms.data = footer_str;
	yellow_state_menu_continue.menu_footer.ms.data = footer_str;

	ret = verified_boot_draw_menu(VERIFIED_BOOT_YELLOW_STATE);

	for (i = 0; i < num_fp; i++) {
		tegrabl_free(fingerprint[i]);
	}
	tegrabl_free(footer_str);

	return ret;
}

status_t verified_boot_orange_state_ui(void)
{
	return verified_boot_draw_menu(VERIFIED_BOOT_ORANGE_STATE);
}
