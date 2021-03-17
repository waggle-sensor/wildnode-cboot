/*
 * Copyright (c) 2019-2020, NVIDIA CORPORATION.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 *
 */

/**
 * @file tegrabl_pcie_soc.c
 */

#include <string.h>
#include <tegrabl_io.h>
#include <tegrabl_clock.h>
#include <tegrabl_timer.h>
#include <tegrabl_pcie.h>
#include <address_map_new.h>
#include <tegrabl_pcie_soc_local.h>
#include <powergate-t194.h>
#include <libfdt.h>
#include <tegrabl_devicetree.h>
#include <tegrabl_regulator.h>
#include <tegrabl_gpio.h>
#include <tegrabl_utils.h>
#include <tegrabl_malloc.h>
#include <tegrabl_error.h>

/**
 * @brief Maximum number of PCIe controllers supported by a chip
 */
#define MAX_CTRL_SUPPORTED	6
uint32_t max_ctrl_supported = MAX_CTRL_SUPPORTED;

#define BIT(nr)			(1UL << (nr))

/*
 * Create a contiguous 32-bit bitmask starting at bit position @l and
 * and ending at position @h.
 *  For example, GENMASK(29, 21) gives us the 32bit vector 0x3fe00000.
 */
#define GENMASK(h, l) ((0xffffffff - (1 << l) + 1) & (0xffffffff >> (31 - h)))

#define APPL_PINMUX								0x0
#define APPL_PINMUX_PEX_RST						BIT(0)
#define APPL_PINMUX_CLKREQ_OVERRIDE_EN			BIT(2)
#define APPL_PINMUX_CLKREQ_OVERRIDE				BIT(3)

#define APPL_CTRL								0x4
#define APPL_CTRL_SYS_PRE_DET_STATE				BIT(6)
#define APP_LTSSM_ENABLE						BIT(7)

#define APPL_DM_TYPE							0x100
#define APPL_DM_TYPE_MASK						GENMASK(3, 0)
#define APPL_DM_TYPE_RP							0x4
#define APPL_DM_TYPE_EP							0x0

#define APPL_CFG_BASE_ADDR						0x104
#define APPL_CFG_BASE_ADDR_MASK					GENMASK(31, 12)

#define APPL_CFG_IATU_DMA_BASE_ADDR				0x108
#define APPL_CFG_IATU_DMA_BASE_ADDR_MASK		GENMASK(31, 18)

#define APPL_CFG_MISC							0x110
#define APPL_CFG_MISC_SLV_EP_MODE				BIT(14)
#define APPL_CFG_MISC_ARCACHE_MASK				GENMASK(13, 10)
#define APPL_CFG_MISC_ARCACHE_SHIFT				10
#define APPL_CFG_MISC_ARCACHE_VAL				3

#define APPL_CFG_SLCG_OVERRIDE					0x114

#define APPL_LINK_STATUS						0xcc
#define RDLH_LINK_UP							BIT(0)
#define RDLH_LINK_UP_MASK						GENMASK(0, 0)

#define APPL_DEBUG								0xd0
#define SMLH_LTSSM_STATE_MASK					GENMASK(8, 3)
#define SMLH_LTSSM_STATE_SHIFT					3

#define PORT_LOGIC_AUX_CLK_FREQ_OFF				0xb40
#define AUX_CLK_FREQ_MASK						GENMASK(9, 0)
#define AUX_CLK_FREQ_SHIFT						0
#define AUX_CLK_FREQ_VAL						19

#define PCIE_CAP_LINK_CONTROL2_LINK_STATUS2_REG	0xa0
#define PCIE_CAP_TARGET_LINK_SPEED_MASK			GENMASK(3, 0)
#define PCIE_CAP_TARGET_LINK_SPEED_SHIFT		0

#define PORT_LOGIC_GEN2_CTRL_OFF				0x80c
#define NUM_OF_LANES_MASK						GENMASK(12, 8)
#define NUM_OF_LANES_SHIFT						8

#define PORT_LOGIC_PORT_LINK_CTRL_OFF			0x710
#define LINK_CAPABLE_MASK						GENMASK(21, 16)
#define LINK_CAPABLE_SHIFT						16

#define PCIE_CAP_LINK_CONTROL_LINK_STATUS_REG	0x80
#define PCIE_CAP_LINK_DISABLE					BIT(4)
#define PCIE_CAP_LINK_DISABLE_MASK				GENMASK(4, 4)
#define PCIE_CAP_LINK_DISABLE_SHIFT				4

#define P2U_CONTROL_GEN1									0x78
#define P2U_CONTROL_GEN1_ENABLE_RXIDLE_ENTRY_ON_LINK_STATUS	BIT(2)
#define P2U_CONTROL_GEN1_ENABLE_RXIDLE_ENTRY_ON_EIOS		BIT(3)

#define P2U_PERIODIC_EQ_CTRL_GEN3							0xc0
#define P2U_PERIODIC_EQ_CTRL_GEN3_PERIODIC_EQ_EN			BIT(0)
#define P2U_PERIODIC_EQ_CTRL_GEN3_INIT_PRESET_EQ_TRAIN_EN	BIT(1)
#define P2U_PERIODIC_EQ_CTRL_GEN4							0xc4
#define P2U_PERIODIC_EQ_CTRL_GEN4_INIT_PRESET_EQ_TRAIN_EN	BIT(1)

