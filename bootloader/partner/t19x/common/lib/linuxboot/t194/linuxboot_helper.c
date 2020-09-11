/*
 * Copyright (c) 2015-2019, NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#define MODULE	TEGRABL_ERR_LINUXBOOT

#include "build_config.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <libfdt.h>
#include <tegrabl_io.h>
#include <tegrabl_drf.h>
#include <tegrabl_error.h>
#include <tegrabl_debug.h>
#include <tegrabl_malloc.h>
#include <tegrabl_compiler.h>
#include <tegrabl_addressmap.h>
#include <tegrabl_linuxboot.h>
#include <tegrabl_linuxboot_helper.h>
#include <tegrabl_sdram_usage.h>
#include <tegrabl_cpubl_params.h>
#include <linux_load.h>
#include <tegrabl_ar_macro.h>
#include <armiscreg.h>
#include <arscratch.h>
#include <tegrabl_fuse.h>
#include <tegrabl_profiler.h>
#include <tegrabl_uart.h>
#include <tegrabl_comb_uart.h>
#include <tegrabl_soc_misc.h>
#include <tegrabl_board_info.h>
#include <tegrabl_devicetree.h>
#include <tegrabl_odmdata_soc.h>
#include <tegrabl_t194_ccplex_nvg.h>
#include <tegrabl_blockdev.h>
#include <tegrabl_storage_device_params.h>
#include <tegrabl_vprinfo.h>
#include <cboot_rollback_protection.h>
#include <nvboot_boot_component.h>

#if defined(CONFIG_ENABLE_STAGED_SCRUBBING)
#include <qual_engine.h>
#endif

#if defined(CONFIG_ENABLE_A_B_SLOT)
#include <tegrabl_a_b_boot_control.h>
#endif

#if defined(CONFIG_ENABLE_ANDROID_BOOTREASON)
#include <arpmc_impl.h>
#include <tegrabl_pmic.h>

#define PMC_RST_REASONS_NR (18)
#endif

#define mc_read32(reg)	NV_READ32(NV_ADDRESS_MAP_MCB_BASE + reg)

#define SCRATCH_READ(reg)			\
		NV_READ32(NV_ADDRESS_MAP_SCRATCH_BASE + SCRATCH_##reg)

extern struct tboot_cpubl_params *boot_params;
static uint64_t os_carveout_next_free_addr;

static const uint32_t oem_fw_bin_type_mapping[TEGRABL_BINARY_MAX] = {
	[TEGRABL_BINARY_KERNEL] = OEM_FW_RATCHET_IDX_KERNEL,
	[TEGRABL_BINARY_KERNEL_DTB] = OEM_FW_RATCHET_IDX_KERNEL_DTB,
};

static int add_tegraid(char *cmdline, int len, char *param, void *priv)
{
	uint8_t chip_id = 0;
	uint8_t major = 0;
	uint8_t minor = 0;
	uint16_t netlist = 0;
	uint16_t patch = 0;
	uint32_t reg;
	TEGRABL_UNUSED(priv);

	if (!cmdline || !param) {
		return -1;
	}

	reg = NV_READ32(NV_ADDRESS_MAP_MISC_BASE + MISCREG_HIDREV_0);

	chip_id = NV_DRF_VAL(MISCREG, HIDREV, CHIPID, reg);
	major = NV_DRF_VAL(MISCREG, HIDREV, MAJORREV, reg);
	minor = NV_DRF_VAL(MISCREG, HIDREV, MINORREV, reg);

	reg = NV_READ32(NV_ADDRESS_MAP_MISC_BASE + MISCREG_EMU_REVID_0);

	netlist = NV_DRF_VAL(MISCREG, EMU_REVID, NETLIST, reg);
	patch = NV_DRF_VAL(MISCREG, EMU_REVID, PATCH, reg);

	return tegrabl_snprintf(cmdline, len, "%s=%x.%x.%x.%x.%x ", param,
					chip_id, major, minor, netlist, patch);
}

static int add_kerneltype(char *, int, char *, void *) __attribute__ ((unused));
static int add_kerneltype(char *cmdline, int len, char *param, void *priv)
{
	tegrabl_binary_type_t bin_type;
	int ret = -1;

	TEGRABL_UNUSED(priv);

	if (!cmdline || !param) {
		goto fail;
	}

	bin_type = tegrabl_get_kernel_type();
	ret = tegrabl_snprintf(cmdline, len, "%s=%s ", param,
			bin_type == TEGRABL_BINARY_KERNEL ? "normal" : "recovery");
fail:
	return ret;
}

static int add_maxcpus(char *cmdline, int len, char *param, void *priv)
{
	uint32_t num_cores = 0;
	TEGRABL_UNUSED(priv);

	if (!cmdline || !param) {
		return -1;
	}

#if defined(CONFIG_MULTICORE_SUPPORT)
	num_cores = tegrabl_ccplex_nvg_num_cores();

	return tegrabl_snprintf(cmdline, len, "%s=%u ", param, num_cores);
#else
	TEGRABL_UNUSED(num_cores);

	return tegrabl_snprintf(cmdline, len, "%s=1 ", param);
#endif
}

static int add_vdk_ip(char *cmdline, int len, char *param, void *priv)
{
	TEGRABL_UNUSED(priv);

	if (!cmdline || !param)
		return TEGRABL_ERROR(TEGRABL_ERR_INVALID, 0);

	if (tegrabl_is_vdk()) {
		return tegrabl_snprintf(cmdline, len, "%s=%s ", param, "10.0.2.15");
	} else {
		return 0;
	}
}

static int add_presilicon(char *, int, char *, void *) __attribute__ ((unused));
static int add_presilicon(char *cmdline, int len, char *param, void *priv)
{
	TEGRABL_UNUSED(priv);

	if (!cmdline || !param)
		return TEGRABL_ERROR(TEGRABL_ERR_INVALID, 0);

	if (tegrabl_is_vdk() || tegrabl_is_fpga()) {
		return tegrabl_snprintf(cmdline, len, "%s=%s ", param, "true");
	} else {
		return 0;
	}
}

static int add_secure_state(char *, int, char *, void *) __attribute__ ((unused));
static int add_secure_state(char *cmdline, int len, char *param, void *priv)
{
	tegrabl_error_t err = TEGRABL_NO_ERROR;
	uint32_t secure_info;
	bool is_nv_production_mode = false;
	char *secure_state = NULL;

	TEGRABL_UNUSED(priv);

	if (!cmdline || !param) {
		return TEGRABL_ERROR(TEGRABL_ERR_INVALID, 0);
	}

	err = tegrabl_fuse_read(FUSE_TYPE_BOOT_SECURITY_INFO, &secure_info,
							sizeof(uint32_t));
	if (err != TEGRABL_NO_ERROR) {
		pr_error("Failed to read security info from fuse\n");
		return err;
	}

	is_nv_production_mode = fuse_is_nv_production_mode();

	/**
	 *  When FUSE_BOOT_SECURITY_INFO =
	 *  FUSE_BOOT_SECURITY_AESCMAC,
	 *  it means device is in non-secure mode.
	 *  When FUSE_BOOT_SECURITY_INFO =
	 *  FUSE_BOOT_SECURITY_RSA,
	 *  FUSE_BOOT_SECURITY_ECC,
	 *  FUSE_BOOT_SECURITY_AESCMAC_ENCRYPTION,
	 *  FUSE_BOOT_SECURITY_RSA_ENCRYPTION,
	 *  FUSE_BOOT_SECURITY_ECC_ENCRYPTION
	 *  it means device is in secure mode.
	 */
	if (is_nv_production_mode && secure_info > FUSE_BOOT_SECURITY_AESCMAC) {
		/* true secure mode */
		secure_state = "enabled";
	} else if (!is_nv_production_mode &&
			   secure_info > FUSE_BOOT_SECURITY_AESCMAC) {
		/* mixed configuration secure mode, customer will never get this
		 * mode of device */
		secure_state = "test.enabled";
	} else
		secure_state = "non-secure";

	return tegrabl_snprintf(cmdline, len, "%s=%s ", param, secure_state);
}

