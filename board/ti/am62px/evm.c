// SPDX-License-Identifier: GPL-2.0+
/*
 * Board specific initialization for AM62Px platforms
 *
 * Copyright (C) 2023 Texas Instruments Incorporated - https://www.ti.com/
 *
 */

#include <asm/arch/hardware.h>
#include <asm/io.h>
#include <dm/uclass.h>
#include <env.h>
#include <fdt_support.h>
#include <spl.h>

#include "../common/k3-ddr-init.h"

int board_init(void)
{
	return 0;
}

#if defined(CONFIG_SPL_BUILD)
void spl_perform_fixups(struct spl_image_info *spl_image)
{
	if (IS_ENABLED(CONFIG_K3_DDRSS)) {
		if (IS_ENABLED(CONFIG_K3_INLINE_ECC))
			fixup_ddr_driver_for_ecc(spl_image);
	} else {
		fixup_memory_node(spl_image);
	}
}
#endif
