/*
 * Copyright (c) 2014-2018, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited
 */

#define MODULE TEGRABL_ERR_MENU

#include <tegrabl_error.h>
#include <tegrabl_debug.h>
#include <tegrabl_utils.h>
#include <tegrabl_keyboard.h>
#include <tegrabl_timer.h>
#include <menu.h>
#include <err.h>
#include <fastboot.h>
#include <string.h>
#include <common.h>
#include <boot.h>
#include <kernel/thread.h>
#include <kernel/mutex.h>

#include <tegrabl_display.h>

#define SINGLE_BUTTION_LONG_PRESS_TIME_MS	(1500)

static bool s_is_menu_initialised;
static mutex_t menu_mutex;

inline void menu_init(void)
{
	if (s_is_menu_initialised)
		return;
	mutex_init(&menu_mutex);

	s_is_menu_initialised = true;
}

inline void menu_lock(void)
{
	mutex_acquire(&menu_mutex);
}

inline void menu_unlock(void)
{
	mutex_release(&menu_mutex);
}

static tegrabl_error_t display_bmp(tegrabl_image_type_t img_type)
{
	tegrabl_error_t err = TEGRABL_NO_ERROR;
	struct tegrabl_image_info image;

	image.type = img_type;
	image.format = TEGRABL_IMAGE_FORMAT_BMP;

	err = tegrabl_display_clear();
	if (err != TEGRABL_NO_ERROR) {
		TEGRABL_SET_HIGHEST_MODULE(err);
		goto fail;
	}

	err = tegrabl_display_show_image(&image);
	if (err != TEGRABL_NO_ERROR) {
		TEGRABL_SET_HIGHEST_MODULE(err);
		goto fail;
	}

fail:
	return err;
}

static tegrabl_error_t menu_display_backgnd(struct menu *menu)
{
	tegrabl_error_t ret = TEGRABL_NO_ERROR;

	/* The menu background is optional */
	if (!menu->menu_backgnd.valid)
		return ret;

	ret = display_bmp(menu->menu_backgnd.img);

	return ret;
}

static tegrabl_error_t menu_display_header(struct menu *menu)
{
	tegrabl_error_t ret = TEGRABL_NO_ERROR;
	struct menu_string ms;

	/* The menu header is optional */
	if (!menu->menu_header.valid)
		return ret;

	/* Set cursor to the beginning of the screen */
	ret = tegrabl_display_text_set_cursor(CURSOR_START);
	if (ret != TEGRABL_NO_ERROR) {
		TEGRABL_SET_HIGHEST_MODULE(ret);
		return ret;
	}

	/* Display the menu header text */
	ms = menu->menu_header.ms;
	ret = menu_string_print(&ms);
	if (ret != TEGRABL_NO_ERROR)
		return ret;

	return ret;
}

static tegrabl_error_t menu_display_body(struct menu *menu)
{
	tegrabl_error_t ret = TEGRABL_NO_ERROR;
	struct menu_string ms;
	uint32_t i;

	for (i = 0; i < menu->num_menu_entries; i++) {
		if (i == menu->current_entry)
			ms.color = GREEN;
		else
			ms.color = menu->menu_entries[i].ms_entry.color;

		ms.data = menu->menu_entries[i].ms_entry.data;

		ret = menu_string_print(&ms);
		if (ret != TEGRABL_NO_ERROR)
			return ret;
	}

	return ret;
}

static tegrabl_error_t menu_display_footer(struct menu *menu)
{
	tegrabl_error_t ret = TEGRABL_NO_ERROR;
	struct menu_string ms;
	struct text_position *position;

	/* The menu footer is optional */
	if (!menu->menu_footer.valid)
		return ret;

	/* Save current position */
	position = tegrabl_render_text_get_position();

	/* Set cursor position */
	tegrabl_render_text_set_position(menu->menu_footer.pos.x,
									 menu->menu_footer.pos.y);

	/* Print menu string in the menu footer */
	ms = menu->menu_footer.ms;
	ret = menu_string_print(&ms);
	if (ret != TEGRABL_NO_ERROR)
		return ret;

	/* Resume to original position */
	tegrabl_render_text_set_position(position->x, position->y);

	return ret;
}