#define P2U_RX_DEBOUNCE_TIME								0xa4
#define P2U_RX_DEBOUNCE_TIME_DEBOUNCE_TIMER_MASK			0xffff
#define P2U_RX_DEBOUNCE_TIME_DEBOUNCE_TIMER_VAL				160

#define PADCTL_PEX_CTL_2_PEX_L5_RST							0x08
#define PADCTL_PEX_RST_E_INPUT								BIT(6)

/**
 * @brief Definition of the register base address inside each PCIe controller.
 *
 * - DBI: Data Bus Interface
 * - IATU: Internal Address Translation Unit
 * - IO: IO Address to map endpoint BAR
 * - Memory: Memory Address to map endpoint BAR
 *
 * NV_ADDRESS_MAP_PCIE_C0_32BIT_RP_BASE: DBI Base Address with size of 256 KB
 *   - 0x40000     Offset of iATU DMA base with size of 256 KB
 *   - 0x100000    Offset of IO base address with size of 1 MB
 *   - 0x200000    Offset of Memory base address with size of 30 MB
 */

/** DBI registers offsets */
static uint32_t dbi_reg_offset[] = {
	NV_ADDRESS_MAP_PCIE_C0_32BIT_RP_BASE,
	NV_ADDRESS_MAP_PCIE_C1_32BIT_RP_BASE,
	NV_ADDRESS_MAP_PCIE_C2_32BIT_RP_BASE,
	NV_ADDRESS_MAP_PCIE_C3_32BIT_RP_BASE,
	NV_ADDRESS_MAP_PCIE_C4_32BIT_RP_BASE,
	NV_ADDRESS_MAP_PCIE_C5_32BIT_RP_BASE,
};

/** DMA registers offsets */
static uint32_t iatu_dma_offset[] = {
	NV_ADDRESS_MAP_PCIE_C0_32BIT_DMA_BASE,
	NV_ADDRESS_MAP_PCIE_C1_32BIT_DMA_BASE,
	NV_ADDRESS_MAP_PCIE_C2_32BIT_DMA_BASE,
	NV_ADDRESS_MAP_PCIE_C3_32BIT_DMA_BASE,
	NV_ADDRESS_MAP_PCIE_C4_32BIT_DMA_BASE,
	NV_ADDRESS_MAP_PCIE_C5_32BIT_DMA_BASE,
};

/** IO map offsets */
static uint32_t pcie_io_base[] = {
	NV_ADDRESS_MAP_PCIE_C0_32BIT_RP_BASE + PCIE_IO_OFFSET,
	NV_ADDRESS_MAP_PCIE_C1_32BIT_RP_BASE + PCIE_IO_OFFSET,
	NV_ADDRESS_MAP_PCIE_C2_32BIT_RP_BASE + PCIE_IO_OFFSET,
	NV_ADDRESS_MAP_PCIE_C3_32BIT_RP_BASE + PCIE_IO_OFFSET,
	NV_ADDRESS_MAP_PCIE_C4_32BIT_RP_BASE + PCIE_IO_OFFSET,
	NV_ADDRESS_MAP_PCIE_C5_32BIT_RP_BASE + PCIE_IO_OFFSET,
};

/** Memory map offsets */
static uint32_t pcie_mem_base[] = {
	NV_ADDRESS_MAP_PCIE_C0_32BIT_RP_BASE + PCIE_MEM_OFFSET,
	NV_ADDRESS_MAP_PCIE_C1_32BIT_RP_BASE + PCIE_MEM_OFFSET,
	NV_ADDRESS_MAP_PCIE_C2_32BIT_RP_BASE + PCIE_MEM_OFFSET,
	NV_ADDRESS_MAP_PCIE_C3_32BIT_RP_BASE + PCIE_MEM_OFFSET,
	NV_ADDRESS_MAP_PCIE_C4_32BIT_RP_BASE + PCIE_MEM_OFFSET,
	NV_ADDRESS_MAP_PCIE_C5_32BIT_RP_BASE + PCIE_MEM_OFFSET,
};

#define PCIE_COMPATIBLE "nvidia,tegra194-pcie"
#define PCIE_PHY_COMPATIBLE "nvidia,phy-p2u"
#define PROPERTY_PHYS "phys"
#define PROPERTY_PHANDLE "phandle"
#define PROPERTY_VOLTAGE "regulator-max-microvolt"
#define PROPERTY_PLAT_GPIOS "nvidia,plat-gpios"

static char *pcie_regulator_names[2] = {
	"vpcie3v3-supply",
	"vpcie12v-supply",
};

static int32_t pcie_ctrl_regulator_handles[MAX_CTRL_SUPPORTED][2];

/**
 * @brief API to get DBI base address
 *
 * @retval Pointer to the array of DBI base addresses for each controller
 */
uint32_t *tegrabl_pcie_get_dbi_reg(void)
{
	return &dbi_reg_offset[0];
}

/**
 * @brief API to get iATU base address
 *
 * @retval Pointer to the array of iATU base addresses for each controller
 */
uint32_t *regrabl_pcie_get_iatu_reg(void)
{
	return &iatu_dma_offset[0];
}

/**
 * @brief API to get IO map starting address
 *
 * @retval Pointer to the array of IO map addresses for each controller
 */
uint32_t *regrabl_pcie_get_io_base(void)
{
	return &pcie_io_base[0];
}

/**
 * @brief API to get Memory map starting address
 *
 * @retval Pointer to the array of Memory map addresses for each controller
 */
uint32_t *regrabl_pcie_get_mem_base(void)
{
	return &pcie_mem_base[0];
}

