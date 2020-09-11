/*
 * Copyright (c) 2014-2017, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#ifndef __DISPLAY_H
#define __DISPLAY_H

#include <debug.h>
#include <err.h>
#include <assert.h>
#include <common.h>
#include <errno.h>

/* macro color_t*/
typedef uint32_t color_t;
#define RED 0
#define WHITE 1
#define GREEN 2
#define BLUE 3
#define YELLOW 4
#define ORANGE 5

color_t color;

/* macro cursor position */
typedef uint32_t cursor_position_t;
#define CURSOR_START 0
#define CURSOR_CENTRE 1
#define CURSOR_END 2

cursor_position_t cursor_position;

/* macro rotation type */
#define TEXT_ROTATION 0
#define IMAGE_ROTATION 1
#define TEXT_IMAGE_ROTATION 2
typedef uint32_t rotation_type;

#define DISPLAY_IOCTL_SET_SW_ROTATION 0
#define DISPLAY_IOCTL_SET_HW_ROTATION 1
#define DISPLAY_IOCTL_SET_FONT 2
#define DISPLAY_IOCTL_CONTROL_BACKLIGHT 3
#define DISPLAY_IOCTL_GET_DISPLAY_PARAMS 4

struct backlight_pwm {
	uint32_t handle;
	uint32_t channel;
	uint32_t period;
	uint32_t max_brightness;
	uint32_t default_brightness;
};

/* Used for DISPLAY_IOCTL_SET_SW_ROTATION ioctl */
struct display_sw_rotation {
	uint32_t type;
	uint32_t angle;
};

/* Used for DISPLAY_IOCTL_SET_FONT ioctl */
struct display_font {
	uint32_t type;
	uint32_t size;
};

/* Used for DISPLAY_IOCTL_CONTROL_BACKLIGHT ioctl */
struct display_backlight {
	bool is_on;
};

/* Used for DISPLAY_IOCTL_GET_DISPLAY_PARAMS ioctl */
struct display_params {
	uint32_t addr; /* frame buffer address */
	uint32_t size; /* frame buffer size */
	uint32_t instance; /* Dc instance */
	uint32_t out_type; /* DC out type */
	uint32_t orientation; /* Orientation data for the kernel */
};

/**
 *  @brief get display resolution
 *
 *  @param pImageRes resolution of display, to be filled
 *  @param pPortraitPanel whether panel is portrait, to be filled
 *
 *  @return NO_ERROR if valid resolution is found
 */
status_t display_resolution(uint32_t *pImageRes, bool *pPortraitPanel);

/**
 *  @brief shutdown display panel
 */
void display_console_shutdown(void);

/**
 *  @brief Initializes the display console
 *
 *  @return NO_ERROR if initialization is successful
 */
status_t display_console_init(void);

/**
 *  @brief Switches backlight ON or OFF
 *
 *  @param turn_on must be true/false to turn ON/OFF the backlight
 */
void switch_backlight(bool turn_on);

/**
 *  @brief Prints the given text in given color on display console
 *         The text should be at most 1024 characters long
 *
 *  @param color Color of characters in the text.
 *  @param format  Pointer to the text string to print
 *
 *  @return NO_ERROR Printing the given text on display console is successful.
 */
status_t display_console_printf(uint32_t color, const char *format, ...);

/**
 *  @brief Prints the given bmp image on the centre of display console
 *
 *  @param image  Pointer to the buffer of the image
 *  @param length  Length of the given image buffer.
 *
 *  @return NO_ERROR Printing the given text on display console is successful.
 */
status_t display_console_show_bmp(uint8_t *image, uint32_t length);

/**
 *  @brief Clears the display console and sets the cursor position to start
 *
 *  @return NO_ERROR  display console is blank and cursor position is at start.
 */
status_t display_console_clear(void);

/**
 *  @brief Setup orientation of display
 *
 *  @return NO_ERROR setup display orientation successful
 */
status_t display_setup_orientation(void);

/**
 *  @brief Sets the cursor to given position. Then further text prints starts
 *         from there.
 *
 *  @param position  Position of the cursor to set either at start or
 *                   centre or end.
 *
 *  @return NO_ERROR Set cursor to given position is successful.
 */
status_t display_console_text_set_cursor(uint32_t position);

/**
 *  @brief  Ioctl for the display console.
 *
 *  @param ioctl Ioctl to be set or get.
 *
 *  @param in_args Input arguments for the ioctl.
 *
 *  @param out_args Output argumnets for the ioct.
 *
 *  @return NO_ERROR  Successfully set ioctl.
 */
status_t display_console_ioctl(uint32_t ioctl, void *in_args, void *out_args);


#endif
