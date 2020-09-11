#
# Copyright (c) 2017, NVIDIA Corporation.  All Rights Reserved.
#
# NVIDIA Corporation and its licensors retain all intellectual property and
# proprietary rights in and to this software and related documentation.  Any
# use, reproduction, disclosure or distribution of this software and related
# documentation without an express license agreement from NVIDIA Corporation
# is strictly prohibited.


LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

GLOBAL_INCLUDES += \
	$(LOCAL_DIR)/../../include \
	$(LOCAL_DIR)/../../include/lib \
	$(LOCAL_DIR)/../../include/drivers \
	$(LOCAL_DIR)/../../../$(TARGET_FAMILY)/common/include \
	$(LOCAL_DIR)/../../../$(TARGET_FAMILY)/common/include/lib \
	$(LOCAL_DIR)/../../../$(TARGET_FAMILY)/common/include/drivers \
	$(LOCAL_DIR)/../../../$(TARGET_FAMILY)/common/include/soc/$(TARGET)

MODULE_SRCS += \
	$(LOCAL_DIR)/tegrabl_a_b_boot_control.c \
	$(LOCAL_DIR)/tegrabl_a_b_partition_naming.c

include make/module.mk
