#
# Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
#
# NVIDIA CORPORATION and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA CORPORATION is strictly prohibited.
#

LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

GLOBAL_DEFINES += \
	ARM64_CPU_$(ARM_CPU)=1 \
	ARM_ISA_ARMV8=1 \
	ARM64_WITH_EL2=1 \
	ARM_WITH_CACHE=1 \
	WITH_MMU=1

GLOBAL_INCLUDES += \
	$(LOCAL_DIR)/include

MODULE_SRCS += \
	$(LOCAL_DIR)/arch.c \
	$(LOCAL_DIR)/asm.S \
	$(LOCAL_DIR)/exceptions.S \
	$(LOCAL_DIR)/exceptions_c.c \
	$(LOCAL_DIR)/thread.c \
	$(LOCAL_DIR)/start.S \
	$(LOCAL_DIR)/header.S \
	$(LOCAL_DIR)/cpuinfo.c \
	$(LOCAL_DIR)/mmu.c \
	$(LOCAL_DIR)/stacktrace.c \
	$(LOCAL_DIR)/cache-ops.S \
	$(LOCAL_DIR)/dmamap.c \
	$(LOCAL_DIR)/../../../../$(TARGET_FAMILY)/nvtboot/cpu/soc/$(TARGET)/tz_init.c

GLOBAL_DEFINES += \
	ARCH_DEFAULT_STACK_SIZE=8192

# try to find the toolchain
ifndef TOOLCHAIN_PREFIX
TOOLCHAIN_PREFIX := aarch64-elf-
endif

FOUNDTOOL=$(shell which $(TOOLCHAIN_PREFIX)gcc)
ifeq ($(FOUNDTOOL),)
$(error cannot find toolchain, please set TOOLCHAIN_PREFIX or add it to your path)
endif
$(info TOOLCHAIN_PREFIX = $(TOOLCHAIN_PREFIX))

# make sure some bits were set up
MEMVARS_SET := 0
ifneq ($(MEMBASE),)
MEMVARS_SET := 1
endif
ifneq ($(MEMSIZE),)
MEMVARS_SET := 1
endif
ifeq ($(MEMVARS_SET),0)
$(error missing MEMBASE or MEMSIZE variable, please set in target rules.mk)
endif

LIBGCC := $(shell $(TOOLCHAIN_PREFIX)gcc $(GLOBAL_COMPILEFLAGS) $(THUMBCFLAGS) -print-libgcc-file-name)
$(info LIBGCC = $(LIBGCC))
$(info GLOBAL_COMPILEFLAGS = $(GLOBAL_COMPILEFLAGS) $(THUMBCFLAGS))

# potentially generated files that should be cleaned out with clean make rule
GENERATED += \
	$(BUILDDIR)/system-onesegment.ld

# rules for generating the linker script
$(BUILDDIR)/system-onesegment.ld: $(LOCAL_DIR)/system-onesegment.ld $(wildcard arch/*.ld)
	@echo generating $@
	@$(MKDIR)
	$(NOECHO)sed "s/%MEMBASE%/$(MEMBASE)/;s/%MEMSIZE%/$(MEMSIZE)/" < $< > $@

include make/module.mk
