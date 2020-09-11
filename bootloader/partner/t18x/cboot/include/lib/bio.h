/*
 * Copyright (c) 2009 Travis Geiselbrecht
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
/*
 * Copyright (c) 2014-2016, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited
 */


#ifndef __LIB_BIO_H
#define __LIB_BIO_H

#include <sys/types.h>
#include <list.h>
#include <kernel/mutex.h>

typedef long long off_t;
typedef uint32_t bnum_t;

struct bdev_struct {
	struct list_node list;
	mutex_t lock;
};

typedef struct bdev {
	struct list_node node;
	volatile int ref;
	bool published;

	/* info about the block device */
	char *name;
	size_t size;
	size_t block_size;
	bnum_t block_count;

	lk_time_t last_read_start_time;
	lk_time_t last_read_end_time;
	lk_time_t last_write_start_time;
	lk_time_t last_write_end_time;
	lk_time_t total_read_time;
	lk_time_t total_write_time;
	ssize_t total_read_size;
	ssize_t total_write_size;

	/* driver specific private data */
	void *priv_data;

	/* function pointers */
	ssize_t (*read)(struct bdev *, void *buf, off_t offset, size_t len);
	ssize_t (*read_block)(struct bdev *, void *buf, bnum_t block, uint count);
	ssize_t (*write)(struct bdev *, const void *buf, off_t offset, size_t len);
	ssize_t (*write_block)(struct bdev *, const void *buf, bnum_t block, uint count);
	ssize_t (*erase)(struct bdev *, bnum_t block, uint count);
	int (*ioctl)(struct bdev *, int request, void *argp);
	void (*close)(struct bdev *);
} bdev_t;

/* user api */
bdev_t *bio_open(const char *name);
void bio_close(bdev_t *dev);
ssize_t bio_read(bdev_t *dev, void *buf, off_t offset, size_t len);
ssize_t bio_read_block(bdev_t *dev, void *buf, bnum_t block, uint count);
ssize_t bio_write(bdev_t *dev, const void *buf, off_t offset, size_t len);
ssize_t bio_write_block(bdev_t *dev, const void *buf, bnum_t block, uint count);
ssize_t bio_erase(bdev_t *dev, bnum_t block, uint count);
ssize_t bio_erase_all(bdev_t *dev);
int bio_ioctl(bdev_t *dev, int request, void *argp);
void bio_list_kpi(void);

/* register a block device */
status_t bio_register_device(bdev_t *dev);
void bio_unregister_device(bdev_t *dev);

/* used during bdev construction */
void bio_initialize_bdev(bdev_t *dev, const char *name, size_t block_size, bnum_t block_count);

/* debug stuff */
void bio_dump_devices(void);

/* remove multiple bdevs with given name prefixes */
int bio_remove_by_prefix(char *device_prefix);

/* subdevice support */
status_t bio_publish_subdevice(const char *parent_dev, const char *subdev, bnum_t startblock, size_t len);

/* memory based block device */
int create_membdev(const char *name, void *ptr, size_t len);

#endif

