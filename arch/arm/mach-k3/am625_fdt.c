// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright 2023 Toradex - https://www.toradex.com/
 */

#include <asm/hardware.h>
#include <asm/global_data.h>
#include "common_fdt.h"
#include <fdt_support.h>
#include <linux/printk.h>

DECLARE_GLOBAL_DATA_PTR;

static void fdt_fixup_cores_nodes_am625(void *blob, int core_nr)
{
	char node_path[32];

	if (core_nr < 1)
		return;

	for (; core_nr < 4; core_nr++) {
		snprintf(node_path, sizeof(node_path), "/cpus/cpu@%d", core_nr);
		fdt_del_node_path(blob, node_path);
		snprintf(node_path, sizeof(node_path), "/cpus/cpu-map/cluster0/core%d", core_nr);
		fdt_del_node_path(blob, node_path);
		snprintf(node_path, sizeof(node_path), "/bus@f0000/watchdog@e0%d0000", core_nr);
		fdt_del_node_path(blob, node_path);
	}
}

static void fdt_fixup_gpu_nodes_am625(void *blob, int has_gpu)
{
	if (!has_gpu) {
		fdt_del_node_path(blob, "/bus@f0000/gpu@fd00000");
		fdt_del_node_path(blob, "/bus@f0000/watchdog@e0f0000");
	}
}

static void fdt_fixup_pru_node_am625(void *blob, int has_pru)
{
	if (!has_pru)
		fdt_del_node_path(blob, "/bus@f0000/pruss@30040000");
}

/*
 * Das U-Boot, having initialized DRAM in earlier stages of boot, would
 * know its capacity and therefore should inform Linux via updating the
 * device-tree blob before jumping to Linux.
 *
 * This helps our distrobutions remain agnostic to the hardware they
 * find themselves running on. Ideally we should be creating the memory{}
 * node ourself at U-Boot runtime to avoid any confusion for devs.
 */
static void fdt_fixup_memory_node_am625(void *blob)
{
	u64 s[CONFIG_NR_DRAM_BANKS];
	u64 e[CONFIG_NR_DRAM_BANKS];
	u64 t = gd->ram_size;
	int i = 0;

	pr_info("adjusting ram size to: %08llx\n", t);
	for (; i < CONFIG_NR_DRAM_BANKS; i++) {
		s[i] = gd->bd->bi_dram[i].start;
		e[i] = gd->bd->bi_dram[i].size;

		if (e[i] > t)
			e[i] = t;
		t -= e[i];

		pr_debug("bank %02d start: %08llx\n", i, s[i]);
		pr_debug("bank %02d size : %08llx\n", i, e[i]);
	}

	if (fdt_fixup_memory_banks(blob, s, e, CONFIG_NR_DRAM_BANKS))
		pr_err("Oops: failed to resize the memory{} node\n");
}

int ft_system_setup(void *blob, struct bd_info *bd)
{
	fdt_fixup_cores_nodes_am625(blob, k3_get_core_nr());
	fdt_fixup_gpu_nodes_am625(blob, k3_has_gpu());
	fdt_fixup_pru_node_am625(blob, k3_has_pru());
	fdt_fixup_reserved(blob, "tfa", CONFIG_K3_ATF_LOAD_ADDR, 0x80000);
	fdt_fixup_reserved(blob, "optee", CONFIG_K3_OPTEE_LOAD_ADDR, 0x1800000);
	fdt_fixup_memory_node_am625(blob);

	return 0;
}