static int add_bootmode(char *, int, char *, void *) __attribute__ ((unused));
static int add_bootmode(char *cmdline, int len, char *param, void *priv)
{
	uint32_t odmdata_bootmode;
	char *bootmode_str = NULL;

	TEGRABL_UNUSED(priv);

	if (cmdline == NULL || param == NULL)
		return -1;

	odmdata_bootmode = tegrabl_odmdata_get() & TEGRA_BOOTMODE_MASK;

	switch (odmdata_bootmode) {
	case TEGRA_BOOTMODE_ANDROID_SHELL_VAL:
		/* boot to shell */
		bootmode_str = "shell";
		break;
	case TEGRA_BOOTMODE_ANDROID_PRESI_VAL:
		/* pre-silicon mode  */
		bootmode_str = "pre_si";
		break;
	case TEGRA_BOOTMODE_ANDROID_UI_VAL:
		/* fall through */
	default:
		/* boot to ui by default */
		bootmode_str = "ui";
		break;
	}

	return tegrabl_snprintf(cmdline, len, "%s=%s ", param, bootmode_str);
}

#if defined(CONFIG_ENABLE_ANDROID_BOOTREASON)
const char *pmc_reset_reason_string[PMC_RST_REASONS_NR] = {
	"power_on_reset",
	"ao_watchdog",
	"denver_watchdog",
	"bpmp_watchdog",
	"sce_watchdog",
	"spe_watchdog",
	"ape_watchdog",
	"a57_watchdog",
	"sensor",
	"aotag",
	"vfsensor",
	"software_reset",
	"sc7",
	"hsm",
	"csite",
	"watchdog",
	"lp0",
	"reset_reason_max"
};

static int add_androidboot_bootreason(char *, int, char *, void *) __attribute__ ((unused));
static int add_androidboot_bootreason(char *cmdline, int len, char *param, void *priv)
{
	uint32_t reg;
	uint32_t rst_src;
	const char *reset_reason;
	uint32_t nverc;
	tegrabl_error_t err;

	TEGRABL_UNUSED(priv);

	if (!cmdline || !param) {
		return -1;
	}

	reg = NV_READ32(NV_ADDRESS_MAP_PMC_IMPL_BASE + PMC_IMPL_RST_STATUS_0);
	rst_src = (reg & PMC_IMPL_RST_STATUS_0_READ_MASK) >> PMC_IMPL_RST_STATUS_0_RST_SOURCE_SHIFT;

	if (rst_src >= PMC_RST_REASONS_NR) {
		reset_reason = "unknown";
	} else {
		reset_reason = pmc_reset_reason_string[rst_src];
	}

	err = tegrabl_pmic_get_reset_reason(&nverc);

	if (TEGRABL_NO_ERROR != err) {
		nverc = 0xff;
	}

	return tegrabl_snprintf(cmdline, len, "%s=pmc:%s,pmic:0x%x ", param, reset_reason, nverc);
}
#endif

static int tegrabl_linuxboot_add_vpr_info(char *cmdline, int len,
										  char *param, void *priv)
{
	uint64_t base = 0;
	uint64_t size = 0;

	TEGRABL_UNUSED(priv);

	if (!cmdline || !param) {
		return -1;
	}

	base = boot_params->carveout_info[CARVEOUT_VPR].base;
	size = boot_params->carveout_info[CARVEOUT_VPR].size;

	if (size)
		return tegrabl_snprintf(cmdline, len, "%s=0x%" PRIx64 "@0x%08" PRIx64
								" ", param, size, base);
	else
		return 0;
}

static int tegrabl_linuxboot_add_vprresize_info(char *cmdline, int len,
												char *param, void *priv)
{
	TEGRABL_UNUSED(priv);

	if (!cmdline || !param) {
		return -1;
	}

	if (tegrabl_is_vpr_resize_enabled())
		return tegrabl_snprintf(cmdline, len, "%s ", param);
	else
		return 0;
}

static int add_profiler_carveout(char *cmdline, int len,
								 char *param, void *priv)
{
	int ret = 0;
	TEGRABL_UNUSED(priv);

#if !defined(CONFIG_BOOT_PROFILER)
	TEGRABL_UNUSED(cmdline);
	TEGRABL_UNUSED(len);
	TEGRABL_UNUSED(param);
#else
	if (boot_params->profiling_data_address != 0) {
		ret += tegrabl_snprintf(cmdline, len, "%s=0x%" PRIx32 "@0x%08" PRIx64
								" ", param, TEGRABL_PROFILER_PAGE_SIZE,
								boot_params->profiling_data_address);
	}
#endif

	return ret;
}

#if defined(CONFIG_ENABLE_NVDEC)
static int tegrabl_linuxboot_add_nvdec_enabled_info(char *cmdline, int len,
	char *param, void *priv)
{
	TEGRABL_UNUSED(priv);
	return tegrabl_snprintf(cmdline, len, "%s=1 ", param);
}
#endif

#if defined(CONFIG_ENABLE_VERIFIED_BOOT)
static const char *cmdline_vb_boot_state;
tegrabl_error_t tegrabl_linuxboot_set_vbstate(const char *vbstate)
{
	cmdline_vb_boot_state = vbstate;
	return TEGRABL_NO_ERROR;
}

static int add_vb_boot_state(char *, int, char *, void *) __attribute__ ((unused));
static int add_vb_boot_state(char *cmdline, int len, char *param, void *priv)
{
	TEGRABL_UNUSED(priv);

	if (!cmdline || !param) {
		return -1;
	}

	if (!cmdline_vb_boot_state) {
		return 0;
	}

	return tegrabl_snprintf(cmdline, len, "%s=%s ", param,
							cmdline_vb_boot_state);
}

static const char *cmdline_vbmeta_info;
tegrabl_error_t tegrabl_linuxboot_set_vbmeta_info(const char *vbmeta)
{
	cmdline_vbmeta_info = vbmeta;
	return TEGRABL_NO_ERROR;
}

static int add_vbmeta_info(char *, int, char *, void *) __attribute__ ((unused));
static int add_vbmeta_info(char *cmdline, int len, char *param, void *priv)
{
	TEGRABL_UNUSED(priv);
	TEGRABL_UNUSED(param);

	if (!cmdline || !param) {
		return -1;
	}

	if (!cmdline_vbmeta_info) {
		return 0;
	}

	return tegrabl_snprintf(cmdline, len, "%s ", cmdline_vbmeta_info);
}
#endif

