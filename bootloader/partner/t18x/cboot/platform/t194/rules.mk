#
# Copyright (c) 2017-2019, NVIDIA CORPORATION.  All rights reserved.
#
# NVIDIA CORPORATION and its licensors retain all intellectual property
# and proprietary rights in and to this software and related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA CORPORATION is strictly prohibited.
#

LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

ARCH := arm64
ARM64_CPU := cortex-a57

include $(LOCAL_DIR)/../../../../../../hwinc-t19x/rules.mk

GLOBAL_INCLUDES += \
	$(LOCAL_DIR) \
	$(LOCAL_DIR)/../../../../common/include/soc/t194 \
	$(LOCAL_DIR)/../../../../common/include/arch \
	$(LOCAL_DIR)/../../../../common/include/drivers \
	$(LOCAL_DIR)/../../../../common/include/drivers/display \
	$(LOCAL_DIR)/../../../../$(TARGET_FAMILY)/common/include/soc/$(TARGET) \
	$(LOCAL_DIR)/../../../../$(TARGET_FAMILY)/common/lib/config_storage \
	$(LOCAL_DIR)/../../../../$(TARGET_FAMILY)/mb1-headers \
	$(LOCAL_DIR)/../../../../t18x/common/include \
	$(LOCAL_DIR)/../../../../$(NV_TARGET_SOC_FAMILY)/common/include/lib \
	$(LOCAL_DIR)/../../../../../../hwinc-$(TARGET_FAMILY)/$(NV_HWINC_T19X_CL)

MODULE_DEPS += \
	platform/tegra_shared \
	lib/exit \
	lib/menu \
	$(LOCAL_DIR)/../../../../$(TARGET_FAMILY)/common/drivers/timer \
	$(LOCAL_DIR)/../../../../$(TARGET_FAMILY)/common/drivers/fuse \
	$(LOCAL_DIR)/../../../../$(TARGET_FAMILY)/common/drivers/padctl \
	$(LOCAL_DIR)/../../../../$(TARGET_FAMILY)/common/lib/tegrabl_brbit \
	$(LOCAL_DIR)/../../../../$(TARGET_FAMILY)/common/lib/tegrabl_brbct \
	$(LOCAL_DIR)/../../../../$(TARGET_FAMILY)/common/lib/tegrabl_auth \
	$(LOCAL_DIR)/../../../../$(TARGET_FAMILY)/common/soc/$(TARGET)/misc \
	$(LOCAL_DIR)/../../../../$(TARGET_FAMILY)/common/soc/$(TARGET)/ccplex_nvg \
	$(LOCAL_DIR)/../../../../$(TARGET_FAMILY)/common/soc/$(TARGET)/ccplex_cache \
	$(LOCAL_DIR)/../../../../$(TARGET_FAMILY)/common/drivers/soc/$(TARGET)/clocks \
	$(LOCAL_DIR)/../../../../$(TARGET_FAMILY)/common/soc/t194/qual_engine \
	$(LOCAL_DIR)/../../../../common/lib/ipc \
	$(LOCAL_DIR)/../../../../common/lib/blockdev \
	$(LOCAL_DIR)/../../../../common/lib/tegrabl_error \
	$(LOCAL_DIR)/../../../../common/drivers/sdmmc \
	$(LOCAL_DIR)/../../../../common/drivers/sata \
	$(LOCAL_DIR)/../../../../common/drivers/ufs \
	$(LOCAL_DIR)/../../../../common/drivers/gpcdma \
	$(LOCAL_DIR)/../../../../common/drivers/qspi \
	$(LOCAL_DIR)/../../../../common/drivers/qspi_flash \
	$(LOCAL_DIR)/../../../../common/drivers/i2c \
	$(LOCAL_DIR)/../../../../common/drivers/i2c_dev \
	$(LOCAL_DIR)/../../../../common/drivers/eeprom \
	$(LOCAL_DIR)/../../../../common/drivers/usbh \
	$(LOCAL_DIR)/../../../../common/drivers/usb/storage \
	$(LOCAL_DIR)/../../../../common/lib/eeprom_manager \
	$(LOCAL_DIR)/../../../../common/arch/arm64 \
	$(LOCAL_DIR)/../../../../t18x/common/lib/mce \
	$(LOCAL_DIR)/../../../../common/lib/psci \
	$(LOCAL_DIR)/../../../../common/lib/exit \
	$(LOCAL_DIR)/../../../../common/drivers/pmic \
	$(LOCAL_DIR)/../../../../common/drivers/pmic/max77620 \
	$(LOCAL_DIR)/../../../../common/drivers/regulator \
	$(LOCAL_DIR)/../../../../common/drivers/gpio \
	$(LOCAL_DIR)/../../../../common/drivers/keyboard \
	$(LOCAL_DIR)/../../../../common/drivers/comb_uart \
	$(LOCAL_DIR)/../../../../common/lib/a_b_boot \
	$(LOCAL_DIR)/../../../../common/drivers/pwm \
	$(LOCAL_DIR)/../../../../common/drivers/display \
	$(LOCAL_DIR)/../../../../common/lib/cbo \
	$(LOCAL_DIR)/../../../../$(TARGET_FAMILY)/common/lib/device_prod

