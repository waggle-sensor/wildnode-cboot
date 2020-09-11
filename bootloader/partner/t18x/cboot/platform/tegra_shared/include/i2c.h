/*
 * Copyright (c) 2014-2017, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */
#ifndef __TEGRA_I2C_H
#define __TEGRA_I2C_H

#include <sys/types.h>
/* i2c instances */
/* macro i2c1 */
#define i2c1 1
#define i2c2 2
#define i2c3 3
#define i2c4 4
#define i2c5 5
#define i2c6 6
    /* Should be last enum. */
#define i2c_instances i2c6
typedef uint32_t i2c_instance;


/**
 * @brief Read number of bytes from slave connected to
 * specified i2c controller
 *
 * @param instance instance of i2c controller
 * @param slave_addr address of i2c slave
 * @param slave_reg internal register offset
 * @param buf buffer into which read data to be stored
 * @param buf_size number of bytes to be read from slave
 *
 * @return zero if no error else negative value
 */
status_t i2c_read(uint32_t instance,
				  uint32_t slave_addr,
				  uint8_t slave_reg,
				  uint8_t *buf,
				  uint32_t buf_size);

/**
 * @brief Read number of bytes from slave connected to
 * specified i2c controller with flexible reg addressing
 * and flexible value length per register
 *
 * @param instance instance of i2c controller
 * @param slave_addr address of i2c slave
 * @param reg_addr internal register offset
 * @param byte_per_reg value length per internal register
 * @param reg_size addressing length of internal register offset
 * @param buf buffer into which read data to be stored
 * @param buf_size number of bytes to be read from slave
 *
 * @return zero if no error else negative value
 */
status_t i2c_read_extend(uint32_t instance,
						 uint32_t slave_addr,
						 uint8_t *reg_addr,
						 uint32_t reg_size,
						 uint32_t byte_per_reg,
						 uint8_t *buf,
						 uint32_t buf_size);

/**
 * @brief Write number of bytes to slave connected to
 * specified i2c cotntroller
 *
 * @param instance instance of i2c controller
 * @param slave_addr address of i2c slave
 * @param slave_reg internal register offset
 * @param buf buffer containing data to be written
 * @param buf_size number of bytes to be written to slave
 *
 * @return zero if no error else negative value
 */
status_t i2c_write(uint8_t instance,
				   uint32_t slave_addr,
				   uint8_t slave_reg,
				   uint8_t *buf,
				   uint32_t buf_size);

/**
 * @brief Write number of bytes to slave connected to
 * specified i2c controller with flexible reg addressing
 * and flexible value length per register
 *
 * @param instance instance of i2c controller
 * @param slave_addr address of i2c slave
 * @param reg_addr internal register offset
 * @param reg_size addressing length of internal register offset
 * @param byte_per_reg value length per internal register
 * @param buf buffer containing data to be written
 * @param buf_size number of bytes to be written to slave
 *
 * @return zero if no error else negative value
 */
status_t i2c_write_extend(uint8_t instance,
						  uint32_t slave_addr,
						  uint8_t *reg_addr,
						  uint32_t reg_size,
						  uint32_t byte_per_reg,
						  uint8_t *buf,
						  uint32_t buf_size);

/**
 * @brief I2C message parameters.
 *
 * @param is_write true if the msg is to be sent to a module from master
 * @param is_repeat_start true if repeat_start should be sent instead of stop after
 *        this msg
 * @param slave_addr address of slave device
 * @param slave_reg register to target with message
 * @param buf pointer to data buffer
 * @param buf_size size in bytes of data buffer
 */
typedef struct {
	bool is_write;
	bool is_repeated_start;

	uint32_t slave_addr;
	uint8_t slave_reg;
	uint8_t *buf;
	uint32_t buf_size;
} i2c_transaction_msg_t;

/**
 * @brief Transfer up to N separate messages to the
 * specified i2c controller with repeated start in between if the caller
 * requests so
 *
 * @param instance instance of i2c controller
 * @param msgs array of messages to be sent
 * @param num_msgs the number of elements in msgs
 *
 * @return zero if no error else negative value
 */
status_t i2c_transaction(uint8_t instance, i2c_transaction_msg_t *msg,
		uint32_t num_msgs);

#endif