static uintptr_t appl_reg_offset[6] = {
	NV_ADDRESS_MAP_PCIE_C0_CTL_BASE,
	NV_ADDRESS_MAP_PCIE_C1_CTL_BASE,
	NV_ADDRESS_MAP_PCIE_C2_CTL_BASE,
	NV_ADDRESS_MAP_PCIE_C3_CTL_BASE,
	NV_ADDRESS_MAP_PCIE_C4_CTL_BASE,
	NV_ADDRESS_MAP_PCIE_C5_CTL_BASE
};

static uint32_t pcie_appl_read32(uint8_t ctrl_num, uint32_t offset)
{
	return NV_READ32(appl_reg_offset[ctrl_num] + offset);
}

static void pcie_appl_write32(uint8_t ctrl_num, uint32_t offset, uint32_t val)
{
	NV_WRITE32(appl_reg_offset[ctrl_num] + offset, val);
}

static uint32_t pcie_dbi_read32(uint8_t ctrl_num, uint32_t offset)
{
	return NV_READ32(dbi_reg_offset[ctrl_num] + offset);
}

static void pcie_dbi_write32(uint8_t ctrl_num, uint32_t offset, uint32_t val)
{
	NV_WRITE32(dbi_reg_offset[ctrl_num] + offset, val);
}

static tegrabl_error_t tegra_reg_poll_mask(uintptr_t reg, uint32_t mask, uint32_t val, uint32_t timeout_us)
{
	tegrabl_error_t ret = TEGRABL_ERR_TIMEOUT;

	if ((NV_READ32(reg) & mask) == val) {
		/* No need to poll */
		ret = 0;
	} else {
		/* Poll */
		const uint32_t start = tegrabl_get_timestamp_us();

		do {
			if ((NV_READ32(reg) & mask) == val) {
				ret = 0;
				break;
			}
		} while ((tegrabl_get_timestamp_us() - start) < timeout_us);
	}

	return ret;
}

/* returns pcie controller node_offset in fdt */
static tegrabl_error_t tegrabl_locate_pcie_ctrl_in_dt(void *fdt, uint8_t ctrl_num, int32_t *ctrl_offset)
{
	tegrabl_error_t err = TEGRABL_NO_ERROR;
	uint8_t i;
	int32_t node_offset = 0;
	uintptr_t pcie_addr;

	/* scan the DT tree for a PCI_COMPATIBLE node */
	for  (i = 0; i < max_ctrl_supported; ++i) {
		err = tegrabl_dt_get_node_with_compatible(fdt, node_offset, PCIE_COMPATIBLE, &node_offset);
		if (TEGRABL_NO_ERROR != err) {
			pr_error("%s: compatible node not found; error=0x%x\n", __func__, err);
			goto fail;
		}

		/* parse the reg prop to match with ctrl_num's address */
		err = tegrabl_dt_read_reg_by_index(fdt, node_offset, 0, &pcie_addr, NULL);
		if (err != TEGRABL_NO_ERROR) {
			pr_error("%s: failed to get pcie address\n", __func__);
			goto fail;
		}

		if (pcie_addr == appl_reg_offset[ctrl_num]) {
			pr_info("%s: found match at 0x%lx\n", __func__, pcie_addr);
			*ctrl_offset = node_offset;
			break;
		}
	}

	if (i >= max_ctrl_supported) {
		pr_error("%s: cannot find controller\n", __func__);
		err = TEGRABL_ERR_NOT_SUPPORTED;
		goto fail;

	}

fail:
	return err;
}

static void tegrabl_power_on_one_phy(uintptr_t base)
{
	uintptr_t ptr;
	uint32_t val;

	pr_debug("base=0x%lx\n", base);
	ptr = base + P2U_CONTROL_GEN1;
	pr_debug("ptr=0x%lx\n", ptr);
	val = NV_READ32(ptr);
	pr_debug("R: val=0x%x\n", val);
	val &= ~P2U_CONTROL_GEN1_ENABLE_RXIDLE_ENTRY_ON_EIOS;
	val |= P2U_CONTROL_GEN1_ENABLE_RXIDLE_ENTRY_ON_LINK_STATUS;
	pr_debug("W: ptr=0x%lx, val=0x%x\n", ptr, val);
	NV_WRITE32(ptr, val);

	ptr = base + P2U_PERIODIC_EQ_CTRL_GEN3;
	pr_debug("ptr=0x%lx\n", ptr);
	val = NV_READ32(ptr);
	pr_debug("R: val=0x%x\n", val);
	val &= ~P2U_PERIODIC_EQ_CTRL_GEN3_PERIODIC_EQ_EN;
	val |= P2U_PERIODIC_EQ_CTRL_GEN3_INIT_PRESET_EQ_TRAIN_EN;
	pr_debug("W: ptr=0x%lx, val=0x%x\n", ptr, val);
	NV_WRITE32(ptr, val);
	ptr = base + P2U_PERIODIC_EQ_CTRL_GEN4;
	pr_debug("ptr=0x%lx\n", ptr);
	val = NV_READ32(ptr);
	pr_debug("R: val=0x%x\n", val);
	val |= P2U_PERIODIC_EQ_CTRL_GEN4_INIT_PRESET_EQ_TRAIN_EN;
	pr_debug("W: ptr=0x%lx, val=0x%x\n", ptr, val);
	NV_WRITE32(ptr, val);

	ptr = base + P2U_RX_DEBOUNCE_TIME;
	pr_debug("ptr=0x%lx\n", ptr);
	val = NV_READ32(ptr);
	pr_debug("R: val=0x%x\n", val);
	val &= ~P2U_RX_DEBOUNCE_TIME_DEBOUNCE_TIMER_MASK;
	val |= P2U_RX_DEBOUNCE_TIME_DEBOUNCE_TIMER_VAL;
	pr_debug("W: ptr=0x%lx, val=0x%x\n", ptr, val);
	NV_WRITE32(ptr, val);
}