#if defined(CONFIG_ENABLE_A_B_SLOT)
static int add_boot_slot_suffix(char *, int, char *, void *) __attribute__ ((unused));
static int add_boot_slot_suffix(char *cmdline, int len, char *param, void *priv)
{
	char slot_suffix[BOOT_CHAIN_SUFFIX_LEN + 1];
	tegrabl_error_t status;

	TEGRABL_UNUSED(priv);

	if (!cmdline || !param) {
		return TEGRABL_ERROR(TEGRABL_ERR_INVALID, 0);
	}

	status = tegrabl_a_b_get_bootslot_suffix(slot_suffix, false);
	if (status != TEGRABL_NO_ERROR) {
		return TEGRABL_ERROR(TEGRABL_ERR_INVALID, 1);
	}

	pr_info("%s: slot_suffix = %s\n", __func__, slot_suffix);

	/* Add slot_suffix to cmdline*/
	return tegrabl_snprintf(cmdline, len, "%s=%s ", param, slot_suffix);
}

static int add_ratchet_values(char *cmdline, int len, char *param, void *priv)
{
	uint32_t bct_ratchet = 0;
	uint32_t mb1_ratchet = 0;
	uint32_t mts_ratchet = 0;

	bct_ratchet = SCRATCH_READ(SECURE_RSV98_SCRATCH_0);

	mb1_ratchet = SCRATCH_READ(SECURE_RSV82_SCRATCH_0);

	mts_ratchet = SCRATCH_READ(SECURE_RSV82_SCRATCH_1);

	return tegrabl_snprintf(cmdline, len, "%s=%u.%u.%u ", param,
							bct_ratchet, mb1_ratchet, mts_ratchet);
}

bool is_system_as_root_enabled(void)
{
	tegrabl_error_t err = TEGRABL_NO_ERROR;
	bool ret = true;
	void *fdt;
	int fw_node, fstab_node, sys_node;

	/* Check dtb to see if system as root is enabled */
	err = tegrabl_dt_get_fdt_handle(TEGRABL_DT_BL, &fdt);
	if (err != TEGRABL_NO_ERROR) {
		goto fail;
	}
	err = tegrabl_dt_get_node_with_path(fdt, "/firmware/android", &fw_node);
	if (err != TEGRABL_NO_ERROR) {
		goto fail;
	}
	err = tegrabl_dt_get_node_with_compatible(fdt, fw_node, "android,fstab", &fstab_node);
	if (err != TEGRABL_NO_ERROR) {
		goto fail;
	}
	err = tegrabl_dt_get_node_with_compatible(fdt, fstab_node, "android,system", &sys_node);
	if (err == TEGRABL_NO_ERROR) {
		/* if DT node exists, system as root is disabld hence no extra cmdline param */
		ret = false;
	}
fail:
	return ret;
}

#if defined(CONFIG_ENABLE_SYSTEM_AS_ROOT)
static int add_boot_recovery_info(char *cmdline, int len, char *param,
								  void *priv)
{
	tegrabl_binary_type_t bin_type;
	int ret = -1;
	bool is_system_root_enabled;

	TEGRABL_UNUSED(priv);

	if (!cmdline || !param) {
		goto done;
	}

	is_system_root_enabled = is_system_as_root_enabled();

	if (!is_system_root_enabled) {
		/* there is recovery partition hence no extra cmdline param */
		ret = 0;
		goto done;
	}

	bin_type = tegrabl_get_kernel_type();
	if (bin_type != TEGRABL_BINARY_RECOVERY_KERNEL) {
		ret = tegrabl_snprintf(cmdline, len, "%s init=/init ", param);
	} else {
		ret = tegrabl_snprintf(cmdline, len, "init=/init ");
	}

done:
	return ret;
}
#endif /* CONFIG_ENABLE_SYSTEM_AS_ROOT */
#endif

static struct tegrabl_linuxboot_param extra_params[] = {
	{ "tegraid", add_tegraid, NULL },
	{ "maxcpus", add_maxcpus, NULL },
	{ "ip", add_vdk_ip, NULL },
#if !defined(CONFIG_OS_IS_L4T)
	{ "tegra_keep_boot_clocks", tegrabl_linuxboot_add_string, NULL},
	{ "android.kerneltype", add_kerneltype, NULL },
	{ "androidboot.presilicon", add_presilicon, NULL },
	{ "androidboot.security", add_secure_state, NULL },
	{ "androidboot.bootmode", add_bootmode, NULL },
#if defined(CONFIG_ENABLE_ANDROID_BOOTREASON)
	{ "androidboot.bootreason", add_androidboot_bootreason, NULL },
#endif
#if defined(CONFIG_ENABLE_VERIFIED_BOOT)
	{ "androidboot.verifiedbootstate", add_vb_boot_state, NULL },
	{ "androidboot.vbmeta", add_vbmeta_info, NULL },
#endif
#endif	/* !CONFIG_OS_IS_L4T */
#if defined(CONFIG_ENABLE_A_B_SLOT)
#if !defined(CONFIG_OS_IS_L4T)
	{ "androidboot.slot_suffix", add_boot_slot_suffix, NULL },
	{ "androidboot.ratchetvalues", add_ratchet_values, NULL },
#else
	{ "boot.slot_suffix", add_boot_slot_suffix, NULL },
	{ "boot.ratchetvalues", add_ratchet_values, NULL },
#endif	/* !CONFIG_OS_IS_L4T */
#if defined(CONFIG_ENABLE_SYSTEM_AS_ROOT)
	{ "skip_initramfs", add_boot_recovery_info, NULL },
#endif	/* CONFIG_ENABLE_SYSTEM_AS_ROOT */
#endif	/* CONFIG_ENABLE_A_B_SLOT */
	{ "vpr", tegrabl_linuxboot_add_vpr_info, NULL },
	{ "vpr_resize", tegrabl_linuxboot_add_vprresize_info, NULL },
	{ "bl_prof_dataptr", add_profiler_carveout, NULL},
	{ "sdhci_tegra.en_boot_part_access", tegrabl_linuxboot_add_string, "1" },
#if defined(CONFIG_ENABLE_NVDEC)
	{ "nvdec_enabled", tegrabl_linuxboot_add_nvdec_enabled_info, NULL },
#endif
	{ NULL, NULL, NULL},
};

