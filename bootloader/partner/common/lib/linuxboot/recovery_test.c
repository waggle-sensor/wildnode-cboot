/**
 * TOOD
 */

#define MODULE  TEGRABL_ERR_LINUXBOOT

// TODO: filter these down
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <fs.h>
#include <tegrabl_debug.h>
#include <tegrabl_error.h>
#include <tegrabl_malloc.h>
#include <tegrabl_file_manager.h>
#include <tegrabl_linuxboot_helper.h>
#include <tegrabl_linuxboot_utils.h>
#include <tegrabl_sdram_usage.h>
#include <tegrabl_devicetree.h>
#include <tegrabl_linuxboot.h>
#include <tegrabl_binary_types.h>
#include <tegrabl_fuse.h>
#include <recovery_test.h>
#include <linux_load.h>
#include <tegrabl_auth.h>

#define REC_FAIL_PATH					"/fail_media"
#define REC_VALID_PATH				"/valid_media"
#define REC_INVALID_CNT_PATH	"/invalid_count"
#define REC_FILE_MAX_SIZE			4096UL
#define REC_INVALID_MAX				10

// JOE add details, rename and move to a more appropriate file (maybe my own file)
tegrabl_error_t recovery_test(struct tegrabl_fm_handle *fm_handle)
{
	void *file_load_addr = NULL;
	uint32_t file_size;
	tegrabl_error_t err = TEGRABL_NO_ERROR;
	uint8_t invalid_count = 0;

	pr_info("%s(): %u\n", __func__, __LINE__);

	if ((fm_handle == NULL) || (fm_handle->mount_path == NULL)) {
		pr_error("Invalid fm_handle (%p) or mount path passed\n", fm_handle);
		err = TEGRABL_ERROR(TEGRABL_ERR_INVALID, 0);
		goto fail;
	}

	/* Allocate space for files */
	file_load_addr = tegrabl_malloc(REC_FILE_MAX_SIZE);
	if (file_load_addr == NULL) {
		pr_error("Failed to allocate memory\n");
		err = TEGRABL_ERROR(TEGRABL_ERR_NO_MEMORY, 0);
		goto fail;
	}

	/* Test for the existance of the immediate fail flag file first */
	pr_info("Test for fail media flag %s\n", REC_FAIL_PATH);
	file_size = REC_FILE_MAX_SIZE;
	err = tegrabl_fm_read(fm_handle, REC_FAIL_PATH, NULL, file_load_addr, &file_size, NULL);
	if (TEGRABL_ERROR_REASON(err) == TEGRABL_ERR_OPEN_FAILED) {
		pr_info("Fail media flag %s NOT found, proceed\n", REC_FAIL_PATH);
	} else if (err == TEGRABL_NO_ERROR) {
		pr_info("Fail media flag %s WAS FOUND, reject booting from this media\n", REC_FAIL_PATH);
		err = TEGRABL_ERROR(TEGRABL_ERR_MEDIA_FAILED, 0x0);
		goto fail;
	} else {
		pr_error("Failed testing fail media flag file %s\n", REC_FAIL_PATH);
		goto fail;
	}

	/* Load the current failure count */
	// load the value, if file is present, else set to 0
	pr_info("Load the current failure count %s\n", REC_INVALID_CNT_PATH);
	file_size = REC_FILE_MAX_SIZE;
	err = tegrabl_fm_read(fm_handle, REC_INVALID_CNT_PATH, NULL, file_load_addr, &file_size, NULL);
	if (err != TEGRABL_NO_ERROR) {
		// TODO: can get rid of this code, assumed 0 from init above
		pr_info("Unable to load failure count %s, assume failure count of 0.\n", REC_INVALID_CNT_PATH);
		invalid_count = 0;
	} else {
		// TODO use the file_size to convert the buffer, do a memcopy to an int or something with the filesize
		// invalid_count = atoi(file_load_addr);
		// memcpy(&invalid_count, file_load_addr, sizeof(uint8_t));
		invalid_count = *(uint8_t*)file_load_addr;
		pr_info("Fail count %d loaded from file %s\n", invalid_count, REC_INVALID_CNT_PATH);
	}

	/* Test for the existance of the valid media flag file, if present clear and boot */
	pr_info("Test for valid media flag %s\n", REC_VALID_PATH);
	file_size = REC_FILE_MAX_SIZE;
	err = tegrabl_fm_read(fm_handle, REC_VALID_PATH, NULL, file_load_addr, &file_size, NULL);
	if (err == TEGRABL_NO_ERROR) {
		pr_info("Valid media flag %s present, clear and proceed\n", REC_VALID_PATH);
		err = tegrabl_fm_remove(fm_handle, REC_VALID_PATH);
		if (err != TEGRABL_NO_ERROR) { // TODO: option to format mount??
			pr_warn("Unable to remove valid media flag %s, %u.\n", REC_VALID_PATH, err);
			TEGRABL_ERROR_PRINT(err);
		}
	} else if (TEGRABL_ERROR_REASON(err) == TEGRABL_ERR_OPEN_FAILED) {
		invalid_count = invalid_count + 1;
		pr_info("Valid media flag %s is missing, increment fail counter %d\n", REC_VALID_PATH, invalid_count);
		err = TEGRABL_NO_ERROR;
	} else {
		pr_error("Failed testing valid media flag file %s\n", REC_VALID_PATH);
		goto fail;
	}

	/* record the failure count */ // TODO  assume the file exists
	err = tegrabl_fm_write(fm_handle, REC_INVALID_CNT_PATH, &invalid_count, sizeof(uint8_t));
	if (err != TEGRABL_NO_ERROR) {
		pr_error("Unable to save failure count %d to failure count file %s\n", invalid_count, REC_INVALID_CNT_PATH);
		TEGRABL_ERROR_PRINT(err);
		goto fail;
	} else {
		pr_info("Successfully wrote failure count %d to failure count file %s\n", invalid_count, REC_INVALID_CNT_PATH);
	}

	/* test fail counter for threshold */
	if (invalid_count > REC_INVALID_MAX) {
		pr_error("Failure count %d has exceeded max available %d, reject booting from this media\n", invalid_count, REC_INVALID_MAX);
		err = TEGRABL_ERROR(TEGRABL_ERR_MEDIA_FAILED, 0x1);
		goto fail;
	} else if (invalid_count > 0) {
		pr_warn("Failure count %d > 0. %d attempts remaining.\n", invalid_count, REC_INVALID_MAX - invalid_count);
	}

fail:
	if (file_load_addr != NULL) {
		tegrabl_free(file_load_addr);
	}
	TEGRABL_ERROR_PRINT(err);
	// return err;
	return TEGRABL_NO_ERROR;
}
