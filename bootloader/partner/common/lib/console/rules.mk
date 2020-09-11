#
# Copyright (c) 2016, NVIDIA Corporation.  All rights reserved.
#
# NVIDIA Corporation and its licensors retain all intellectual property
# and proprietary rights in and to this software and related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA Corporation is strictly prohibited.
#

LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

GLOBAL_INCLUDES += \
	$(LOCAL_DIR) \
	$(LOCAL_DIR)/../../include \
	$(LOCAL_DIR)/../../include/drivers \
	$(LOCAL_DIR)/../../include/lib

MODULE_SRCS += \
	$(LOCAL_DIR)/tegrabl_console.c

include make/module.mk
