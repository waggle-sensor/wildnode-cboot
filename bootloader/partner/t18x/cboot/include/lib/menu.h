/*
 * Copyright (c) 2014-2018, NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#ifndef __MENU_H__
#define __MENU_H__

#include <sys/types.h>
#include <tegrabl_debug.h>
#include <tegrabl_nvblob.h>
#include <tegrabl_display.h>

/* macro menu type */
typedef uint32_t menu_type_t;
#define ANDROID_BOOT_MENU 0
#define FASTBOOT_MENU 1
#define VERIFIED_BOOT_MENU 2
#define OTHER_MENU 3

/**
 * @brief String object for the console menu
 *
 * @param color The color to display the string in
 * @param data	The char array pointing to string data
 *				In most cases, its gonna point to a literal string. Hence the
 *				const qualifier
 */
struct menu_string {
	color_t color;
	const char *data;
};

/**
 * @brief Menu background image object
 *
 * @param valid whether the object is valid
 * @param img Background image id
 */
struct menu_backgnd {
	bool valid;
	tegrabl_image_type_t img;
};

/**
 * @brief Menu header object
 *
 * @param valid whether the object is valid
 * @param ms Header menu string
 */
struct menu_header {
	bool valid;
	struct menu_string ms;
};

/**
 * @brief Menu footer object
 *
 * @param valid whether the object is valid
 * @param ms Footer menu string
 * @param pos position to show the footer
 */
struct menu_footer {
	bool valid;
	struct menu_string ms;
	struct text_position pos;
};

/**
 * @brief Menu entry object
 *
 * @param ms_entry text corresponding to the entry on menu
 * @param ms_on_select text to show when the menu entry gets selected
 * @param on_select_callback
 *		  @brief Callback function associated with when the entry gets selected.
 *				 The onus of calling this function lies on the consumer code.
 *		  @param arg An optional parameter to be passed by consumer code to the
 *				 callback function
 *		  @return TEGRABL_NO_ERROR if the callback executes successfully
 * @arg Argument meaningful to the callback function
 */
struct menu_entry {
	struct menu_string ms_entry;
	struct menu_string ms_on_select;
	tegrabl_error_t (*on_select_callback)(void *arg);
	void *arg;
};

/**
 * @brief Menu object
 *
 * @param menu_type Type of the menu.
 * @param name Name of the menu. Last resort for (hopefully temp) workarounds
 * @param menu_backgnd Optinal background object for verified boot state notification
 * @param menu_header Optional header object for the menu
 * @param menu_footer Optional footer object for the menu
 * @param menu_entries List of menu entry objects for the menu
 * @param num_menu_entries Number of entries in menu_entries array
 * @param timeout The menu times out after 'timeout' seconds. 0 to disable this
 * @param current_entry currently pointed to menu entry (state variable)
 */
struct menu {
	menu_type_t menu_type;
	const char *name;
	struct menu_backgnd menu_backgnd;
	struct menu_header menu_header;
	struct menu_footer menu_footer;
	struct menu_entry *menu_entries;
	uint32_t num_menu_entries;
	uint32_t timeout;
	uint8_t current_entry;
};

/**
 * @brief initialise menu framework
 */
void menu_init(void);

/**
 * @brief lock menu mutex
 */
void menu_lock(void);

/**
 * @brief release menu mutex
 */
void menu_unlock(void);

/**
 * @brief starts bootloader menu
 *
 * @param menutype shows booloader menu or fastboot menu
 *
 * @return ERR_type on error otherwise TEGRABL_NO_ERROR
 */
tegrabl_error_t menu_draw(struct menu *menu);

/**
 * @brief Clears text console before printing the menu string
 *
 * @param color color of the text
 * @param msg text to be printed on the console
 *
 * @return TEGRABL_NO_ERROR on success
 */
tegrabl_error_t menu_print(struct menu_string *ms);

/**
 * @brief process menu entry specific on-select data
 *
 * @param menu menu to be handled
 *
 * @return TEGRABL_NO_ERROR on success
 */
tegrabl_error_t menu_handle_on_select(struct menu *menu);

/**
 * @brief print a given menu string on display console
 *		  Does not complain about an empty string to facilitate WARs for
 *		  text positioning
 *
 * @param ms Menu string to print
 *
 * @return TEGRABL_NO_ERROR on success
 */
static inline tegrabl_error_t menu_string_print(struct menu_string *ms)
{
	if (!ms->data)
		return TEGRABL_NO_ERROR;

	return tegrabl_display_printf(ms->color, ms->data);
}

#endif /* __MENU_H__ */
