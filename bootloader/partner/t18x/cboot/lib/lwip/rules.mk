#
# Copyright (c) 2018, NVIDIA CORPORATION.  All rights reserved.
#
# NVIDIA CORPORATION and its licensors retain all intellectual property
# and proprietary rights in and to this software and related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA CORPORATION is strictly prohibited.
#
LWIPDIR := $(GET_LOCAL_DIR)
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

GLOBAL_INCLUDES += \
	$(LOCAL_DIR) \
	$(LOCAL_DIR)/include \
	$(LOCAL_DIR)/include/lwip \
	$(LOCAL_DIR)/include/netif \
	$(LOCAL_DIR)/include/posix

# Include Standard LWIP makefile here
include $(LOCAL_DIR)/Filelists.mk

MODULE_SRCS += \
	$(LWIPNOAPPSFILES) \
	$(TFTPCLIENTFILES)

include make/module.mk

