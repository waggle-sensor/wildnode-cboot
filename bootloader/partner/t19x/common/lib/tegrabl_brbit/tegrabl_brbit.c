/*
 * Copyright (c) 2015-2018, NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#define MODULE TEGRABL_ERR_BRBIT

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <tegrabl_utils.h>
#include <tegrabl_debug.h>
#include <tegrabl_brbit.h>
#include <tegrabl_brbit_core.h>
#include <tegrabl_brbit_err_aux.h>

static uint8_t *brptr;

/**
 * @brief Gets the location of BR-BIT and verifies for
 * authenticity and sets brptr.
 *
 * @return TEGRABL_NO_ERROR if successful else appropriate error.
 */
static tegrabl_error_t tegrabl_brbit_open(void)
{
	tegrabl_error_t error = TEGRABL_NO_ERROR;

	if (brptr != NULL) {
		goto fail;
	}

	brptr = (uint8_t *)tegrabl_brbit_location();

	TEGRABL_ASSERT(brptr != NULL);

	pr_trace("%s @ %p\n", "BR-BIT", brptr);

	if (!tegrabl_brbit_verify(brptr)) {
		brptr = NULL;
		error = TEGRABL_ERROR(TEGRABL_ERR_VERIFY_FAILED, TEGRABL_BRBIT_OPEN_1);
		TEGRABL_SET_ERROR_STRING(error, "BR-BIT");
		goto fail;
	}

fail:
	return error;
}

tegrabl_error_t tegrabl_brbit_get_data(tegrabl_brbit_data_type_t type,
		uint32_t instance, void **buffer, uint32_t *buffer_size)
{
	tegrabl_error_t error = TEGRABL_NO_ERROR;
	uint32_t offset = 0;
	uint32_t size = 0;

	if (buffer == NULL) {
		error = TEGRABL_ERROR(TEGRABL_ERR_BAD_PARAMETER, TEGRABL_BRBIT_GET_DATA_1);
		TEGRABL_SET_ERROR_STRING(error, "buffer = %p", buffer);
		goto fail;
	}

	if ((*buffer != NULL) && (buffer_size == NULL)) {
		error = TEGRABL_ERROR(TEGRABL_ERR_BAD_PARAMETER, TEGRABL_BRBIT_GET_DATA_2);
		TEGRABL_SET_ERROR_STRING(error, "*buffer = %p, buffer_size = %p", *buffer, buffer_size);
		goto fail;
	}

	error = tegrabl_brbit_open();
	if (TEGRABL_NO_ERROR != error) {
		TEGRABL_PRINT_ERROR_STRING(TEGRABL_ERR_OPEN_FAILED, "BR-BIT");
		goto fail;
	}

	error = tegrabl_brbit_get_offset_size(type, instance, &offset,
			&size);
	if (TEGRABL_NO_ERROR != error) {
		TEGRABL_PRINT_ERROR_STRING(TEGRABL_ERR_GET_FAILED, "offset & size", "type %d", "BR-BIT", type);
		goto fail;
	}

	/* If input buffer is given then do memcopy else
	 * update the location pointed by input buffer.
	 */
	if (*buffer != NULL) {
		if (*buffer_size < size) {
			error = TEGRABL_ERROR(TEGRABL_ERR_TOO_SMALL, TEGRABL_BRBIT_GET_DATA_1);
			TEGRABL_SET_ERROR_STRING(error, "buffer size %d", "%d", *buffer_size, size);
			goto fail;
		}
		memcpy(*buffer, brptr + offset, size);
	} else {
		*buffer = brptr + offset;
		*buffer_size = size;
	}

fail:
	return error;
}

tegrabl_error_t tegrabl_brbit_set_data(tegrabl_brbit_data_type_t type,
		uint32_t instance, void *buffer, uint32_t buffer_size)
{
	tegrabl_error_t error = TEGRABL_NO_ERROR;
	uint32_t offset = 0;
	uint32_t size = 0;

	if ((buffer == NULL) || (buffer_size == 0U)) {
		error = TEGRABL_ERROR(TEGRABL_ERR_BAD_PARAMETER, TEGRABL_BRBIT_SET_DATA_1);
		TEGRABL_SET_ERROR_STRING(error, "buffer = %p, buffer_size = %d", buffer, buffer_size);
		goto fail;
	}

	error = tegrabl_brbit_open();
	if (TEGRABL_NO_ERROR != error) {
		TEGRABL_PRINT_ERROR_STRING(TEGRABL_ERR_OPEN_FAILED, "BR-BIT");
		goto fail;
	}

	error = tegrabl_brbit_get_offset_size(type, instance, &offset,
			&size);
	if (TEGRABL_NO_ERROR != error) {
		TEGRABL_PRINT_ERROR_STRING(TEGRABL_ERR_GET_FAILED, "offset & size", "type %d", "BR-BIT", type);
		goto fail;
	}

	if (buffer_size > size) {
		error = TEGRABL_ERROR(TEGRABL_ERR_TOO_LARGE, TEGRABL_BRBIT_GET_DATA_1);
		TEGRABL_SET_ERROR_STRING(error, "buffer size %d", "%d", buffer_size, size);
		goto fail;
	}

	memcpy(brptr + offset, buffer, buffer_size);

fail:
	return error;
}