/* Add reset status under 'chosen/reset' node in DTB */
static tegrabl_error_t add_reset_info(void *fdt, int nodeoffset)
{
	tegrabl_error_t status = TEGRABL_NO_ERROR;
	int err;
	int node;
	uint32_t reset_status = 0;
	tegrabl_rst_source_t rst_source;
	tegrabl_rst_level_t rst_level;

	/* Check if the property is already present under 'chosen' node */
	node = tegrabl_add_subnode_if_absent(fdt, nodeoffset, "reset");
	if (node < 0) {
		status = TEGRABL_ERROR(TEGRABL_ERR_ADD_FAILED, 0);
		goto fail;
	}

	/* Obtain pmc reset reason */
	status = tegrabl_get_rst_status(&rst_source, &rst_level);
	if (status != TEGRABL_NO_ERROR) {
		TEGRABL_SET_HIGHEST_MODULE(status);
		pr_error("Unable to set pmc-reset-reason\n");
		goto fail;
	}

	/* Set 'pmc reset reason' */
	/* RST_SRC[5:2], RST_LVL[1,0]*/
	reset_status = ((rst_source & 0xF) << 2) | (rst_level & 0x3);

	err = fdt_setprop_cell(fdt, node, "pmc-reset-reason", reset_status);
	if (err < 0) {
		pr_error("Unable to set pmc-reset-reason (%s)\n",
				 fdt_strerror(err));
		status = TEGRABL_ERROR(TEGRABL_ERR_ADD_FAILED, 0);
		goto fail;
	}

	/* Set 'pmic reset reason' */
	reset_status = boot_params->pmic_rst_reason;

	err = fdt_setprop_cell(fdt, node, "pmic-reset-reason", reset_status);
	if (err < 0) {
		pr_error("Unable to set pmic-reset-reason (%s)\n",
				 fdt_strerror(err));
		status = TEGRABL_ERROR(TEGRABL_ERR_ADD_FAILED, 2);
		goto fail;
	}

	pr_debug("Updated %s info to DTB\n", "reset");
	return TEGRABL_NO_ERROR;

fail:
	return status;
}

static int get_addr_cells(void *fdt, int nodeoffset, uint32_t *cells)
{
	const uint32_t *prop;

	prop = fdt_getprop(fdt, nodeoffset, "#address-cells", NULL);
	if (prop != NULL) {
		*cells = fdt32_to_cpu(*(uint32_t *)prop);
		return 0;
	}

	return -FDT_ERR_NOTFOUND;
}

#define MAX_T194_CPUS U32(8)
#define MAX_T194_CLUSTERS	U32(4)
static tegrabl_error_t update_cpu_floorsweeping_config(void *fdt, int nodeoffset)
{
	uint32_t cpu = 0;
	uint32_t cluster;
	int offset, prev = 0;
	int cpu_map_offset;
	char cluster_node_str[] = "cluster10";
	uint32_t num_cores = tegrabl_ccplex_nvg_num_cores();
	uint32_t addr_cells;
	int err;

	err = get_addr_cells(fdt, nodeoffset, &addr_cells);
	if (err < 0) {
		pr_error("Couldn't find #address-cells for /cpus\n");
		return TEGRABL_ERROR(TEGRABL_ERR_NOT_FOUND, 0);
	}

	if (addr_cells != 1 && addr_cells != 2) {
		pr_error("Invalid value '%d' for /cpus #address-cells\n", addr_cells);
		return TEGRABL_ERROR(TEGRABL_ERR_INVALID, 0);
	}

	/* Update the correct MPIDR and enable the DT nodes of each enabled CPU;
	 * disable the DT nodes of the floorswept cores.*/
	for (offset = fdt_first_subnode(fdt, nodeoffset);
	     offset > 0;
	     offset = fdt_next_subnode(fdt, offset)) {
		/* enough to accomodate "cpu@0000000000000000\0" */
		char name[4 + 16 + 1];
		const void *prop;
		int len;

		/* skip non-CPU nodes */
		prop = fdt_getprop(fdt, offset, "device_type", &len);
		if (!prop || strcmp(prop, "cpu") != 0)
			continue;

		if (cpu < num_cores) {
			uint32_t value[2], *ptr = value, mpidr;

			mpidr = tegrabl_ccplex_nvg_logical_to_mpidr(cpu);
			mpidr &= 0x00ffffffUL;

			tegrabl_snprintf(name, sizeof(name), "cpu@%x", mpidr);

			err = fdt_set_name(fdt, offset, name);
			if (err < 0) {
				pr_error("failed to set name for /cpus/%s: %s\n",
					 name, fdt_strerror(err));
			}

			if (addr_cells > 1)
				*ptr++ = cpu_to_fdt32(U64_TO_U32_HI(mpidr));

			*ptr++ = cpu_to_fdt32(U64_TO_U32_LO(mpidr));

			len = (ptr - value) * sizeof(*ptr);

			err = fdt_setprop_inplace(fdt, offset, "reg", value, len);
			if (err < 0)
				pr_error("failed to write MPIDR to /cpus/%s: %s\n",
					 name, fdt_strerror(err));
		} else {
			const char *node = fdt_get_name(fdt, offset, &len);

			strncpy(name, node, sizeof(name));

			err = fdt_del_node(fdt, offset);
			if (err < 0)
				pr_error("failed to delete /cpus/%s: %s\n",
					 name, fdt_strerror(err));
			else {
				pr_info("Deleted %s node in DT\n", name);

				/*
				 * Deleting a node is a destructive operation,
				 * which means that offsets will get messed up
				 * and the iteration in the loop will fail.
				 *
				 * Correct for that by rewinding to the last
				 * known subnode offset. Note that this won't
				 * work if what we deleted was the first sub-
				 * node. There's no good way to rewind in that
				 * case because it would break the for-loop.
				 *
				 * However, we should never really get into
				 * that situation because the last CPUs are
				 * always the ones that get floorswept.
				 */
				if (prev > 0)
					offset = prev;
			}
		}

		/*
		 * Keep a reference to the last subnode in case we need to
		 * delete a node and rewind.
		 */
		prev = offset;
		cpu++;
	}

	cpu_map_offset = fdt_subnode_offset(fdt, nodeoffset, "cpu-map");
	if (cpu_map_offset < 0) {
		pr_error("/cpus/cpu-map does not exist\n");
		return TEGRABL_NO_ERROR;
	}

	for (cluster = 0; cluster < MAX_T194_CLUSTERS; cluster++) {
		tegrabl_snprintf(cluster_node_str, 9, "cluster%u", cluster);
		offset = fdt_subnode_offset(fdt, cpu_map_offset, cluster_node_str);

		if ((offset >= 0) && (cluster * 2 >= num_cores)) {
			err = fdt_del_node(fdt, offset);
			if (err < 0) {
				pr_error("Failed to delete /cpus/cpu-map/%s node: %s\n",
					 cluster_node_str, fdt_strerror(err));
				return TEGRABL_ERROR(TEGRABL_ERR_DEL_FAILED, 0);
			}

			pr_info("Deleted cluster%u node in FDT\n", cluster);
		}
	}

	return TEGRABL_NO_ERROR;
}

static tegrabl_error_t update_armpmu_prop(void *fdt,
										  int nodeoffset,
										  uint32_t num_cores,
										  const char *prop_name,
										  uint32_t num_prop)
{
	const uint32_t *temp = NULL;
	uint32_t i, k;
	int dterr;
	uint32_t *buffer = NULL;
	size_t sz_buffer = 0;
	tegrabl_error_t ret_code = TEGRABL_NO_ERROR;

	temp = fdt_getprop(fdt, nodeoffset, prop_name, NULL);
	if (temp == NULL) {
		pr_error("Couldn't find property: %s\n", prop_name);
		ret_code = TEGRABL_ERROR(TEGRABL_ERR_NOT_FOUND, 0);
		goto out;
	}

	sz_buffer = sizeof(uint32_t) * num_cores * num_prop;
	buffer = (uint32_t *)tegrabl_malloc(sz_buffer);
	if (buffer == NULL) {
		pr_error("%d: Failed to allocate memory\n", __LINE__);
		ret_code = TEGRABL_ERROR(TEGRABL_ERR_NO_MEMORY, 0);
		goto out;
	}

	for (i = 0; i < num_cores; ++i) {
		for (k = 0; k < num_prop; ++k) {
			buffer[(i * num_prop) + k] = *(temp + (i * num_prop) + k);
		}
	}

	dterr = fdt_setprop(fdt, nodeoffset, prop_name, buffer, sz_buffer);
	if (dterr < 0) {
		pr_error("Failed to update %s property\n", prop_name);
		ret_code = TEGRABL_ERROR(TEGRABL_ERR_ADD_FAILED, 0);
		goto out_free;
	}

	pr_info("- update property: %s\n", prop_name);

out_free:
	tegrabl_free(buffer);
out:
	return ret_code;
}

