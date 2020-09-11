# Copyright (c) 2015-2016, NVIDIA Corporation. All Rights Reserved.
#
# NVIDIA Corporation and its licensors retain all intellectual property and
# proprietary rights in and to this software and related documentation.  Any
# use, reproduction, disclosure or distribution of this software and related
# documentation without an express license agreement from NVIDIA Corporation
# is strictly prohibited.

LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/menu.c

GLOBAL_INCLUDES += \
	$(LOCAL_DIR)/../../../../common/include/drivers \
	$(LOCAL_DIR)/../../../../common/include/lib \
	$(LOCAL_DIR)/../../../../common/include

include make/module.mk
