/*
 * Copyright (c) 2014-2017, NVIDIA CORPORATION. All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#ifndef __GPIO_DRIVER_H
#define __GPIO_DRIVER_H

#include <sys/types.h>
#include <kernel/mutex.h>
#include <list.h>
#include <stdlib.h>

/* macro gpio pin state */
typedef uint32_t gpio_pin_state_t;
#define GPIO_PIN_STATE_LOW 0 /* GPIO pin in LOW state. */
#define GPIO_PIN_STATE_HIGH 1 /* GPIO pin in HIGH state. */

/* macro gpio pin mode */
typedef uint32_t gpio_pin_mode_t;
#define GPIO_PINMODE_INPUT 0 /* GPIO pin in INPUT mode */
#define GPIO_PINMODE_OUTPUT 1 /* GPIO pin in OUTPUT mode */
#define GPIO_PINMODE_SPIO 2 /* Configure pin to SPIO mode */

/* It is mandatory for any GPIO driver to register these APIs.
 * Else the driver will not be registered to the GPIO core
 */
struct gpio_driver_ops {
	status_t (*read)(uint32_t gpio_num, gpio_pin_state_t *state,
					void *drv_data);
	status_t (*write)(uint32_t gpio_num, gpio_pin_state_t state,
					void *drv_data);
	status_t (*config)(uint32_t gpio_num, gpio_pin_mode_t mode,
					void *drv_data);
};

/* Every gpio driver has to fill the below fields before calling
 * gpio_driver_register()
 */
struct gpio_driver {
	uint32_t phandle; /* phandle of the GPIO controller. */
	const char *name; /* GPIO driver name. like 'tegra_gpio_driver' etc. */
	void *driver_data; /* private driver data for each device */
	struct list_node node; /* will be part of GPIO drivers list. */
	struct gpio_driver_ops *ops; /* Handle the consumers are interested in */
};

/**
 * @brief The GPIO API for gpio driver registration.
 *
 * @param drv Pointer to the GPIO driver to be registered.
 *
 * @return NO_ERROR on success otherwise error.
 */
status_t gpio_driver_register(struct gpio_driver *drv);

/**
 * @brief Retrieve handle to a gpio driver from its DTB phandle
 *
 * @param phandle phandle of the controller node in DTB
 * @param out Callee filled. Points to the driver structure on success
 *
 * @return NO_ERROR on success and *out points to the required gpio driver
 *		   handle. On Failure, *out shall be NULL
 */
status_t gpio_driver_get(uint32_t phandle, struct gpio_driver **out);


/**
 * @brief The GPIO API for reading a GPIO pin state
 *
 * @param GPIO driver handle
 * @param gpio_num GPIO pin number
 * @param mode GPIO pin state (LOW/HIGH)
 *
 * @return NO_ERROR on succes with GPIO state in 'state' argument
 *		   otherwise error
 */
static inline status_t gpio_read(struct gpio_driver *drv,
				uint32_t gpio_num, gpio_pin_state_t *state)
{
	return drv->ops->read(gpio_num, state, drv->driver_data);
}

/**
 * @brief The GPIO API for setting the state of a GPIO pin
 *
 * @param GPIO driver handle
 * @param gpio_num GPIO pin number
 * @param mode GPIO pin state (LOW/HIGH)
 *
 * @return NO_ERROR on succes otherwise error
 */
static inline status_t gpio_write(struct gpio_driver *drv,
				uint32_t gpio_num, gpio_pin_state_t state)
{
	return drv->ops->write(gpio_num, state, drv->driver_data);
}

/**
 * @brief The GPIO API for configuring a GPIO pin
 *
 * @param GPIO driver handle
 * @param gpio_num GPIO pin number.
 * @param mode GPIO pin mode (INPUT/OUTPUT)
 *
 * @return NO_ERROR on success otherwise error
 */
static inline status_t gpio_config(struct gpio_driver *drv,
				uint32_t gpio_num, gpio_pin_mode_t mode)
{
	return drv->ops->config(gpio_num, mode, drv->driver_data);
}

#endif