#define INTERRUPT_PROP_SIZE	U32(3)
static tegrabl_error_t update_armpmu_floorsweeping_config(void *fdt, int nodeoffset)
{
	tegrabl_error_t err_code = TEGRABL_NO_ERROR;
	uint32_t num_cores = tegrabl_ccplex_nvg_num_cores();

	/*
	 * Modify arm-pmu DT node to ensure element count of properties:
	 * interrupts, interrupt-affinity are equal to number of CPUs.
	 */
	if (num_cores == MAX_T194_CPUS) {
		goto out;
	}

	pr_info("Update arm-pmu in FDT\n");

	/* Update property: interrupts */
	err_code = update_armpmu_prop(fdt, nodeoffset, num_cores, "interrupts", INTERRUPT_PROP_SIZE);
	if (err_code != TEGRABL_NO_ERROR) {
		if (TEGRABL_ERROR_REASON(err_code) == TEGRABL_ERR_NOT_FOUND)
			err_code = TEGRABL_NO_ERROR;

		goto out;
	}

	/* Update property: interrupt-affinity */
	err_code = update_armpmu_prop(fdt, nodeoffset, num_cores, "interrupt-affinity", U32(1));
	if (err_code != TEGRABL_NO_ERROR) {
		if (TEGRABL_ERROR_REASON(err_code) == TEGRABL_ERR_NOT_FOUND)
			err_code = TEGRABL_NO_ERROR;

		goto out;
	}

out:
	return err_code;
}

static tegrabl_error_t update_vpr_info(void *fdt, int nodeoffset)
{
	int node;
	uint64_t size = 0;
	/*
	 * If BL did not carveout vpr and vpr-resize is allowed, then
	 * allow vpr DT node as it is the only way to specify vpr carveout size.
	 */
	if (tegrabl_is_vpr_resize_enabled()) {
		/* Get the vpr size */
		size = boot_params->carveout_info[CARVEOUT_VPR].size;
		if (!size) {
			return TEGRABL_NO_ERROR;
		}
	}

	/* Otherwise, remove vpr DT node. */
	node = fdt_subnode_offset(fdt, nodeoffset, "vpr-carveout");
	if (node < 0) {
		return TEGRABL_NO_ERROR; /* vpr DT node not present. we are good */
	}

	fdt_delprop(fdt, node, "compatible");
	fdt_delprop(fdt, node, "reg");
	fdt_delprop(fdt, node, "size");
	return TEGRABL_NO_ERROR;
}

#define GRANULARITY_4KB 0x1000
#define GRANULARITY_128KB 0x20000

/* Grid of Semaphore for Computer Vision */
static tegrabl_error_t update_cv_gos_info(void *fdt, int nodeoffset)
{
	int dterr, node;
	uint64_t size;
	uint64_t base;
	uint64_t buf[2];

	size = boot_params->carveout_info[CARVEOUT_CV].size;
	if (size == 0LLU) {
		pr_info("warning: CV carveout not allocated\n");
		return 0;
	}

	/* Check if the node is already present */
	node = tegrabl_add_subnode_if_absent(fdt, nodeoffset, "grid-of-semaphores");
	if (node < 0) {
		pr_info("failed to add grid-of-semaphores node\n");
		return TEGRABL_ERROR(TEGRABL_ERR_ADD_FAILED, 0);
	}

	base = boot_params->carveout_info[CARVEOUT_CV].base;
	buf[0] = cpu_to_fdt64(base);
	buf[1] = cpu_to_fdt64(size);

	dterr = fdt_setprop(fdt, node, "reg", buf, sizeof(buf));
	if (dterr < 0) {
		pr_info("failed to add grid-of-semaphores/reg property\n");
		return TEGRABL_ERROR(TEGRABL_ERR_ADD_FAILED, 0);
	}

	return TEGRABL_NO_ERROR;
}

static tegrabl_error_t add_device_info(void *fdt, int nodeoffset)
{
	tegrabl_error_t status = TEGRABL_NO_ERROR;
	const char *dev_info = NULL;
	tegrabl_storage_type_t boot_dev;
	struct tegrabl_device *storage_dev = boot_params->storage_devices;
	uint32_t i, instance;
	int node, err;

	node = tegrabl_add_subnode_if_absent(fdt, nodeoffset, "plugin-manager");
	if (node < 0) {
		status = TEGRABL_ERROR(TEGRABL_ERR_ADD_FAILED, 0);
		return status;
	}

	node = tegrabl_add_subnode_if_absent(fdt, node, "misc-data");
	if (node < 0) {
		status = TEGRABL_ERROR(TEGRABL_ERR_ADD_FAILED, 1);
		return status;
	}

	/* Adding boot device info */
	status = tegrabl_soc_get_bootdev(&boot_dev, &instance);
	if (status != TEGRABL_NO_ERROR) {
		pr_error("Failed to get boot device\n");
		goto storage;
	}

	switch (boot_dev) {
	case TEGRABL_FUSE_BOOT_DEV_SDMMC:
		dev_info = "boot-sdmmc";
		break;
	case TEGRABL_FUSE_BOOT_DEV_SPIFLASH:
		dev_info = "boot-qspi";
		break;
	case TEGRABL_FUSE_BOOT_DEV_UFS:
		dev_info = "boot-ufs";
		break;
	case TEGRABL_FUSE_BOOT_DEV_SATA:
		dev_info = "boot-sata";
		break;
	default:
		pr_warn("Unknown boot device\n");
		goto storage;
	}

	pr_info("Add %s to plugin-manager/misc-data\n", dev_info);
	err = fdt_setprop_cell(fdt, node, dev_info, 1);
	if (err < 0) {
		pr_error("Failed to add boot device info to plugin-manager\n");
		status = TEGRABL_ERROR(TEGRABL_ERR_ADD_FAILED, 2);
		return status;
	}

	/* Adding storage device info */
storage:
	for (i = 0; i < TEGRABL_MAX_STORAGE_DEVICES; i++, storage_dev++) {
		if (storage_dev->type == TEGRABL_BOOT_DEV_NONE) {
			break;
		}

		switch (storage_dev->type) {
		case TEGRABL_BOOT_DEV_SDMMC_BOOT:
			dev_info = "storage-sdmmc";
			break;
		case TEGRABL_BOOT_DEV_QSPI:
			dev_info = "storage-qspi";
			break;
		case TEGRABL_BOOT_DEV_SATA:
			dev_info = "storage-sata";
			break;
		case TEGRABL_BOOT_DEV_UFS:
			dev_info = "storage-ufs";
			break;
		default:
			pr_warn("Unknown storage device\n");
			return TEGRABL_NO_ERROR;
		}

		pr_info("Add %s to plugin-manager/misc-data\n", dev_info);
		err = fdt_setprop_cell(fdt, node, dev_info, 1);
		if (err < 0) {
			pr_error("Failed to add boot device info to plugin-manager\n");
			status = TEGRABL_ERROR(TEGRABL_ERR_ADD_FAILED, 3);
			return status;
		}
	}

	return status;
}