ifeq ($(filter t19x, $(TARGET_FAMILY)),)
MODULE_DEPS += \
	lib/menu
endif

MODULE_SRCS += \
	$(LOCAL_DIR)/platform.c \
	$(LOCAL_DIR)/platform_config.c \
	$(LOCAL_DIR)/../../../../$(TARGET_FAMILY)/common/lib/config_storage/config_storage.c

MEMBASE := 0x96000000
GLOBAL_DEFINES += \
	MEMBASE=$(MEMBASE) \
	IS_A64_MODE=$(IS_A64_MODE) \
	NV_HWINC_T19X_CL=$(NV_HWINC_T19X_CL) \
	CONFIG_SIMULATION=1 \
	AVB_COMPILATION=1 \
	MAX_CPU_CLUSTERS=4 \
	MAX_CPUS_PER_CLUSTER=2 \
	CONFIG_ENABLE_GPT=1 \
	CONFIG_ENABLE_DEBUG=1 \
	CONFIG_ENABLE_UART=1 \
	CONFIG_ENABLE_COMB_UART=1 \
	CONFIG_PAGE_SIZE_LOG2=16 \
	CONFIG_ENABLE_PARTITION_MANAGER=1 \
	CONFIG_DEBUG_TIMESTAMP=1 \
	CONFIG_DT_SUPPORT=1 \
	CONFIG_MULTICORE_SUPPORT=1 \
	CONFIG_ENABLE_EMMC=1 \
	CONFIG_ENABLE_QSPI=1 \
	CONFIG_ENABLE_SATA=1 \
	CONFIG_ENABLE_UFS=1 \
	CONFIG_ENABLE_UFS_HS_MODE=1 \
	CONFIG_ENABLE_UFS_USE_CAR=1 \
	CONFIG_ENABLE_UFS_SKIP_PMC_IMPL=1 \
	CONFIG_ENABLE_NVBLOB=1 \
	CONFIG_WDT_PERIOD_IN_EXECUTION=100 \
	CONFIG_ENABLE_WDT=1 \
	CONFIG_ENABLE_EEPROM=1 \
	CONFIG_ENABLE_PLUGIN_MANAGER=1 \
	CONFIG_ENABLE_I2C=1 \
	CONFIG_POWER_I2C_BPMPFW=1 \
	CONFIG_ENABLE_GPIO=1 \
	CONFIG_ENABLE_GPIO_DT_BASED=1 \
	CONFIG_ENABLE_PMIC_MAX20024=1 \
	CONFIG_ENABLE_A_B_SLOT=1 \
	CONFIG_ENABLE_SDMMC_64_BIT_SUPPORT=1 \
	CONFIG_ENABLE_DPAUX=1 \
	CONFIG_ENABLE_PWM=1 \
	CONFIG_ENABLE_BL_DTB_OVERRIDE=1 \
	CONFIG_ENABLE_DEVICE_PROD=1 \
	CONFIG_ENABLE_STAGED_SCRUBBING=1 \
	CONFIG_ENABLE_WAR_CBOOT_STAGED_SCRUBBING=1 \
	CONFIG_SKIP_GPCDMA_RESET=1 \
	CONFIG_DEBUG_LOGLEVEL=TEGRABL_LOG_INFO

ALLMODULE_OBJS += $(LOCAL_DIR)/../../../../t19x/common/drivers/se/prebuilt/se.mod.o

# Move optional CONFIG items into sub-make files
ifeq ($(NV_BUILD_SYSTEM_TYPE),l4t)
	include $(LOCAL_DIR)/l4t.mk
else ifeq ($(NV_BUILD_SYSTEM_TYPE),mods)
	include $(LOCAL_DIR)/l4t.mk
else
	include $(LOCAL_DIR)/android.mk
endif

GLOBAL_DEFINES += WITH_CPU_EARLY_INIT=1

LINKER_SCRIPT += \
	$(BUILDDIR)/system-onesegment.ld

include make/module.mk