static void show_busy(void)
{
	static uint32_t i;
	char *progress[] = {".    ", "..   ", "...  ", ".... ", "....."};
	char *wait = "Please wait";

	i = (i + 1) % ARRAY_SIZE(progress);
	tegrabl_display_text_set_cursor(CURSOR_CENTER);
	tegrabl_display_printf(RED, "%s%s\n", wait, progress[i]);
	thread_sleep(1000);
	/* Clear the busy text */
	tegrabl_display_text_set_cursor(CURSOR_CENTER);
	tegrabl_display_printf(RED, "                \n");
	return;
}

static tegrabl_error_t menu_display(struct menu *menu)
{
	tegrabl_error_t ret = TEGRABL_NO_ERROR;

	ret = tegrabl_display_clear();
	if (ret != TEGRABL_NO_ERROR)
		return ret;

	ret = menu_display_backgnd(menu);
	if (ret != TEGRABL_NO_ERROR)
		return ret;

	ret = menu_display_header(menu);
	if (ret != TEGRABL_NO_ERROR)
		return ret;

	ret = menu_display_body(menu);
	if (ret != TEGRABL_NO_ERROR)
		return ret;

	ret = menu_display_footer(menu);
	if (ret != TEGRABL_NO_ERROR)
		return ret;

	return ret;
}

tegrabl_error_t menu_print(struct menu_string *ms)
{
	tegrabl_error_t ret = TEGRABL_NO_ERROR;

	if (!ms || !ms->data)
		return TEGRABL_ERROR(TEGRABL_ERR_INVALID, 0);

	pr_debug("%s\n", ms->data);

	ret = tegrabl_display_clear();
	if (ret != TEGRABL_NO_ERROR) {
		TEGRABL_SET_HIGHEST_MODULE(ret);
		return ret;
	}

	ret = menu_string_print(ms);

	return ret;
}

/**
 * @brief check key press
 *
 * @param key_code code of pressed key, return KEY_IGNORE if no key pressed
 *
 * @return TEGRABL_NO_ERROR if successful
 */
static tegrabl_error_t get_pressed_keys(key_code_t *key_code)
{
	tegrabl_error_t ret = TEGRABL_NO_ERROR;
	key_code_t _key_code;
	key_event_t key_event;
	time_t time_lapse = 0;

	ret = tegrabl_keyboard_get_key_data(&_key_code, &key_event);
	if (ret != TEGRABL_NO_ERROR) {
		TEGRABL_SET_HIGHEST_MODULE(ret);
		return ret;
	}

	/* handle single button usecase */
	if ((_key_code == KEY_HOLD) && (key_event == KEY_PRESS_FLAG)) {
		/* start timer for get key data */
		time_lapse = tegrabl_get_timestamp_ms();
		pr_debug("Single Button Key Hold pressed\n");

		do {
			ret = tegrabl_keyboard_get_key_data(&_key_code, &key_event);
			if (ret != TEGRABL_NO_ERROR) {
				TEGRABL_SET_HIGHEST_MODULE(ret);
				return ret;
			}
		} while (key_event != KEY_RELEASE_FLAG);

		if ((tegrabl_get_timestamp_ms() - time_lapse)
				>= SINGLE_BUTTION_LONG_PRESS_TIME_MS) {
			pr_debug("Single Button Key Enter\n");
			_key_code = KEY_ENTER;
		} else {
			pr_debug("Single Button Key Press\n");
			_key_code = KEY_DOWN;
		}

		key_event = KEY_PRESS_FLAG;
	}

	if (key_event != KEY_PRESS_FLAG)
		*key_code = KEY_IGNORE;
	else
		*key_code = _key_code;

	return ret;
}