static struct tegrabl_linuxboot_dtnode_info extra_nodes[] = {
	{ "chosen", add_reset_info},
	{ "cpus" , update_cpu_floorsweeping_config },
	{ "arm-pmu", update_armpmu_floorsweeping_config },
	{ "reserved-memory", update_vpr_info },
	{ "reserved-memory", update_cv_gos_info },
	{ "chosen", add_device_info },
	{ NULL, NULL},
};

/**
 * @brief Compare BOM/base of two carveouts
 *
 * @param a Index of the first carveout
 * @param b Index of the second carveout
 *
 * @return -1: if BOM[a] < BOM [b],
 *          0: if BOM[a] == BOM[b], and
 *          1: if BOM[a] > BOM[b]
 */
static int bom_compare(const uint32_t a, const uint32_t b)
{
	static struct tegrabl_carveout_info *p_carveout;

	p_carveout = boot_params->carveout_info;

	if (p_carveout[a].base < p_carveout[b].base)
		return -1;
	else if (p_carveout[a].base > p_carveout[b].base)
		return 1;
	else
		return 0;
}

/**
 * @brief Sorts the given array of carveout indexes in the increasing order of
 * their base address.
 *
 * @param array[] Array of integers to be sorted
 * @param count Number of elements in the array
 */
static void sort(uint32_t array[], int32_t count)
{
	uint32_t val;
	int32_t i;
	int32_t j;

	if (count < 2)
		return;

	for (i = 1; i < count; i++) {
		val = array[i];

		for (j = (i - 1);
			 (j >= 0) && (bom_compare(val, array[j]) < 0);
			 j--) {
			array[j + 1] = array[j];
		}

		array[j + 1] = val;
	}
}

static struct tegrabl_linuxboot_memblock free_dram_block[CARVEOUT_NUM + 1];
static uint32_t free_dram_block_count;

static void calculate_free_dram_regions(void)
{
	carve_out_type_t cotype;
	int32_t i, rgn;
	int32_t count;
	uint64_t cur_start, cur_end;
	uint32_t perm_carveouts[CARVEOUT_NUM];
	static struct tegrabl_carveout_info *p_carveout;

	if (p_carveout != NULL) {
		/* We calculate all free DRAM regions at once,
		 * If called again, just return*/
		return;
	}

	p_carveout = boot_params->carveout_info;
	count = 0;

	/* Prepare a list of permanent DRAM carveouts */
	for (cotype = CARVEOUT_NVDEC; cotype < CARVEOUT_NUM; cotype++) {
		if ((p_carveout[cotype].base < SDRAM_START_ADDRESS) ||
			(p_carveout[cotype].size == 0)) {
			continue;
		}

		switch (cotype) {
		/* Skip the temporary/invalid carveouts */
		case CARVEOUT_MB2:
		case CARVEOUT_CPUBL:
		case CARVEOUT_OS:
		case CARVEOUT_RCM_BLOB:
			break;
		default:
			perm_carveouts[count] = (uint32_t)cotype;
			count++;
			break;
		}
	}

	/* Sort the carveouts in increasing order of their base */
	sort(perm_carveouts, count);

	/* Determine the free regions */
	cur_start = SDRAM_START_ADDRESS;
	rgn = 0;

	for (i = 0; i < count; i++) {
		cur_end = p_carveout[perm_carveouts[i]].base;
		if (cur_end > cur_start) {
			pr_info("[%d] START: 0x%"PRIx64", END: 0x%"PRIx64"\n",
					 rgn, cur_start, cur_end);
			free_dram_block[rgn].base = cur_start;
			free_dram_block[rgn].size = cur_end - cur_start;
			rgn++;
		}
		cur_start = p_carveout[perm_carveouts[i]].base +
			p_carveout[perm_carveouts[i]].size;
	}

	pr_info("dram_block larger than %x\n", SDRAM_START_ADDRESS);

	cur_end = SDRAM_START_ADDRESS +	boot_params->sdram_size;
	if (cur_end > cur_start) {
		pr_info("[%d] START: 0x%"PRIx64", END: 0x%"PRIx64"\n",
				 rgn, cur_start, cur_end);
		free_dram_block[rgn].base = cur_start;
		free_dram_block[rgn].size = cur_end - cur_start;
		rgn++;
	}

	free_dram_block_count = rgn;
}

