/*
 * Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#ifndef INCLUDED_TEGRABL_SPI_H
#define INCLUDED_TEGRABL_SPI_H

#include <stdint.h>
#include <tegrabl_error.h>

/**
 * @brief spi context structure
 */
struct tegrabl_spi {
	uint8_t instance;
	uint32_t base_addr;
	uint32_t freq_khz;
};

/**
 * @brief spi transfer context structure
 */
struct tegrabl_spi_transfer {
	uint8_t *tx_buf;
	uint8_t *rx_buf;
	uint32_t write_len;
	uint32_t read_len;
};

/**
* @brief spi data endianes
*/
struct tegrabl_spi_endianess {
	bool is_lsbyte_first;
	bool is_lsbit_first;
};

/**
* @brief spi ioctl set configuration
*/
/* macro spi ioctl set */
typedef uint32_t tegrabl_spi_ioctl_t;
#define SPI_IOCTL_SET_ENDIANESS 0
#define SPI_IOCTL_SET_MODE 1
#define SPI_IOCTL_SET_FREQ 2

/**
* @brief spi modes 0,1,2,3
*/
/* macro spi mode */
typedef uint32_t tegrabl_spi_mode_t;
#define SPI_MODE0 0
#define SPI_MODE1 1
#define SPI_MODE2 2
#define SPI_MODE3 3


/**
* @brief spi instance
*/
/* macro tegrabl spi instance */
typedef uint32_t tegrabl_spi_instance_t;
#define TEGRABL_SPI_INSTANCE_1 0
#define TEGRABL_SPI_INSTANCE_2 1
#define TEGRABL_SPI_INSTANCE_3 2
#define TEGRABL_SPI_INSTANCE_4 3
#define TEGRABL_SPI_INSTANCE_COUNT 4

/**
 * @brief Initializes the spi controller
 *
 * @param instance Instance of the spi controller.
 *
 * @param frequency frequency of the spi clock in KHz.
 *
 * @return Returns handle of the spi, if success. Null in cases of failure.
 */
struct tegrabl_spi *tegrabl_spi_open(uint8_t instance, uint32_t freq_khz);

/**
 * @brief Performs given spi transactions write and read
 *
 * @param hspi Handle of the spi.
 *
 * @param transfers address of array of spi transfers
 *
 * @param no_of_transfers Number of transfers.
 *
 * @return TEGRABL_NO_ERROR if success. Error code in case of failure.
 */
tegrabl_error_t tegrabl_spi_transaction(struct tegrabl_spi *hspi,
	struct tegrabl_spi_transfer *transfers,	uint8_t no_of_transfers);

/**
* @brief Sets the given ioctl
*
* @param hspi Handle of the spi.
* @param ioctl Ioctl to set.
* @param args Arguments for the ioctl(input or output based on ioctl)
*
* @return TEGRABL_NO_ERROR if success, error code if fails.
*/
tegrabl_error_t tegrabl_spi_ioctl(struct tegrabl_spi *hspi,
	tegrabl_spi_ioctl_t ioctl, void *args);

/**
 * @brief Register prod settings for particular instance.
 *
 * @param instance SPI controller instance
 * @param mode Transfer speed mode
 * @param settings Pointer to buffer containing <address, mask, value> triplets
 * @param num_settings Number of triplets in buffer
 *
 */
tegrabl_error_t tegrabl_spi_register_prod_settings(uint32_t instance,
		uint32_t mode, uint32_t *settings, uint32_t num_settings);
/**
 * @brief Disables the spi controller
 *
 * @param hspi Handle of the spi.
 *
 * @return TEGRABL_NO_ERROR if success. Error code in case of failure.
 */
tegrabl_error_t tegrabl_spi_close(struct tegrabl_spi *hspi);

#endif /* #ifndef INCLUDED_TEGRABL_SPI_H */
