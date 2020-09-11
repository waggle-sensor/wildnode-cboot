#
# Copyright (c) 2018, NVIDIA Corporation.  All Rights Reserved.
#
# NVIDIA Corporation and its licensors retain all intellectual property and
# proprietary rights in and to this software and related documentation.  Any
# use, reproduction, disclosure or distribution of this software and related
# documentation without an express license agreement from NVIDIA Corporation
# is strictly prohibited.
#

LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

GLOBAL_INCLUDES += \
	$(LOCAL_DIR)/ \
    $(LOCAL_DIR)/../../../../../common/include \
    $(LOCAL_DIR)/../../../../../common/include/lib \
    $(LOCAL_DIR)/../../../../../common/include/drivers \
    $(LOCAL_DIR)/../../../../../common/include/soc/$(TARGET) \
    $(LOCAL_DIR)/../../../../../$(TARGET_FAMILY)/common/include/soc/$(TARGET) \
	$(LOCAL_DIR)/../../../../../../../core/include

ifneq ($(filter t19x, $(TARGET_FAMILY)),)
ALLMODULE_OBJS += $(LOCAL_DIR)/prebuilt/qual_engine_priv.o
endif

MODULE_SRCS += \
	$(LOCAL_DIR)/qual_engine.c

include make/module.mk