tegrabl_error_t tegrabl_linuxboot_helper_get_info(tegrabl_linux_boot_info_t info,
												  const void *in_data,
												  void *out_data)
{
	struct tegrabl_linuxboot_memblock *memblock;
	uint32_t temp32;
	uint64_t addr;
	uint32_t mailbox_addr;
	tegrabl_error_t err = TEGRABL_NO_ERROR;

	TEGRABL_UNUSED(mailbox_addr);

	/* Note: in_data is not mandatory for all info-types */
	if (out_data == NULL) {
		pr_error("out_data is NULL\n");
		err = TEGRABL_ERROR(TEGRABL_ERR_INVALID, 0);
		goto fail;
	}

	pr_debug("Got request for %u info\n", info);

	switch (info) {
	case TEGRABL_LINUXBOOT_INFO_EXTRA_CMDLINE_PARAMS:
		pr_debug("%s: extra_params: %p\n", __func__, extra_params);
		*(struct tegrabl_linuxboot_param **)out_data = extra_params;
		break;

	case TEGRABL_LINUXBOOT_INFO_EXTRA_DT_NODES:
		pr_debug("%s: extra_nodes: %p\n", __func__, extra_nodes);
		*(struct tegrabl_linuxboot_dtnode_info **)out_data = extra_nodes;
		break;

	case TEGRABL_LINUXBOOT_INFO_DEBUG_CONSOLE:
		if (boot_params->verbose == 0U) {
			*(tegrabl_linuxboot_debug_console_t *)out_data = TEGRABL_LINUXBOOT_DEBUG_CONSOLE_NONE;

		} else if (boot_params->enable_combined_uart == 1U) {
			*(tegrabl_linuxboot_debug_console_t *)out_data = TEGRABL_LINUXBOOT_DEBUG_CONSOLE_COMB_UART;

		} else {
			*(tegrabl_linuxboot_debug_console_t *)out_data =
				TEGRABL_LINUXBOOT_DEBUG_CONSOLE_UARTA + boot_params->uart_instance;
		}
		pr_debug("%s: console = %u\n", __func__, *((tegrabl_linuxboot_debug_console_t *)out_data));
		break;

	case TEGRABL_LINUXBOOT_INFO_EARLYUART_BASE:
		err = tegrabl_uart_get_address(boot_params->uart_instance, &addr);
		if (err == TEGRABL_NO_ERROR) {
			*(uint64_t *)out_data = addr;
		} else {
			*(uint64_t *)out_data = 0;
		}
		pr_debug("earlyuart_base = 0x%"PRIx64"\n", *((uint64_t *)out_data));
		break;

#if defined(CONFIG_ENABLE_COMB_UART)
	case TEGRABL_LINUXBOOT_INFO_TCU_MBOX_ADDR:
		if (boot_params->enable_combined_uart == 1U) {
			err = tegrabl_comb_uart_get_mailbox_port_addr(COMB_UART_MAILBOX_TX_PORT, &mailbox_addr);
			if (err != TEGRABL_NO_ERROR) {
				goto fail;
			}
			*(uint32_t *)out_data = mailbox_addr;
		} else {
			pr_error("combined uart is not enabled\n");
			err = TEGRABL_ERROR(TEGRABL_ERR_NOT_SUPPORTED, 0);
			goto fail;
		}
		pr_debug("combined uart mailbox tx addr = 0x%08x\n", *((uint32_t *)out_data));
		break;
#endif

	case TEGRABL_LINUXBOOT_INFO_CARVEOUT:
		if (in_data == NULL) {
			err = TEGRABL_ERROR(TEGRABL_ERR_INVALID, 0);
			goto fail;
		}
		temp32 = *((uint32_t *)in_data);
		memblock = (struct tegrabl_linuxboot_memblock *)out_data;
		switch (temp32) {
		case TEGRABL_LINUXBOOT_CARVEOUT_VPR:
			memblock->base =
				boot_params->carveout_info[CARVEOUT_VPR].base;
			memblock->size =
				boot_params->carveout_info[CARVEOUT_VPR].size;
			break;
		case TEGRABL_LINUXBOOT_CARVEOUT_BPMPFW:
			memblock->base =
				boot_params->carveout_info[CARVEOUT_BPMP].base;
			memblock->size =
				boot_params->carveout_info[CARVEOUT_BPMP].size;
			break;
		case TEGRABL_LINUXBOOT_CARVEOUT_LP0:
			memblock->base =
				boot_params->carveout_info[CARVEOUT_SC7_RF].base;
			memblock->size =
				boot_params->carveout_info[CARVEOUT_SC7_RF].size;
			break;
		default:
			memblock->base = 0x0;
			memblock->size = 0x0;
		}
		break;

	case TEGRABL_LINUXBOOT_INFO_MEMORY:
		if (in_data == NULL) {
			err = TEGRABL_ERROR(TEGRABL_ERR_INVALID, 0);
			goto fail;
		}
		temp32 = *((uint32_t *)in_data);
		memblock = (struct tegrabl_linuxboot_memblock *)out_data;

		calculate_free_dram_regions();

		if (temp32 >= free_dram_block_count) {
			memblock->base = 0;
			memblock->size = 0;
		} else {
			memblock->base = free_dram_block[temp32].base;
			memblock->size = free_dram_block[temp32].size;
		}

		pr_debug("%s: memblock(%u) (base:0x%"PRIx64", size:0x%"PRIx64")\n",
				 __func__, *((uint32_t *)in_data),
				 memblock->base, memblock->size);
		break;

	case TEGRABL_LINUXBOOT_INFO_INITRD:
		memblock = (struct tegrabl_linuxboot_memblock *)out_data;
		tegrabl_get_ramdisk_info(&memblock->base, &memblock->size);
		pr_debug("%s: ramdisk (base:0x%lx, size:0x%lx)\n",
				 __func__, memblock->base, memblock->size);
		break;

	case TEGRABL_LINUXBOOT_INFO_BOOTIMAGE_CMDLINE:
		*(char **)out_data = tegrabl_get_bootimg_cmdline();
		break;

	case TEGRABL_LINUXBOOT_INFO_SECUREOS:
		*(uint32_t *)out_data = boot_params->secureos_type;
		break;

	case TEGRABL_LINUXBOOT_INFO_BOARD:
	default:
		err = TEGRABL_ERROR(TEGRABL_ERR_NOT_SUPPORTED, 0);
		break;
	};

fail:
	return err;
}

#ifdef CONFIG_ENABLE_STAGED_SCRUBBING
static struct tegrabl_linuxboot_memblock unscrubbed_dram_block[CARVEOUT_NUM + 1];
static uint32_t unscrubbed_dram_block_count;

static void calculate_unscrubbed_dram_regions(void)
{
	carve_out_type_t cotype;
	int32_t i, rgn;
	int32_t count;
	uint32_t scrubbed_carveouts[CARVEOUT_NUM];
	uint64_t cur_start, cur_end;

	static struct tegrabl_carveout_info *p_carveout;

	if (p_carveout != NULL) {
		/* We calculate all unscrubbed DRAM regions at once,
		 * If called again, just return*/
		return;
	}

	p_carveout = boot_params->carveout_info;
	count = 0;

	/* Prepare a list of permanent DRAM carveouts */
	for (cotype = CARVEOUT_NVDEC; cotype < CARVEOUT_NUM; cotype++) {
		if ((p_carveout[cotype].base < SDRAM_START_ADDRESS) ||
			(p_carveout[cotype].size == 0)) {
			continue;
		}

		/* Skip already scrubbed carveouts */
		switch (cotype) {
		/* Scrubbed in MB1 */
		case CARVEOUT_NVDEC:
		case CARVEOUT_WPR1:
		case CARVEOUT_WPR2:
		case CARVEOUT_TSECA:
		case CARVEOUT_TSECB:
		case CARVEOUT_SC7_RF:
		case CARVEOUT_SE_SC7:
		case CARVEOUT_MTS:
		case CARVEOUT_MB2:
		case CARVEOUT_MISC:
		case CARVEOUT_ECC_TEST:
		case CARVEOUT_TZDRAM:
		case CARVEOUT_OEM_SC7:
		/* Scrubbed in MB2 */
		case CARVEOUT_BPMP:
		case CARVEOUT_APE:
		case CARVEOUT_SPE:
		case CARVEOUT_SCE:
		case CARVEOUT_APR:
		case CARVEOUT_RCE:
		case CARVEOUT_IPC_SE_TSEC:
		case CARVEOUT_IPC_SE_SPE_SCE_BPMP:
		case CARVEOUT_CAMERA_TASK:
		case CARVEOUT_SMMU:
		case CARVEOUT_CPUBL:
#if defined(CONFIG_ENABLE_WAR_CBOOT_STAGED_SCRUBBING)
		case CARVEOUT_OS:
#endif
			pr_debug("PRE scrubbed \t co: [%d] START: 0x%"PRIx64", END: 0x%"PRIx64"\n",
					cotype, p_carveout[cotype].base, p_carveout[cotype].base + p_carveout[cotype].size);
			scrubbed_carveouts[count] = (uint32_t)cotype;
			count++;
			break;
		default:
			break;
		}
	}

	/* Sort the scrubbed carveouts in increasing order of their base */
	sort(scrubbed_carveouts, count);

	/* Determine the unscrubbed DRAM regions */
	cur_start = SDRAM_START_ADDRESS;
	rgn = 0;

	for (i = 0; i < count; i++) {
		cur_end = p_carveout[scrubbed_carveouts[i]].base;
		if (cur_end > cur_start) {
			pr_debug("[%d] unscrubbed region START: 0x%"PRIx64", END: 0x%"PRIx64"\n",
					 rgn, cur_start, cur_end);
			unscrubbed_dram_block[rgn].base = cur_start;
			unscrubbed_dram_block[rgn].size = cur_end - cur_start;
			rgn++;
		}
		cur_start = p_carveout[scrubbed_carveouts[i]].base +
			p_carveout[scrubbed_carveouts[i]].size;
	}

	cur_end = SDRAM_START_ADDRESS + boot_params->sdram_size;
	if (cur_end > cur_start) {
		pr_debug("[%d] unscrubbed region START: 0x%"PRIx64", END: 0x%"PRIx64"\n",
				 rgn, cur_start, cur_end);
		unscrubbed_dram_block[rgn].base = cur_start;
		unscrubbed_dram_block[rgn].size = cur_end - cur_start;
		rgn++;
	}
	unscrubbed_dram_block_count = rgn;
}

