#
# Copyright (c) 2017-2019, NVIDIA CORPORATION.  All rights reserved.
#
# NVIDIA CORPORATION and its licensors retain all intellectual property
# and proprietary rights in and to this software and related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA CORPORATION is strictly prohibited.
#

# Add any needed GLOBAL_DEFINES here
GLOBAL_DEFINES += \
	CONFIG_OS_IS_L4T=1 \
	CONFIG_ENABLE_BOOT_DEVICE_SELECT=1 \
	CONFIG_ENABLE_SDCARD=1 \
	CONFIG_ENABLE_USB_MS=1 \
	CONFIG_ENABLE_USB_SD_BOOT=1 \
	CONFIG_ENABLE_ETHERNET_BOOT=1 \
	CONFIG_ENABLE_SECURE_BOOT=1 \
	CONFIG_ENABLE_DISPLAY=1 \
	CONFIG_ENABLE_SHELL=1 \
	CONFIG_ENABLE_L4T_RECOVERY=1 \
	CONFIG_ENABLE_EXTLINUX_BOOT=1

MODULE_DEPS +=	\
	lib/lwip \
	lib/console

MODULE_DEPS += \
	$(LOCAL_DIR)/../../../../common/drivers/eqos \
	$(LOCAL_DIR)/../../../../common/drivers/phy
