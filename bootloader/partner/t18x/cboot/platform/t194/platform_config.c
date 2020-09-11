/*
 * Copyright (c) 2017-2018, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited
 */

#include <string.h>
#include <tegrabl_debug.h>
#include <tegrabl_error.h>
#include <tegrabl_timer.h>
#include <tegrabl_devicetree.h>
#include <libfdt.h>
#include <tegrabl_i2c_dev_basic.h>
#include <platform_config.h>
#include <tegrabl_io.h>

#define ALIAS_NAME_LEN 50

/**
 * @brief Applies i2c type platform config from dtb
 *
 * @param fdt Pointer to dt
 * @param node block of type i2c
 * @param cmd_delay delay after each command
 */
static void process_i2c_config(void *fdt, int32_t node, uint32_t cmd_delay)
{
	const uint32_t *prop_value = NULL;
	const void *node_value = NULL;
	const void *prop = NULL;
	int32_t value_len = 0;
	int32_t phandle = 0;
	int32_t i2c_nodeoffset = 0;
	uint32_t i = 0;
	char i2c_nodename[ALIAS_NAME_LEN];
	char i2c_alias[ALIAS_NAME_LEN];
	int32_t i2c_instance = 0;
	uint32_t slave_addr = 0;
	uint16_t reg_addr;
	bool is_addr_16_bit = false;
	bool is_data_16_bit = false;
	uint16_t mask = 0;
	uint16_t value = 0;
	tegrabl_error_t err = TEGRABL_NO_ERROR;
	uint16_t data;

	pr_debug("Applying i2c configs\n");
	node_value = fdt_getprop(fdt, node, "controller", &value_len);
	if (node_value == NULL) {
		pr_warn("%s: Cannot find controller property\n", __func__);
		return;
	}
	phandle = fdt32_to_cpu(*(uint32_t *)node_value);

	i2c_nodeoffset = fdt_node_offset_by_phandle(fdt, phandle);
	if (i2c_nodeoffset <= 0) {
		pr_warn("%s: Cannot get the i2c nodeoffset\n", __func__);
		return;
	}

	/* Get the i2c bus's path via node offset */
	err = fdt_get_path(fdt, i2c_nodeoffset, i2c_nodename, ALIAS_NAME_LEN);
	if (err != TEGRABL_NO_ERROR) {
		pr_warn("%s: Cannot get i2c node path\n", __func__);
		return;
	}

	/* Get the i2c alias name. */
	err = tegrabl_get_alias_by_name(fdt, i2c_nodename, i2c_alias, &value_len);
	if (err != TEGRABL_NO_ERROR) {
		pr_warn("Failed to get alias name\n");
		return;
	}
	/* remove alias' prefix and return the id. */
	/* the real i2c bus should +1 */
	err = tegrabl_get_alias_id("i2c", i2c_alias, &i2c_instance);
	if (err != TEGRABL_NO_ERROR) {
		pr_warn("Failed to get alias id\n");
		return;
	}
	i2c_instance += 1;
	pr_debug("I2C instance is %d\n", i2c_instance);

	node_value = fdt_getprop(fdt, node, "device-address", &value_len);
	if (node_value == NULL) {
		pr_warn("%s: Cannot get device address\n", __func__);
		return;
	}
	slave_addr = fdt32_to_cpu(*(uint32_t *)node_value);
	/* Slave address in dtb is 7 bit address. Convert it to 8 bit */
	slave_addr = slave_addr << 1;
	pr_debug("Slave address is 0x%02x\n", slave_addr);

	prop = fdt_get_property(fdt, node, "enable-16bit-data", &value_len);
	if (prop != NULL) {
		is_data_16_bit = true;
	}

	prop = fdt_get_property(fdt, node, "enable-16bit-register", &value_len);
	if (prop != NULL) {
		is_addr_16_bit = true;
	}

	prop_value = (uint32_t *) fdt_getprop(fdt, node, "commands", &value_len);
	if (prop_value == NULL) {
		return;
	}

	for (i  = 0; i < (value_len / sizeof(uint32_t)); i += 3) {
		reg_addr = fdt32_to_cpu(prop_value[i]);
		mask = fdt32_to_cpu(prop_value[i + 1]);
		value = fdt32_to_cpu(prop_value[i + 2]);

		pr_debug("Offset: 0x%04x, Mask: 0x%04x, Value: 0x%04x\n", reg_addr, mask, value);

		err = tegrabl_i2c_dev_basic_read(i2c_instance, slave_addr, is_addr_16_bit, is_data_16_bit,
										 reg_addr, &data);
		if (err != TEGRABL_NO_ERROR) {
			pr_warn("%s: Failed to read data from slave 0x%08x at offset 0x%08x\n", __func__,
					slave_addr, reg_addr);
			continue;
		}

		pr_debug("Current data 0x%08x\n", data);
		data = (data & (~mask)) | (value & mask);
		pr_debug("Updated data 0x%08x\n", data);

		err = tegrabl_i2c_dev_basic_write(i2c_instance, slave_addr, is_addr_16_bit, is_data_16_bit,
										  reg_addr, data);
		if (err != TEGRABL_NO_ERROR) {
			pr_warn("%s: Failed to write data from slave 0x%08x at offset 0x%08x\n", __func__,
					slave_addr, reg_addr);
			continue;
		}

		pr_debug("Command delay %d usec\n", cmd_delay);
		tegrabl_udelay(cmd_delay);
	}

	pr_debug("%s: success\n", __func__);
}