tegrabl_error_t tegrabl_power_on_phy(uint8_t ctrl_num)
{
	tegrabl_error_t err;
	void *fdt;
	uintptr_t base, ptr;
	uint32_t val;
	const uint32_t *temp;
	int32_t node_offset = 0;
	uintptr_t phy_addr;
	uint32_t n_phys = 0;
	const uint32_t *phys_list;
	uint32_t phy_phandle, cur_phandle;
	bool is_available;

	if (ctrl_num >= max_ctrl_supported) {
		err = TEGRABL_ERR_NOT_SUPPORTED;
		goto fail;
	}

	/* Configure the sideband signals (PERST# and CLKREQ#) for C5 */
	if (ctrl_num == 5) {
		base = NV_ADDRESS_MAP_PADCTL_A20_BASE;
		ptr = base + PADCTL_PEX_CTL_2_PEX_L5_RST;
		pr_debug("PADCTL_PEX_CTL_2_PEX_L5_RST ptr=0x%lx\n", ptr);
		val = NV_READ32(ptr);
		pr_debug("R: val=0x%x\n", val);
		val &= ~PADCTL_PEX_RST_E_INPUT;
		pr_debug("W: ptr=0x%lx, val=0x%x\n", ptr, val);
		NV_WRITE32(ptr, val);
	}

	err = tegrabl_dt_get_fdt_handle(TEGRABL_DT_BL, &fdt);
	if (TEGRABL_NO_ERROR != err) {
		pr_error("%s: failed to get DT_BL; error=0x%x\n", __func__, err);
		goto fail;
	}

	err = tegrabl_locate_pcie_ctrl_in_dt(fdt, ctrl_num, &node_offset);
	if (err != TEGRABL_NO_ERROR) {
		pr_error("%s: no pcie controller node found in DT\n", __func__);
		goto fail;
	}

	err = tegrabl_dt_is_device_available(fdt, node_offset, &is_available);
	if (err != TEGRABL_NO_ERROR) {
		pr_error("%s %d failed\n", __func__, __LINE__);
		goto fail;
	}

	if (!is_available) {
		pr_info("%s: controller %u not available\n", __func__, ctrl_num);
		goto exit;
	}

	/* find "phys" property in this node */
	err = tegrabl_dt_count_elems_of_size(fdt, node_offset, PROPERTY_PHYS, 1, &n_phys);
	n_phys /= sizeof(uint32_t);
	pr_debug("n_phys=%u, err=0x%x\n", n_phys, err);
	if (err != TEGRABL_NO_ERROR) {
		pr_warn("Failed to get %s\n", PROPERTY_PHYS);
		/* not really an error, some controller has no "phys" property */
		err = TEGRABL_NO_ERROR;
		goto fail;
	}

	phys_list = fdt_getprop(fdt, node_offset, PROPERTY_PHYS, NULL);
	if (!phys_list) {
		pr_error("Failed to get %s property\n", PROPERTY_PHYS);
		err = TEGRABL_ERR_NOT_FOUND;
		goto fail;
	}

	/* For each phy in phys_list, find a phy node that has the matching phy_phandle */
	while (n_phys) {
		phy_phandle = fdt32_to_cpu(*phys_list);
		pr_debug("==== phy phandle=0x%x ===\n", phy_phandle);

		cur_phandle = (uint32_t)-1;
		node_offset = 0;
		while (cur_phandle != phy_phandle) {
			/* find a DT phy node that has the matching phy_phandle */
			err = tegrabl_dt_get_node_with_compatible(fdt, node_offset, PCIE_PHY_COMPATIBLE, &node_offset);
			if (TEGRABL_NO_ERROR != err) {
				pr_error("%s: %s node not found; error=0x%x\n", __func__, PCIE_PHY_COMPATIBLE, err);
				goto fail;
			}

			temp = fdt_getprop(fdt, node_offset, PROPERTY_PHANDLE, NULL);
			if (!temp) {
				pr_error("Failed to get %s property\n", PROPERTY_PHANDLE);
				err = TEGRABL_ERR_NOT_FOUND;
				goto fail;
			}

			cur_phandle = fdt32_to_cpu(*temp);
			pr_debug("phy phandle=0x%x, searching for 0x%x.\n", cur_phandle, phy_phandle);

			if (cur_phandle == phy_phandle) {
				pr_debug("found one\n");

				/* get the reg addr from that node */
				err = tegrabl_dt_read_reg_by_index(fdt, node_offset, 0, &phy_addr, NULL);
				if (err != TEGRABL_NO_ERROR) {
					pr_error("%s: failed to get pcie address\n", __func__);
					err = TEGRABL_NO_ERROR;
					goto fail;
				}

				pr_info("%s: power on phy @0x%lx\n", __func__, phy_addr);
				tegrabl_power_on_one_phy(phy_addr);
				break;
			}
		}

		phys_list += 1;
		--n_phys;
	}

exit:
fail:
	return err;
}

