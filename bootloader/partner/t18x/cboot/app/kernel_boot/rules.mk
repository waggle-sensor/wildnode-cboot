#
# Copyright (c) 2016-2018, NVIDIA Corporation.  All Rights Reserved.
#
# NVIDIA Corporation and its licensors retain all intellectual property and
# proprietary rights in and to this software and related documentation.  Any
# use, reproduction, disclosure or distribution of this software and related
# documentation without an express license agreement from NVIDIA Corporation
# is strictly prohibited.
#

LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_DEPS += \
	$(LOCAL_DIR)/../../../../common/drivers/uart \
	$(LOCAL_DIR)/../../../../common/drivers/wdt \
	$(LOCAL_DIR)/../../../../common/lib/console \
	$(LOCAL_DIR)/../../../../common/lib/debug \
	$(LOCAL_DIR)/../../../../common/lib/malloc \
	$(LOCAL_DIR)/../../../../common/lib/gpt \
	$(LOCAL_DIR)/../../../../common/lib/utils \
	$(LOCAL_DIR)/../../../../common/lib/partition_manager \
	$(LOCAL_DIR)/../../../../common/lib/fastboot \
	$(LOCAL_DIR)/../../../../common/lib/nvblob \
	$(LOCAL_DIR)/../../../../common/lib/nvblob_bmp \
	$(LOCAL_DIR)/../../../../common/lib/frp \
	$(LOCAL_DIR)/../../../../common/lib/bootloader_update \
	$(LOCAL_DIR)/../../../../common/lib/linuxboot

GLOBAL_INCLUDES += \
	$(LOCAL_DIR) \
	../../common/include \
	../../common/include/drivers \
	../../common/include/drivers/display

ifeq ($(PLATFORM_IS_AFTER_N),1)
# verified boot 2.0
MODULE_DEPS += $(LOCAL_DIR)/verified_boot/vblib_v2
GLOBAL_INCLUDES += $(LOCAL_DIR)/verified_boot/vblib_v2
else
MODULE_DEPS += $(LOCAL_DIR)/verified_boot/vblib_v1
GLOBAL_INCLUDES += $(LOCAL_DIR)/verified_boot/vblib_v1
endif

MODULE_SRCS += \
	$(LOCAL_DIR)/kernel_boot.c \
	$(LOCAL_DIR)/fastboot_a_b.c \
	$(LOCAL_DIR)/fastboot.c \
	$(LOCAL_DIR)/fastboot_menu.c

ifneq ($(filter t18x, $(TARGET_FAMILY)),)
MODULE_SRCS += \
	$(LOCAL_DIR)/android_boot_menu.c
endif

include make/module.mk
