/*
 * Copyright (c) 2015-2018, NVIDIA Corporation. All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#include <menu.h>

/* Hidden 'Continue' item */
static struct menu_entry continue_menu_entry[] = {
	{ /* Entry 1 */
		.ms_entry = {
			.data = NULL,
		},
	},
};

/* Yellow state pause menu: press power to pause boot */
struct menu yellow_state_menu_pause = {
	.menu_type = VERIFIED_BOOT_MENU,
	.name = "yellow-state-menu-pause",
	.menu_entries = continue_menu_entry,
	.num_menu_entries = ARRAY_SIZE(continue_menu_entry),
	.menu_backgnd = {
		.valid = true,
		.img = IMAGE_VERITY_YELLOW_PAUSE,
	},
	.menu_footer = {
		.valid = true,
		.ms = {
			.color = WHITE,
			.data = NULL
		},
		.pos = {
			.x = 0,
			.y = 1000
		}
	},
	.timeout = 5000, /* in ms */
	.current_entry = 0,
};

/* Yellow state continue menu: press power to continue booting */
struct menu yellow_state_menu_continue = {
	.menu_type = VERIFIED_BOOT_MENU,
	.name = "yellow-state-menu-continue",
	.menu_entries = continue_menu_entry,
	.num_menu_entries = ARRAY_SIZE(continue_menu_entry),
	.menu_backgnd = {
		.valid = true,
		.img = IMAGE_VERITY_YELLOW_CONTINUE,
	},
	.menu_footer = {
		.valid = true,
		.ms = {
			.color = WHITE,
			.data = NULL
		},
		.pos = {
			.x = 0,
			.y = 1000
		}
	},
	.timeout = 0,
	.current_entry = 0,
};

/* Red state pause menu: press power to pause boot */
struct menu red_state_menu_pause = {
	.menu_type = VERIFIED_BOOT_MENU,
	.name = "red-state-menu-pause",
	.menu_entries = continue_menu_entry,
	.num_menu_entries = ARRAY_SIZE(continue_menu_entry),
	.menu_backgnd = {
		.valid = true,
		.img = IMAGE_VERITY_RED_PAUSE,
	},
	.menu_footer = {
		.valid = true,
		.ms = {
			.color = WHITE,
			.data = NULL
		}
	},
	.timeout = 5000, /* in ms */
	.current_entry = 0,
};

/* Red state continue menu: press power to continue booting */
struct menu red_state_menu_continue = {
	.menu_type = VERIFIED_BOOT_MENU,
	.name = "red-state-menu-continue",
	.menu_entries = continue_menu_entry,
	.num_menu_entries = ARRAY_SIZE(continue_menu_entry),
	.menu_backgnd = {
		.valid = true,
		.img = IMAGE_VERITY_RED_CONTINUE,
	},
	.menu_footer = {
		.valid = true,
		.ms = {
			.color = WHITE,
			.data = NULL
		}
	},
	.timeout = 0,
	.current_entry = 0,
};


/* Orange state pause menu: press power to pause boot */
struct menu orange_state_menu_pause = {
	.menu_type = VERIFIED_BOOT_MENU,
	.name = "orange-state-menu-pause",
	.menu_entries = continue_menu_entry,
	.num_menu_entries = ARRAY_SIZE(continue_menu_entry),
	.menu_backgnd = {
		.valid = true,
		.img = IMAGE_VERITY_ORANGE_PAUSE,
	},
	.menu_footer = {
		.valid = true,
		.ms = {
			.color = WHITE,
			.data = NULL
		}
	},
	.timeout = 5000, /* in ms */
	.current_entry = 0,
};

/* Orange state continue menu: press power to continue booting */
struct menu orange_state_menu_continue = {
	.menu_type = VERIFIED_BOOT_MENU,
	.name = "orange-state-menu-continue",
	.menu_entries = continue_menu_entry,
	.num_menu_entries = ARRAY_SIZE(continue_menu_entry),
	.menu_backgnd = {
		.valid = true,
		.img = IMAGE_VERITY_ORANGE_CONTINUE,
	},
	.menu_footer = {
		.valid = true,
		.ms = {
			.color = WHITE,
			.data = NULL
		}
	},
	.timeout = 0,
	.current_entry = 0,
};