static tegrabl_error_t menu_handle_key_press(struct menu *menu,
											 key_code_t key_code,
											 bool *menu_next)
{
	tegrabl_error_t ret = TEGRABL_NO_ERROR;
	*menu_next = false;

	switch (key_code) {
	case KEY_UP:
		pr_debug("KEY_UP pressed\n");

		if (menu->current_entry)
			menu->current_entry--;
		else
			menu->current_entry = menu->num_menu_entries - 1;

		pr_debug("Selected '%s'\n",
				 menu->menu_entries[menu->current_entry].ms_entry.data);
		ret = menu_display(menu);
		if (ret != TEGRABL_NO_ERROR)
			return ret;
		break;

	case KEY_DOWN:
		pr_debug("KEY_DOWN pressed\n");
		menu->current_entry++;
		menu->current_entry %= menu->num_menu_entries;

		pr_debug("Selected '%s'\n",
				 menu->menu_entries[menu->current_entry].ms_entry.data);
		ret = menu_display(menu);
		if (ret != TEGRABL_NO_ERROR)
			return ret;
		break;

	case KEY_ENTER:
		pr_debug("KEY_ENTER pressed\n");
		*menu_next = true;
		break;

	default:
		pr_info("Key Code %u not handled\n", key_code);
		break;
	}

	return TEGRABL_NO_ERROR;
}

tegrabl_error_t menu_draw(struct menu *menu)
{
	tegrabl_error_t ret = TEGRABL_NO_ERROR;
	status_t status = NO_ERROR;

	key_code_t key_code;
	key_event_t key_event;
	bool is_key_press = false;
	time_t start_time = 0;
	bool menu_next;

	/* display boot menu */
	pr_debug("Display boot menu items\n");
	ret = menu_display(menu);
	if (ret != TEGRABL_NO_ERROR)
		goto fail;

	/* initialize keyboard */
	ret = tegrabl_keyboard_init();
	if (ret != TEGRABL_NO_ERROR) {
		pr_debug("Keyboard initialization failed\n");
		goto fail;
	}

	/*
	 * Wait for user to release all keys to avoid pressing the key for too much
	 * time and skip the menu by mistake
	 */
	do {
		ret = tegrabl_keyboard_get_key_data(&key_code, &key_event);
		if (ret != TEGRABL_NO_ERROR) {
			TEGRABL_SET_HIGHEST_MODULE(ret);
			goto fail;
		}
		tegrabl_mdelay(100);
	} while (key_code != KEY_IGNORE);

	/* start timer */
	start_time = tegrabl_get_timestamp_ms();

	while (true) {
		if (menu->menu_type == FASTBOOT_MENU) {
			if (is_fastboot_running() == TERMINATED)
				break;
			else if (is_fastboot_running() == PAUSED)
				continue;
		}

		/* check for time out */
		if (!is_key_press && menu->timeout &&
			((tegrabl_get_timestamp_ms() - start_time) >= menu->timeout)) {
			pr_debug("Timed out waiting for '%u' seconds in menu\n",
					 (menu->timeout / 1000));
			ret = TEGRABL_ERROR(TEGRABL_ERR_TIMEOUT, 1);
			goto fail;
		}

		/* Show busy on menu when mutex is not free */
		status = mutex_acquire_timeout(&menu_mutex, 0);
		if (status == ERR_TIMED_OUT) {
			show_busy();
			continue;
		} else if (status != NO_ERROR) {
			ret = TEGRABL_ERROR(TEGRABL_ERR_LOCK_FAILED, 0);
			goto fail;
		}

		/* wait for key press */
		ret = get_pressed_keys(&key_code);
		if (ret != TEGRABL_NO_ERROR)
			goto fail;

		if (key_code == KEY_IGNORE) {
			menu_unlock();
			continue;
		}

		ret = menu_handle_key_press(menu, key_code, &menu_next);
		if (ret != TEGRABL_NO_ERROR)
			goto fail;

		menu_unlock();

		if (menu_next)
			break;

		tegrabl_mdelay(200);
	}
fail:
	return ret;
}

tegrabl_error_t menu_handle_on_select(struct menu *menu)
{
	tegrabl_error_t ret = TEGRABL_NO_ERROR;
	uint8_t current_entry = menu->current_entry;
	struct menu_entry *cur_menu_entry;
	struct menu_string ms;

	ms = menu->menu_entries[current_entry].ms_on_select;
	menu_print(&ms);

	cur_menu_entry = &menu->menu_entries[current_entry];
	if (cur_menu_entry->on_select_callback)
		ret = cur_menu_entry->on_select_callback(cur_menu_entry->arg);

	if (ret != TEGRABL_NO_ERROR)
		TEGRABL_SET_HIGHEST_MODULE(ret);

	return ret;
}