/**
 * @brief Applies mmio type platform configuration
 *
 * @param fdt pointer to dt
 * @param node block of type mmio
 * @param cmd_delay delay after each command
 */
static void process_mmio_config(void *fdt, int32_t node, uint32_t cmd_delay)
{
	const uint32_t *prop_value = NULL;
	int32_t value_len = 0;
	uint32_t i = 0;
	uint32_t addr = 0;
	uint32_t mask = 0;
	uint32_t value = 0;
	uint32_t data = 0;

	pr_debug("Applying mmio configs\n");

	prop_value = (uint32_t *) fdt_getprop(fdt, node, "commands", &value_len);
	if (prop_value == NULL) {
		return;
	}

	for (i  = 0; i < (value_len / sizeof(uint32_t)); i += 3) {
		addr = fdt32_to_cpu(prop_value[i]);
		mask = fdt32_to_cpu(prop_value[i + 1]);
		value = fdt32_to_cpu(prop_value[i + 2]);

		pr_debug("Address: 0x%08x, Mask: 0x%08x, Value: 0x%08x\n", addr, mask, value);

		data = NV_READ32(addr);
		pr_debug("Current data 0x%08x\n", data);

		data = (data & (~mask)) | (value & mask);
		pr_debug("Updated data 0x%08x\n", data);
		NV_WRITE32(addr, data);
		pr_debug("Command delay %d usec\n", cmd_delay);
		tegrabl_udelay(cmd_delay);
	}

	pr_debug("%s: success\n", __func__);
}

void platform_config(void)
{
	int32_t len = 0;
	void *fdt;
	const char *node_value = NULL;
	int32_t offset = 0;
	int32_t subnode = 0;
	uint32_t block_delay = 0;
	uint32_t cmd_delay = 0;
	tegrabl_error_t err = TEGRABL_NO_ERROR;

	err = tegrabl_dt_get_fdt_handle(TEGRABL_DT_BL, &fdt);
	if (err != TEGRABL_NO_ERROR) {
		pr_warn("Failed to get bl-dtb handle\n");
		return;
	}

	pr_debug("Checking for compatible node\n");
	offset = fdt_node_offset_by_compatible(fdt, offset, "platform-init");

	if (offset < 0) {
		pr_warn("platform-init is not present. Skipping\n");
		return;
	}

	pr_debug("Checking for bootloader-status\n");
	node_value = fdt_getprop(fdt, offset, "bootloader-status", &len);
	if (node_value == NULL || !strcmp(node_value, "disabled")) {
		pr_warn("platform-init is disabled. Skipping\n");
		return;
	}

	tegrabl_dt_for_each_child(fdt, offset, subnode) {
		node_value = fdt_getprop(fdt, subnode, "block-delay", &len);
		if (node_value != NULL) {
			block_delay = fdt32_to_cpu(*(uint32_t *)node_value);
		} else {
			block_delay = 0;
		}

		node_value = fdt_getprop(fdt, subnode, "command-delay", &len);
		if (node_value != NULL) {
			cmd_delay = fdt32_to_cpu(*(uint32_t *)node_value);
		} else {
			cmd_delay = 0;
		}

		pr_debug("Block delay: %d, Cmd delay: %d\n", block_delay, cmd_delay);

		node_value = fdt_getprop(fdt, subnode, "type", &len);
		if (node_value == NULL) {
			continue;
		} else if (strcmp(node_value, "i2c") == 0) {
			process_i2c_config(fdt, subnode, cmd_delay);
		} else if (strcmp(node_value, "mmio") == 0) {
			process_mmio_config(fdt, subnode, cmd_delay);
		}

		tegrabl_udelay(block_delay);
	}
}
