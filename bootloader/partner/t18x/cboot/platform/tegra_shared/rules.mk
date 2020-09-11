# Copyright (c) 2014-2017, NVIDIA CORPORATION. All rights reserved.
#
# NVIDIA Corporation and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA Corporation is strictly prohibited.

LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_DEPS += \

GLOBAL_INCLUDES += \
	$(LOCAL_DIR)/include \
	$(LOCAL_DIR)/../$(TARGET)/include

MODULE_SRCS += \
	$(LOCAL_DIR)/cpu_early_init.c \
	$(LOCAL_DIR)/interrupts.c \
	$(LOCAL_DIR)/timer.c \
	$(LOCAL_DIR)/debug.c

include make/module.mk
