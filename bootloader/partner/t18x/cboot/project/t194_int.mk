# top level project rules for the armemu-test project
#
LOCAL_DIR := $(GET_LOCAL_DIR)

TARGET := t194
TARGET_FAMILY := t19x

IS_A64_MODE := 1

MODULES += \
	app/kernel_boot

# Disable shell to improve boot time kpis
#MODULES += \
	app/cboot-tests \
	app/shell \
	app/mem_test