tegrabl_error_t tegrabl_pcie_soc_preinit(uint8_t ctrl_num)
{
	tegrabl_error_t error;

	pr_info("%s: (%u):\n", __func__, ctrl_num);

	pr_info("Unpowergate\n");
	switch (ctrl_num) {
	case 0:
		error = tegrabl_pcie_unpowergate(TEGRA194_POWER_DOMAIN_PCIEX8B);
		break;
	case 1:
	case 2:
	case 3:
		error = tegrabl_pcie_unpowergate(TEGRA194_POWER_DOMAIN_PCIEX1A);
		break;
	case 4:
		error = tegrabl_pcie_unpowergate(TEGRA194_POWER_DOMAIN_PCIEX4A);
		break;
	case 5:
		error = tegrabl_pcie_unpowergate(TEGRA194_POWER_DOMAIN_PCIEX8A);
		break;
	default:
		error = TEGRABL_ERR_INVALID;
		break;
	}

	if (error != TEGRABL_NO_ERROR) {
		pr_error("pcie_preinit: Failed to unpowergate (err=%d)\n", error);
		goto fail;
	}
	tegrabl_udelay(100);

	/** Assert PEX CORE RST, PEX APB RST and disable PEX CORE CLK */
	pr_info("tegrabl_car_clk_disable(%u) ...\n", ctrl_num);
	error = tegrabl_car_clk_disable(TEGRABL_MODULE_PCIE_CORE, ctrl_num);
	if (error != TEGRABL_NO_ERROR) {
		goto fail;
	}
	pr_info("tegrabl_car_rst_set(CORE, %u) ...\n", ctrl_num);
	error = tegrabl_car_rst_set(TEGRABL_MODULE_PCIE_CORE, ctrl_num);
	if (error != TEGRABL_NO_ERROR) {
		goto fail;
	}
	pr_info("tegrabl_car_rst_set(APB, %u) ...\n", ctrl_num);
	error = tegrabl_car_rst_set(TEGRABL_MODULE_PCIE_APB, ctrl_num);
	if (error != TEGRABL_NO_ERROR) {
		goto fail;
	}

	/** Enable PEX CORE CLK */
	pr_info("tegrabl_car_clk_enable(%u) ...\n", ctrl_num);
	error = tegrabl_car_clk_enable(TEGRABL_MODULE_PCIE_CORE, ctrl_num, NULL);
	if (error != TEGRABL_NO_ERROR) {
		goto fail;
	}

	/** Deassert PEX APB RST */
	pr_info("tegrabl_car_rst_clear(APB, %u) ...\n", ctrl_num);
	error = tegrabl_car_rst_clear(TEGRABL_MODULE_PCIE_APB, ctrl_num);
	if (error != TEGRABL_NO_ERROR) {
		tegrabl_car_clk_disable(TEGRABL_MODULE_PCIE_CORE, ctrl_num);
		goto fail;
	}

	pr_info("tegrabl_set_ctrl_state(%u)\n", ctrl_num);
	error = tegrabl_set_ctrl_state(ctrl_num, true);
	if (error != TEGRABL_NO_ERROR) {
		pr_error("%s: Failed to tegrabl_set_ctrl_state (err=%d)\n", __func__, error);
		goto fail;
	}

	pr_info("CLR PCIE_APB:6\n");
	error = tegrabl_car_rst_clear(TEGRABL_MODULE_PCIE_APB, 6);
	tegrabl_udelay(100);
	if (error != TEGRABL_NO_ERROR) {
		pr_error("%s: Failed to unreset PCIE_APB:6 (err=%d)\n", __func__, error);
		goto fail;
	}

fail:
	return error;
}