tegrabl_error_t dram_staged_scrub(void)
{
	tegrabl_error_t err = TEGRABL_NO_ERROR;
	uint32_t i = 0;

	calculate_unscrubbed_dram_regions();

	for (i = 0; (i < unscrubbed_dram_block_count); i++) {
		/* Scrub region */
		err = tegrabl_sdram_qual_engine_init(unscrubbed_dram_block[i].base, unscrubbed_dram_block[i].size);
		if (err != TEGRABL_NO_ERROR) {
			break;
		}
		err = tegrabl_sdram_qual_engine_wait_for_idle();
		if (err != TEGRABL_NO_ERROR) {
			break;
		}
	}

	if (err != TEGRABL_NO_ERROR) {
		pr_error("dram scrub failed\n");
		pr_debug("last iteration: %d\n", i);
	} else {
		pr_info("dram scrub successful\n");
	}
	return err;

}
#endif /* CONFIG_ENABLE_STAGED_SCRUBBING */

tegrabl_error_t tegrabl_get_nct_load_addr(void **load_addr)
{
	static void *nct_load_addr;
	tegrabl_error_t err = TEGRABL_NO_ERROR;

	if (nct_load_addr == NULL) {
		*load_addr = tegrabl_malloc(NCT_MAX_SIZE);
		if (*load_addr == NULL) {
			pr_error("Failed to allocate memory to load NCT image\n");
			err = TEGRABL_ERROR(TEGRABL_ERR_NO_MEMORY, 0);
			goto fail;
		}
		nct_load_addr = *load_addr;
	} else {
		*load_addr = nct_load_addr;
	}
	pr_trace("%s(): %u, NCT load addr: %p\n", __func__, __LINE__, *load_addr);

fail:
	return err;
}

tegrabl_error_t tegrabl_get_boot_img_load_addr(void **load_addr)
{
	static void *boot_img_load_addr;
	tegrabl_error_t err = TEGRABL_NO_ERROR;

	if (boot_img_load_addr == NULL) {
		*load_addr = tegrabl_alloc_align(TEGRABL_HEAP_DMA,
			BOOT_IMAGE_ALIGNMENT, BOOT_IMAGE_MAX_SIZE);
		if (*load_addr == NULL) {
			pr_error("Failed to allocate memory (0x%08x) to load boot image\n", BOOT_IMAGE_MAX_SIZE);
			err = TEGRABL_ERROR(TEGRABL_ERR_NO_MEMORY, 0);
			goto fail;
		}
		boot_img_load_addr = *load_addr;
	} else {
		*load_addr = boot_img_load_addr;
	}
	pr_trace("%s(): %u, boot image load addr: %p\n", __func__, __LINE__, *load_addr);

fail:
	return err;
}

uint64_t tegrabl_get_kernel_load_addr(void)
{
	uint64_t kernel_load_addr;

	pr_trace("%s(): %u\n", __func__, __LINE__);

	kernel_load_addr = boot_params->carveout_info[CARVEOUT_OS].base;
	kernel_load_addr = ROUND_UP(kernel_load_addr, KERNEL_ALIGNMENT);
	pr_trace("%s(): %u, kernel load addr: 0x%"PRIx64"\n", __func__, __LINE__, kernel_load_addr);

	/* Update next free addr ptr */
	if (os_carveout_next_free_addr == 0ULL) {
		os_carveout_next_free_addr = kernel_load_addr + MAX_KERNEL_IMAGE_SIZE;
	}

	/* Load kernel at text offset */
	return kernel_load_addr + 0x80000ULL;
}

uint64_t tegrabl_get_dtb_load_addr(void)
{
	uint64_t dtb_load_addr;

	pr_trace("%s(): %u\n", __func__, __LINE__);

	/* Get next free addr ptr */
	if (os_carveout_next_free_addr == 0ULL) {
		tegrabl_get_kernel_load_addr();
	}

	dtb_load_addr = ROUND_UP((uintptr_t)os_carveout_next_free_addr, DTB_ALIGNMENT);
	pr_trace("%s(): %u, dtb load addr: 0x%"PRIx64"\n", __func__, __LINE__, dtb_load_addr);

	/* Update next free addr ptr */
	os_carveout_next_free_addr = dtb_load_addr + DTB_MAX_SIZE;

	return dtb_load_addr;
}

uint64_t tegrabl_get_ramdisk_load_addr(void)
{
	uint64_t ramdisk_load_addr;

	pr_trace("%s(): %u\n", __func__, __LINE__);

	/* Update next free addr ptr */
	if (os_carveout_next_free_addr == 0ULL) {
		tegrabl_get_kernel_load_addr();
	}

	ramdisk_load_addr = ROUND_UP(os_carveout_next_free_addr, RAMDISK_ALIGNMENT);
	pr_trace("%s(): %u, ramdisk load addr: 0x%"PRIx64"\n", __func__, __LINE__, ramdisk_load_addr);

	/* Update next free addr ptr */
	os_carveout_next_free_addr = ramdisk_load_addr + RAMDISK_MAX_SIZE;

	return ramdisk_load_addr;
}

#if defined(CONFIG_ENABLE_L4T_RECOVERY)
tegrabl_error_t tegrabl_get_recovery_img_load_addr(void **load_addr)
{
	return tegrabl_get_boot_img_load_addr(load_addr);
}
#endif

bool tegrabl_do_ratchet_check(uint8_t bin_type, void * const addr)
{
	NvBootComponentHeader *bch_addr;
	uint8_t num_bins;
	uint8_t i;
	uint8_t fw_ratchet_idx;
	uint8_t min_ratchet_level;
	uint8_t fw_ratchet_level;
	bool status;

	bch_addr = addr;
	num_bins = bch_addr->NumBinaries2;

	fw_ratchet_idx = oem_fw_bin_type_mapping[bin_type];
	min_ratchet_level = boot_params->min_ratchet[fw_ratchet_idx];

	for (i = 0; i < num_bins; i++) {
		fw_ratchet_level = bch_addr->Stage2Components[i].Version.RatchetLevel;
		pr_trace("%u: min ratchet level: %u, fw ratchet level: %u\n", i, min_ratchet_level, fw_ratchet_level);
		if ((min_ratchet_level != 0) && (min_ratchet_level > fw_ratchet_level)) {
			pr_error("Binary (%u), ratchet version mismatch, expected (>%u), current (%u)\n",
					 bin_type, min_ratchet_level, fw_ratchet_level);
			status = false;
			goto fail;
		}
	}
	status = true;

fail:
	return status;
}
