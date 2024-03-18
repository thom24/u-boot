/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) 2018-2019 Texas Instruments Incorporated - http://www.ti.com/
 *	Lokesh Vutla <lokeshvutla@ti.com>
 */
#ifndef _ASM_ARCH_J721E_SPL_H_
#define _ASM_ARCH_J721E_SPL_H_

/* With BootMode B = 0 */
#include <linux/bitops.h>
#define BOOT_DEVICE_HYPERFLASH		0x00
#define BOOT_DEVICE_OSPI		0x01
#define BOOT_DEVICE_QSPI		0x02
#define BOOT_DEVICE_SPI			0x03
#define BOOT_DEVICE_ETHERNET		0x04
#define BOOT_DEVICE_I2C			0x06
#define BOOT_DEVICE_UART		0x07
#define BOOT_DEVICE_NOR			BOOT_DEVICE_HYPERFLASH

/* With BootMode B = 1 */
#define BOOT_DEVICE_MMC2		0x10
#define BOOT_DEVICE_MMC1		0x11
#define BOOT_DEVICE_DFU			0x12
#define BOOT_DEVICE_UFS			0x13
#define BOOT_DEVIE_GPMC			0x14
#define BOOT_DEVICE_PCIE		0x15
#define BOOT_DEVICE_XSPI		0x16
#define BOOT_DEVICE_RAM			0x17
#define BOOT_DEVICE_MMC2_2		0xFF /* Invalid value */

/* Backup boot modes with MCU Only = 0 */
#define BACKUP_BOOT_DEVICE_RAM		0x0
#define BACKUP_BOOT_DEVICE_USB		0x1
#define BACKUP_BOOT_DEVICE_UART		0x3
#define BACKUP_BOOT_DEVICE_ETHERNET	0x4
#define BACKUP_BOOT_DEVICE_MMC2		0x5
#define BACKUP_BOOT_DEVICE_SPI		0x6
#define BACKUP_BOOT_DEVICE_I2C		0x7

#define BOOT_MODE_B_SHIFT		4
#define BOOT_MODE_B_MASK		BIT(4)

#define K3_PRIMARY_BOOTMODE		0x0
#define K3_BACKUP_BOOTMODE		0x1

/* Starting buffer address is 1MB before the stack address in DDR */
#define BUFFER_ADDR (CONFIG_SPL_STACK_R_ADDR - SZ_1M)

/* This is actually the whole size of the SRAM */
#define BL31_SIZE    0x20000

/* This address belongs to a reserved memory region for the point of view of
 * Linux, U-boot SPL must use the same address to restore TF-A and resume
 * entry point address
 */
#define LPM_SAVE		0xA6000000
#define LPM_BL31		LPM_SAVE
#define LPM_BL31_START		LPM_BL31 + BL31_SIZE
#define LPM_BL31_SIZE		LPM_BL31_START + 4
#define LPM_DM			LPM_BL31_SIZE  + 4

/* Check if the copy of TF-A and DM-Firmware in DRAM does not overlap an
 * over memory section.
 * The resume address of TF-A is also saved in DRAM.
 * At build time we don't know the DM-Firmware size, so we keep 512k to
 * save it.
 */
#if defined(CONFIG_SPL_BUILD) && defined(CONFIG_TARGET_J7200_R5_EVM)
#if ((LPM_DM + SZ_512K) > BUFFER_ADDR)
#error Not enough space to save DM-Firmware and TF-A for S2R
#endif
#endif

#define BL31_MAGIC_SUSPEND 0xA5A5A5A5

#endif