tegrabl_error_t tegrabl_pcie_soc_init(uint8_t ctrl_num, uint8_t link_speed)
{
	tegrabl_error_t error;
	uint32_t val;

	pr_info("%s: (%u):\n", __func__, ctrl_num);

	/** APPL initialization before PCIe*/
	pr_info("APPL initialization ...\n");

	pcie_appl_write32(ctrl_num, APPL_CFG_BASE_ADDR, dbi_reg_offset[ctrl_num] & APPL_CFG_BASE_ADDR_MASK);
	pcie_appl_write32(ctrl_num, APPL_CFG_IATU_DMA_BASE_ADDR,
					  iatu_dma_offset[ctrl_num] & APPL_CFG_IATU_DMA_BASE_ADDR_MASK);
	pcie_appl_write32(ctrl_num, APPL_DM_TYPE, APPL_DM_TYPE_RP);

	val = pcie_appl_read32(ctrl_num, APPL_CTRL);
	pcie_appl_write32(ctrl_num, APPL_CTRL, val | APPL_CTRL_SYS_PRE_DET_STATE);

	pcie_appl_write32(ctrl_num, APPL_CFG_SLCG_OVERRIDE, 0);

	val = pcie_appl_read32(ctrl_num, APPL_CFG_MISC);
	val |= (APPL_CFG_MISC_ARCACHE_VAL << APPL_CFG_MISC_ARCACHE_SHIFT);
	pcie_appl_write32(ctrl_num, APPL_CFG_MISC, val);

	val = pcie_appl_read32(ctrl_num, APPL_PINMUX);
	val |= APPL_PINMUX_CLKREQ_OVERRIDE_EN;	/* CLKREQ_OVERRIDE_EN to 1 */
	val &= ~APPL_PINMUX_CLKREQ_OVERRIDE;	/* CLKREQ_OVERRIDE to 0 */
	pcie_appl_write32(ctrl_num, APPL_PINMUX, val);

	/* power on phys */
	pr_info("poweron phys\n");
	/* ignore error returned from tegrabl_power_on_phy(), treat it as no phy specified in DT. */
	tegrabl_power_on_phy(ctrl_num);

	/** Deassert PEX CORE RST */
	error = tegrabl_car_rst_clear(TEGRABL_MODULE_PCIE_CORE, ctrl_num);
	if (error != TEGRABL_NO_ERROR) {
		goto fail;
	}

	val = pcie_dbi_read32(ctrl_num, PORT_LOGIC_AUX_CLK_FREQ_OFF);
	val &= ~AUX_CLK_FREQ_MASK;
	val |= AUX_CLK_FREQ_VAL << AUX_CLK_FREQ_SHIFT;
	pcie_dbi_write32(ctrl_num, PORT_LOGIC_AUX_CLK_FREQ_OFF, val);

	val = pcie_dbi_read32(ctrl_num, PCIE_CAP_LINK_CONTROL2_LINK_STATUS2_REG);
	val &= ~PCIE_CAP_TARGET_LINK_SPEED_MASK;
	val |= link_speed << PCIE_CAP_TARGET_LINK_SPEED_SHIFT;
	pcie_dbi_write32(ctrl_num, PCIE_CAP_LINK_CONTROL2_LINK_STATUS2_REG, val);

	val = pcie_dbi_read32(ctrl_num, PORT_LOGIC_GEN2_CTRL_OFF);
	val &= ~NUM_OF_LANES_MASK;
	val |= 1 << NUM_OF_LANES_SHIFT;
	pcie_dbi_write32(ctrl_num, PORT_LOGIC_GEN2_CTRL_OFF, val);

	val = pcie_dbi_read32(ctrl_num, PORT_LOGIC_PORT_LINK_CTRL_OFF);
	val &= ~LINK_CAPABLE_MASK;
	val |= 1 << LINK_CAPABLE_SHIFT;
	pcie_dbi_write32(ctrl_num, PORT_LOGIC_PORT_LINK_CTRL_OFF, val);

	/** Deassert PEX_RST signal to endpoint */
	val = pcie_appl_read32(ctrl_num, APPL_PINMUX);
	val &= ~APPL_PINMUX_PEX_RST;	/* APPL_PINMUX_PEX_RST to 0 */
	pcie_appl_write32(ctrl_num, APPL_PINMUX, val);

	tegrabl_udelay(100000U);

	/** Assert PEX_RST signal to endpoint */
	val = pcie_appl_read32(ctrl_num, APPL_PINMUX);
	val |= APPL_PINMUX_PEX_RST;	/* APPL_PINMUX_PEX_RST to 1 */
	pcie_appl_write32(ctrl_num, APPL_PINMUX, val);

	tegrabl_udelay(1U);

	/** Start LTSSM from RP side */
	val = pcie_appl_read32(ctrl_num, APPL_CTRL);
	val |= APP_LTSSM_ENABLE;
	pcie_appl_write32(ctrl_num, APPL_CTRL, val);

	/** Poll for PCIe link up */
	error = tegra_reg_poll_mask(appl_reg_offset[ctrl_num] + APPL_LINK_STATUS, RDLH_LINK_UP_MASK,
								RDLH_LINK_UP, 1000000);
	if (error != TEGRABL_NO_ERROR) {
		pr_critical("Failed to link up controller-%d\n", ctrl_num);
		error = TEGRABL_ERR_INIT_FAILED;
		goto fail;
	}
	pr_info("PCIe controller-%d link is up\n", ctrl_num);

fail:
	if (error != TEGRABL_NO_ERROR) {
		tegrabl_pcie_reset_state(ctrl_num);
	}
	return error;
}

/**
 * @brief API to disable PCIe host controller link with the endpoint
 *
 * @param[in] ctrl_num
 *
 * @returns TEGRABL_NO_ERROR if successful, appropriate error otherwise
 */
tegrabl_error_t tegrabl_pcie_soc_disable_link(uint8_t ctrl_num)
{
	tegrabl_error_t error = TEGRABL_NO_ERROR;
	uint32_t val;

	val = pcie_dbi_read32(ctrl_num, PCIE_CAP_LINK_CONTROL_LINK_STATUS_REG);
	val &= ~PCIE_CAP_LINK_DISABLE_MASK;
	val |= 1 << PCIE_CAP_LINK_DISABLE_SHIFT;
	pcie_dbi_write32(ctrl_num, PCIE_CAP_LINK_CONTROL_LINK_STATUS_REG, val);

	/* Poll for PCIe link disabled */
	error = tegra_reg_poll_mask(appl_reg_offset[ctrl_num] + APPL_DEBUG, SMLH_LTSSM_STATE_MASK,
								(0x19 << SMLH_LTSSM_STATE_SHIFT), 1000000);
	if (error != TEGRABL_NO_ERROR) {
		pr_error("Failed to disable link on controller-%d\n", ctrl_num);
		error = TEGRABL_ERR_DISABLE_FAILED;
	}

	return error;
}

/**
 * @brief Perform enabling regulators for certain PCIe controller
 *
 * Scan device tree for regulators (in pcie_regulator_names[]) of the specified controller.
 * If regulator name is found in the device tree, enable the specified regulator.
 *
 * @param[in] ctrl_num specifies the controller number whose regulators to be enabled
 *
 * @return TEGRABL_NO_ERROR if successful, appropriate error otherwise
 */
