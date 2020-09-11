# Copyright (c) 2018, NVIDIA CORPORATION. All rights reserved.
#
# NVIDIA Corporation and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA Corporation is strictly prohibited.

LOCAL_DIR := $(GET_LOCAL_DIR)

PLATFORM := t194

#For now, we'll show only 1GB memory to LK
MEMSIZE 	:= 0x40000000	# 1GB

#disabling static heap for now as the bootloader is already being loaded @ 0x83D80000
GLOBAL_DEFINES += \
	WITH_STATIC_HEAP=0 \
	START_ON_BOOT=1 \
	ENABLE_NVDUMPER=1 \
	ANDROID=1

# FASTBOOT_PRODUCT name should match the product name in
# $TOP/device/nvidia-t18x/t186/board-info.txt
GLOBAL_DEFINES += FASTBOOT_PRODUCT=\"$(TARGET_PRODUCT)\"