tegrabl_error_t tegrabl_pcie_enable_regulators(uint8_t ctrl_num)
{
	tegrabl_error_t err = TEGRABL_NO_ERROR;
	void *fdt;
	const uint32_t *temp;
	int32_t node_offset = 0;
	int32_t reg_node_offset = 0;
	uint32_t i;
	uint32_t reg_voltage = 0;
	int32_t reg_phandle;
	uint8_t index = 0;
	bool found_regulator = false;

	err = tegrabl_dt_get_fdt_handle(TEGRABL_DT_BL, &fdt);
	if (TEGRABL_NO_ERROR != err) {
		pr_error("%s: failed to get DT_BL; error=0x%x\n", __func__, err);
		goto fail;
	}

	err = tegrabl_locate_pcie_ctrl_in_dt(fdt, ctrl_num, &node_offset);
	if (err != TEGRABL_NO_ERROR) {
		/* if error is returned, treat it as no regulators to be enabled */
		pr_warn("%s: no pcie controller node found in DT\n", __func__);
		err = TEGRABL_NO_ERROR;
		goto fail;
	}

	/* parse all supply names. It is not an error if no regulator is found. */
	for (i = 0; i < ARRAY_SIZE(pcie_regulator_names); i++) {
		reg_phandle = 0;
		temp = fdt_getprop(fdt, node_offset, pcie_regulator_names[i], NULL);

		if (temp != NULL) {
			reg_phandle = fdt32_to_cpu(*temp);
			pr_info("i=%u, reg_phandle=0x%x\n", i, reg_phandle);
		} else {
			pr_info("%s not found\n", pcie_regulator_names[i]);
			continue;
		}

		if (reg_phandle) {
			reg_node_offset = fdt_node_offset_by_phandle(fdt, reg_phandle);
			temp = fdt_getprop(fdt, reg_node_offset, PROPERTY_VOLTAGE, NULL);
			if (temp != NULL) {
				reg_voltage = fdt32_to_cpu(*temp);
				pr_info("reg_voltage=%u\n", reg_voltage);
			} else {
				err = TEGRABL_ERR_NOT_FOUND;
				pr_error("%s: regulator voltage not specified; error=0x%x\n", __func__, err);
				goto fail;
			}

			pr_info("%s: regulator_set_voltage(0x%x, %u)\n", __func__, reg_phandle, reg_voltage);
			err = tegrabl_regulator_set_voltage(reg_phandle, reg_voltage, STANDARD_VOLTS);
			if (err != TEGRABL_NO_ERROR) {
				pr_error("%s: failed tegrabl_regulator_set_voltage(0x%x, %u)\n", __func__,
						 reg_phandle, reg_voltage);
				goto fail;
			}
			pcie_ctrl_regulator_handles[ctrl_num][index] = reg_phandle;
			++index;
			found_regulator = true;
		}
	}

	/* If DT has no pcie_regulator_names, try to look for "nvidia,plat-gpios" */
	if (!found_regulator) {
		uint32_t n_gpios = 0;
		uint32_t gpio_chip_id;
		uint8_t state;
		struct gpio_driver *gpio_drv = NULL;
		uint32_t gpio_phandle, gpio_pin, gpio_polarity;

		pr_trace("Trying to get %s:\n", PROPERTY_PLAT_GPIOS);

		err = tegrabl_dt_count_elems_of_size(fdt, node_offset, PROPERTY_PLAT_GPIOS, 3, &n_gpios);
		pr_debug("err=0x%x, n_gpios=%u\n", err, n_gpios);
		n_gpios /= sizeof(uint32_t);
		if (err != TEGRABL_NO_ERROR) {
			pr_warn("Failed to get nvidia,plat-gpios\n");
			/* not really an error */
			err = TEGRABL_NO_ERROR;
			goto fail;
		}

		temp = fdt_getprop(fdt, node_offset, PROPERTY_PLAT_GPIOS, NULL);
		if (!temp) {
			pr_error("Failed to get %s property\n", PROPERTY_PLAT_GPIOS);
			err = TEGRABL_ERR_NOT_FOUND;
			goto fail;
		}

		while (n_gpios) {
			gpio_phandle = fdt32_to_cpu(*temp);
			gpio_pin = fdt32_to_cpu(*(temp+1));
			gpio_polarity = fdt32_to_cpu(*(temp+2));
			pr_info("gpio phandle=0x%x, pin=0x%x, polarity=0x%x\n", gpio_phandle, gpio_pin, gpio_polarity);

			err = tegrabl_gpio_get_chipid_with_phandle(gpio_phandle, &gpio_chip_id);
			if (err != TEGRABL_NO_ERROR) {
				pr_error("Failed to get chip id from gpio_phandle\n");
				goto fail;
			}
			err = tegrabl_gpio_driver_get(gpio_chip_id, &gpio_drv);
			if (err != TEGRABL_NO_ERROR) {
				pr_error("Failed to get gpio driver handle\n");
				goto fail;
			}
			err = gpio_config(gpio_drv, gpio_pin, GPIO_PINMODE_OUTPUT);
			if (err != TEGRABL_NO_ERROR) {
				pr_error("Failed to configure gpio mode\n");
				goto fail;
			}
			state = GPIO_PIN_STATE_HIGH ^ gpio_polarity;
			err = gpio_write(gpio_drv, gpio_pin, state);
			if (err != TEGRABL_NO_ERROR) {
				pr_error("Failed to write gpio (0x%x) state to %u\n", gpio_pin, state);
				goto fail;
			}

			temp += 3;
			--n_gpios;
		}
	}

fail:
	return err;
}

/**
 * @brief Perform disabling regulators for certain PCIe controller
 *
 * @param[in] ctrl_num specifies the controller number whose regulators to be disabled
*/
void tegrabl_pcie_disable_regulators(uint8_t ctrl_num)
{
	uint8_t i;
	int32_t reg_handle;

	for (i = 0; i < 2; ++i) {
		reg_handle = pcie_ctrl_regulator_handles[ctrl_num][i];
		if (reg_handle) {
			pr_info("%s: disable regulator 0x%x\n", __func__, reg_handle);
			tegrabl_regulator_disable(reg_handle);
		}
	}
}

#if defined(CONFIG_ENABLE_NVME_BOOT)
/*
 * tegrabl_get_pcie_ctrl_nums() function returns a list of pcie controller numbers based
 * on the original boot_dev:
 *
 * Note: boot_dev has the "nvme" stripped from its original boot_dev.
 *
 * original boot_dev		list of ctrl_nums
 * -----------------		-----------------------------------
 *   "nvme"					0, 1, . . , max_ctrl_supported-1 (to probe all PCIe controllers)
 *   "nvme:C<n>"			n (to probe just PCIe controller Cn)
 *   "nvme:pcie@<addr>"		n (to probe a PCIe controller n which has the PCI address of <addr>)
 *
 * The returned list is terminated with -1.
*/

int8_t *tegrabl_get_pcie_ctrl_nums(char *boot_dev)
{
#define PCIE_HEADER	"pcie@"

	int8_t *p_pcie_ctrl_nums;
	char *hdr;
	uintptr_t pcie_addr;
	int8_t ctrl_num;

	/*
	 * Parse boot_dev string, it could be in the following format:
	 *   - :Cn;
	 *   - :pcie@xxxxxxxx;
	 *   - '\0';
	 */

	if (boot_dev == NULL) {
		return NULL;
	}

	switch (boot_dev[0]) {
	case '\0':
		/* This is the bare nvme case: probe all controllers */
		p_pcie_ctrl_nums = (int8_t *)tegrabl_calloc(sizeof(int8_t), max_ctrl_supported + 1);
		if (p_pcie_ctrl_nums == NULL) {
			pr_error("%s: memory allocation (%u) failed\n", __func__, max_ctrl_supported + 1);
			return NULL;
		}
		for (ctrl_num = 0; (uint8_t)ctrl_num < max_ctrl_supported; ++ctrl_num) {
			*(p_pcie_ctrl_nums + ctrl_num) = ctrl_num;
		}
		*(p_pcie_ctrl_nums + ctrl_num) = -1;
		return p_pcie_ctrl_nums;
	case ':':
		/* This is either in "nvme:C<n>" or "nvme:pcie@<address>" case: */
		++boot_dev;
		if ((boot_dev[0] == 'C') || (boot_dev[0] == 'c')) {
			++boot_dev;
			ctrl_num = (int8_t)tegrabl_utils_strtoul(boot_dev, NULL, 10);
			if ((uint8_t)ctrl_num >= max_ctrl_supported) {
				pr_error("%s: wrong controller number (%s)\n", __func__, boot_dev);
				ctrl_num = -1;
			}
		} else {
			hdr = strstr(boot_dev, PCIE_HEADER);
			if (hdr == NULL) {
				/* boot_dev has a wrong header */
				pr_error("%s: invalid boot_dev (%s); return -1\n", __func__, boot_dev);
				ctrl_num = -1;
			} else {
				hdr += strlen(PCIE_HEADER);
				/* hdr now points to the hex number, which is the pcie controller's address */
				pr_debug("hdr=%s\n", hdr);

				pcie_addr = (uintptr_t)tegrabl_utils_strtoul(hdr, NULL, 16);
				pr_debug("pcie_addr=0x%lx\n", pcie_addr);

				/* Find which controller has the pcie_addr */
				for (ctrl_num = 0; (uint8_t)ctrl_num < ARRAY_SIZE(appl_reg_offset); ++ctrl_num) {
					if (appl_reg_offset[ctrl_num] == pcie_addr) {
						pr_info("%s: found ctrl_num=%u\n", __func__, ctrl_num);
						break;
					}
				}
				if (((uint8_t)ctrl_num >= max_ctrl_supported) ||
					((uint8_t)ctrl_num >= ARRAY_SIZE(appl_reg_offset))) {
					pr_error("%s: cannot find a match (boot_dev=%s, pcie_addr=0x%lx\n",
							  __func__, boot_dev, pcie_addr);
					ctrl_num = -1;
				}
			}
		}
		break;
	default:
		pr_error("%s: invalid boot_dev (%s); return NULL\n", __func__, boot_dev);
		return NULL;
	}

	/* allocate 2 elements, one for ctrl_num (even if it is -1), the other one for terminating the array */
	p_pcie_ctrl_nums = (int8_t *)tegrabl_calloc(sizeof(int8_t), 2);
	if (p_pcie_ctrl_nums == NULL) {
		pr_error("%s: memory allocation (%u) failed\n", __func__, 2);
		return NULL;
	}

	*p_pcie_ctrl_nums = ctrl_num;
	*(p_pcie_ctrl_nums + 1) = -1;

	return p_pcie_ctrl_nums;
}
#endif
