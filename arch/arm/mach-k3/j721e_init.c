// SPDX-License-Identifier: GPL-2.0+
/*
 * J721E: SoC specific initialization
 *
 * Copyright (C) 2018-2019 Texas Instruments Incorporated - http://www.ti.com/
 *	Lokesh Vutla <lokeshvutla@ti.com>
 */

#include <common.h>
#include <init.h>
#include <spl.h>
#include <asm/io.h>
#include <asm/armv7_mpu.h>
#include <asm/arch/hardware.h>
#include "sysfw-loader.h"
#include "common.h"
#include <linux/soc/ti/ti_sci_protocol.h>
#include <dm.h>
#include <dm/uclass-internal.h>
#include <dm/pinctrl.h>
#include <dm/root.h>
#include <fdtdec.h>
#include <mmc.h>
#include <remoteproc.h>
#include <linux/delay.h>

#ifdef CONFIG_K3_LOAD_SYSFW
struct fwl_data cbass_hc_cfg0_fwls[] = {
#if defined(CONFIG_TARGET_J721E_R5_EVM)
	{ "PCIE0_CFG", 2560, 8 },
	{ "PCIE1_CFG", 2561, 8 },
	{ "USB3SS0_CORE", 2568, 4 },
	{ "USB3SS1_CORE", 2570, 4 },
	{ "EMMC8SS0_CFG", 2576, 4 },
	{ "UFS_HCI0_CFG", 2580, 4 },
	{ "SERDES0", 2584, 1 },
	{ "SERDES1", 2585, 1 },
#elif defined(CONFIG_TARGET_J7200_R5_EVM)
	{ "PCIE1_CFG", 2561, 7 },
#endif
}, cbass_hc0_fwls[] = {
#if defined(CONFIG_TARGET_J721E_R5_EVM)
	{ "PCIE0_HP", 2528, 24 },
	{ "PCIE0_LP", 2529, 24 },
	{ "PCIE1_HP", 2530, 24 },
	{ "PCIE1_LP", 2531, 24 },
#endif
}, cbass_rc_cfg0_fwls[] = {
	{ "EMMCSD4SS0_CFG", 2380, 4 },
}, cbass_rc0_fwls[] = {
	{ "GPMC0", 2310, 8 },
}, infra_cbass0_fwls[] = {
	{ "PLL_MMR0", 8, 26 },
	{ "CTRL_MMR0", 9, 16 },
}, mcu_cbass0_fwls[] = {
	{ "MCU_R5FSS0_CORE0", 1024, 4 },
	{ "MCU_R5FSS0_CORE0_CFG", 1025, 2 },
	{ "MCU_R5FSS0_CORE1", 1028, 4 },
	{ "MCU_FSS0_CFG", 1032, 12 },
	{ "MCU_FSS0_S1", 1033, 8 },
	{ "MCU_FSS0_S0", 1036, 8 },
	{ "MCU_PSROM49152X32", 1048, 1 },
	{ "MCU_MSRAM128KX64", 1050, 8 },
	{ "MCU_CTRL_MMR0", 1200, 8 },
	{ "MCU_PLL_MMR0", 1201, 3 },
	{ "MCU_CPSW0", 1220, 2 },
}, wkup_cbass0_fwls[] = {
	{ "WKUP_CTRL_MMR0", 131, 16 },
};
#endif

static void ctrl_mmr_unlock(void)
{
	/* Unlock all WKUP_CTRL_MMR0 module registers */
	mmr_unlock(WKUP_CTRL_MMR0_BASE, 0);
	mmr_unlock(WKUP_CTRL_MMR0_BASE, 1);
	mmr_unlock(WKUP_CTRL_MMR0_BASE, 2);
	mmr_unlock(WKUP_CTRL_MMR0_BASE, 3);
	mmr_unlock(WKUP_CTRL_MMR0_BASE, 4);
	mmr_unlock(WKUP_CTRL_MMR0_BASE, 6);
	mmr_unlock(WKUP_CTRL_MMR0_BASE, 7);

	/* Unlock all MCU_CTRL_MMR0 module registers */
	mmr_unlock(MCU_CTRL_MMR0_BASE, 0);
	mmr_unlock(MCU_CTRL_MMR0_BASE, 1);
	mmr_unlock(MCU_CTRL_MMR0_BASE, 2);
	mmr_unlock(MCU_CTRL_MMR0_BASE, 3);
	mmr_unlock(MCU_CTRL_MMR0_BASE, 4);

	/* Unlock all CTRL_MMR0 module registers */
	mmr_unlock(CTRL_MMR0_BASE, 0);
	mmr_unlock(CTRL_MMR0_BASE, 1);
	mmr_unlock(CTRL_MMR0_BASE, 2);
	mmr_unlock(CTRL_MMR0_BASE, 3);
	mmr_unlock(CTRL_MMR0_BASE, 5);
	if (soc_is_j721e())
		mmr_unlock(CTRL_MMR0_BASE, 6);
	mmr_unlock(CTRL_MMR0_BASE, 7);
}

#if defined(CONFIG_K3_LOAD_SYSFW)
void k3_mmc_stop_clock(void)
{
	if (spl_boot_device() == BOOT_DEVICE_MMC1) {
		struct mmc *mmc = find_mmc_device(0);

		if (!mmc)
			return;

		mmc->saved_clock = mmc->clock;
		mmc_set_clock(mmc, 0, true);
	}
}

void k3_mmc_restart_clock(void)
{
	if (spl_boot_device() == BOOT_DEVICE_MMC1) {
		struct mmc *mmc = find_mmc_device(0);

		if (!mmc)
			return;

		mmc_set_clock(mmc, mmc->saved_clock, false);
	}
}
#endif

/*
 * This uninitialized global variable would normal end up in the .bss section,
 * but the .bss is cleared between writing and reading this variable, so move
 * it to the .data section.
 */
u32 bootindex __section(".data");
static struct rom_extended_boot_data bootdata __section(".data");

static void store_boot_info_from_rom(void)
{
	bootindex = *(u32 *)(CONFIG_SYS_K3_BOOT_PARAM_TABLE_INDEX);
	memcpy(&bootdata, (uintptr_t *)ROM_EXTENDED_BOOT_DATA_INFO,
	       sizeof(struct rom_extended_boot_data));
}

#ifdef CONFIG_SPL_OF_LIST
void do_dt_magic(void)
{
	int ret, rescan, mmc_dev = -1;
	static struct mmc *mmc;

	/* Perform board detection */
	do_board_detect();

	/*
	 * Board detection has been done.
	 * Let us see if another dtb wouldn't be a better match
	 * for our board
	 */
	if (IS_ENABLED(CONFIG_CPU_V7R)) {
		ret = fdtdec_resetup(&rescan);
		if (!ret && rescan) {
			dm_uninit();
			dm_init_and_scan(true);
		}
	}

	/*
	 * Because of multi DTB configuration, the MMC device has
	 * to be re-initialized after reconfiguring FDT inorder to
	 * boot from MMC. Do this when boot mode is MMC and ROM has
	 * not loaded SYSFW.
	 */
	switch (spl_boot_device()) {
	case BOOT_DEVICE_MMC1:
		mmc_dev = 0;
		break;
	case BOOT_DEVICE_MMC2:
	case BOOT_DEVICE_MMC2_2:
		mmc_dev = 1;
		break;
	}

	if (mmc_dev > 0 && !is_rom_loaded_sysfw(&bootdata)) {
		ret = mmc_init_device(mmc_dev);
		if (!ret) {
			mmc = find_mmc_device(mmc_dev);
			if (mmc) {
				ret = mmc_init(mmc);
				if (ret) {
					printf("mmc init failed with error: %d\n", ret);
				}
			}
		}
	}
}
#endif

#if defined(CONFIG_SPL_BUILD) && defined(CONFIG_TARGET_J7200_R5_EVM)

void __weak board_set_resuming(void) {}

#if 0
#define CSL_NAVSS0_SPINLOCK_BASE                                    (0x30e00000UL)
#define SOC_NUM_SPINLOCKS                                           (256U)
#define MMR_LOCK0_KICK0                                             (0x01008)
#define MMR_LOCK0_KICK0_UNLOCK_VAL                                  (0x68EF3490)
#define MMR_LOCK0_KICK1_UNLOCK_VAL                                  (0xD172BC5A)
#define SOC_LOCK_MMR_UNLOCK                                         (0U)
#define IO_TIMEOUT                                                  (150)

/* This only gets used to access registers. The original version from the PDK
 * handled the full memory range and had redirects.
 */
static volatile uint32_t *mkptr(uint32_t base, uint32_t offset)
{
	return (volatile uint32_t*)(base+offset);
}

static void busy_wait_10us(void)
{
	volatile unsigned int i = 0;
	for(i = 0; i < IO_TIMEOUT; i++);
}

/* SOC_lock, SOC_unlock, Lpm_mmr_isLocked and Lpm_mmr_unlock are extracted from
 * the PDK lpm.
 */
static int32_t SOC_lock(uint32_t lockNum)
{
	volatile uint32_t *spinLockReg;

	if (lockNum >= SOC_NUM_SPINLOCKS)
		return -1;

	spinLockReg = mkptr(CSL_NAVSS0_SPINLOCK_BASE, 0x800);

	while (spinLockReg[lockNum] != 0x0);

	return 0;
}

static int32_t SOC_unlock(uint32_t lockNum)
{
	volatile uint32_t *spinLockReg;

	if (lockNum >= SOC_NUM_SPINLOCKS)
		return -1;

	spinLockReg = mkptr(CSL_NAVSS0_SPINLOCK_BASE, 0x800);
	spinLockReg[lockNum] = 0x0;

	return 0;
}

static int Lpm_mmr_isLocked(uintptr_t base, uint32_t partition)
{
	volatile uint32_t * lock = mkptr(base, partition * 0x4000 + MMR_LOCK0_KICK0);

	return (*lock & 0x1u) ? 0 : 1;
}

static void Lpm_mmr_unlock(uintptr_t base, uint32_t partition)
{
	volatile uint32_t * lock = mkptr(base, partition * 0x4000 + MMR_LOCK0_KICK0);

	if (!Lpm_mmr_isLocked(base, partition)) {
		return;
	} else {
		SOC_lock(SOC_LOCK_MMR_UNLOCK);
		lock[0] = MMR_LOCK0_KICK0_UNLOCK_VAL;
		lock[1] = MMR_LOCK0_KICK1_UNLOCK_VAL;
		SOC_unlock(SOC_LOCK_MMR_UNLOCK);
	}
}
#endif

/* In this function, we detect resume using DMSC registers and exit MAIN and MCU
 * isolation. This has for side-effect to make MAIN_UART0 available again.
 */
static void spl_exit_ioret(void)
{
	u32 val;

	/* lpm_io_retention uses those registers to detect resume */
	if (readl(CSL_STD_FW_WKUP_DMSC0_PWRCTRL_0_DMSC_PWR_MMR_PWR_START + CSL_PMMCCONTROL_PMCTRL_IO) & 0x2000000 ||
	    readl(CSL_STD_FW_WKUP_DMSC0_PWRCTRL_0_DMSC_PWR_MMR_PWR_START + CSL_PMMCCONTROL_PMCTRL_DDR) & 0x2000000)
		board_set_resuming();

	/* Debug print which pin triggered the wakeup event */
	debug("%s: wakeup event: MAIN_MCAN0_TX=%d MAIN_MCAN1_TX=%d MCU_MCAN0_TX=%d MCU_MCAN1_TX=%d\n",
	      __func__,
	      readl(CTRL_MMR0_BASE + 0x1C020) & (0b1 << 30) ? 1 : 0,
	      readl(CTRL_MMR0_BASE + 0x1C02C) & (0b1 << 30) ? 1 : 0,
	      readl(WKUP_CTRL_MMR0_BASE + 0x1C0B8) & (0b1 << 30) ? 1 : 0,
	      readl(WKUP_CTRL_MMR0_BASE + 0x1C0D0) & (0b1 << 30) ? 1 : 0);

#if 0
	/* Unlock MMRs */
	Lpm_mmr_unlock(WKUP_CTRL_MMR0_BASE, 6);
	Lpm_mmr_unlock(WKUP_CTRL_MMR0_BASE, 7);
	Lpm_mmr_unlock(CTRL_MMR0_BASE, 7);
#endif

	/* exit MCU canuart io retention with magic word - won't have any affect if already out of io retention */
	writel(0x55555554, WKUP_CTRL_MMR0_BASE + CSL_WKUP_CTRL_MMR_CFG0_MCU_GEN_WAKE_CTRL);
	writel(0x55555555, WKUP_CTRL_MMR0_BASE + CSL_WKUP_CTRL_MMR_CFG0_MCU_GEN_WAKE_CTRL);
	writel(0x55555554, WKUP_CTRL_MMR0_BASE + CSL_WKUP_CTRL_MMR_CFG0_MCU_GEN_WAKE_CTRL);

	/* exit MAIN canuart io retention with magic word - won't have any affect if already out of io retention */
	writel(0x55555554, WKUP_CTRL_MMR0_BASE + CSL_WKUP_CTRL_MMR_CFG0_CANUART_WAKE_CTRL);
	writel(0x55555555, WKUP_CTRL_MMR0_BASE + CSL_WKUP_CTRL_MMR_CFG0_CANUART_WAKE_CTRL);
	writel(0x55555554, WKUP_CTRL_MMR0_BASE + CSL_WKUP_CTRL_MMR_CFG0_CANUART_WAKE_CTRL);

	/* Disable MCU isolation bit if it is active */
	val = readl(CSL_STD_FW_WKUP_DMSC0_PWRCTRL_0_DMSC_PWR_MMR_PWR_START + CSL_PMMCCONTROL_PMCTRL_IO);
	while(val & 0x2000000)
	{
		writel(val & ~(0b1 << 24), CSL_STD_FW_WKUP_DMSC0_PWRCTRL_0_DMSC_PWR_MMR_PWR_START + CSL_PMMCCONTROL_PMCTRL_IO);
		udelay(10);
		val = readl(CSL_STD_FW_WKUP_DMSC0_PWRCTRL_0_DMSC_PWR_MMR_PWR_START + CSL_PMMCCONTROL_PMCTRL_IO);
	}

	/* Disable MAIN isolation bit if it is active */
	val = readl(CSL_STD_FW_WKUP_DMSC0_PWRCTRL_0_DMSC_PWR_MMR_PWR_START + CSL_PMMCCONTROL_PMCTRL_DDR);
	while(val & 0x2000000)
	{
		writel(val & ~(0b1 << 24), CSL_STD_FW_WKUP_DMSC0_PWRCTRL_0_DMSC_PWR_MMR_PWR_START + CSL_PMMCCONTROL_PMCTRL_DDR);
		udelay(10);
		val = readl(CSL_STD_FW_WKUP_DMSC0_PWRCTRL_0_DMSC_PWR_MMR_PWR_START + CSL_PMMCCONTROL_PMCTRL_DDR);
	}
}
#endif

void board_init_f(ulong dummy)
{
#if defined(CONFIG_K3_J721E_DDRSS) || defined(CONFIG_K3_LOAD_SYSFW)
	struct udevice *dev;
	int ret;
#endif
	/*
	 * Cannot delay this further as there is a chance that
	 * K3_BOOT_PARAM_TABLE_INDEX can be over written by SPL MALLOC section.
	 */
	store_boot_info_from_rom();

	/* Make all control module registers accessible */
	ctrl_mmr_unlock();

#ifdef CONFIG_CPU_V7R
	disable_linefill_optimization();
	setup_k3_mpu_regions();
#endif

	/* Init DM early */
	spl_early_init();

#if defined(CONFIG_SPL_BUILD) && defined(CONFIG_TARGET_J7200_R5_EVM)
	spl_exit_ioret();
#endif

#ifdef CONFIG_K3_LOAD_SYSFW
	/*
	 * Process pinctrl for the serial0 a.k.a. MCU_UART0 module and continue
	 * regardless of the result of pinctrl. Do this without probing the
	 * device, but instead by searching the device that would request the
	 * given sequence number if probed. The UART will be used by the system
	 * firmware (SYSFW) image for various purposes and SYSFW depends on us
	 * to initialize its pin settings.
	 */
	ret = uclass_find_device_by_seq(UCLASS_SERIAL, 0, &dev);
	if (!ret)
		pinctrl_select_state(dev, "default");

	/*
	 * Load, start up, and configure system controller firmware. Provide
	 * the U-Boot console init function to the SYSFW post-PM configuration
	 * callback hook, effectively switching on (or over) the console
	 * output.
	 */
	k3_sysfw_loader(is_rom_loaded_sysfw(&bootdata),
			k3_mmc_stop_clock, k3_mmc_restart_clock);

#ifdef CONFIG_SPL_OF_LIST
	do_dt_magic();
#endif

	/*
	 * Force probe of clk_k3 driver here to ensure basic default clock
	 * configuration is always done.
	 */
	if (IS_ENABLED(CONFIG_SPL_CLK_K3)) {
		ret = uclass_get_device_by_driver(UCLASS_CLK,
						  DM_DRIVER_GET(ti_clk),
						  &dev);
		if (ret)
			panic("Failed to initialize clk-k3!\n");
	}

	/* Prepare console output */
	preloader_console_init();

	/* Disable ROM configured firewalls right after loading sysfw */
	remove_fwl_configs(cbass_hc_cfg0_fwls, ARRAY_SIZE(cbass_hc_cfg0_fwls));
	remove_fwl_configs(cbass_hc0_fwls, ARRAY_SIZE(cbass_hc0_fwls));
	remove_fwl_configs(cbass_rc_cfg0_fwls, ARRAY_SIZE(cbass_rc_cfg0_fwls));
	remove_fwl_configs(cbass_rc0_fwls, ARRAY_SIZE(cbass_rc0_fwls));
	remove_fwl_configs(infra_cbass0_fwls, ARRAY_SIZE(infra_cbass0_fwls));
	remove_fwl_configs(mcu_cbass0_fwls, ARRAY_SIZE(mcu_cbass0_fwls));
	remove_fwl_configs(wkup_cbass0_fwls, ARRAY_SIZE(wkup_cbass0_fwls));
#else
	/* Prepare console output */
	preloader_console_init();
#endif

	/* Output System Firmware version info */
	k3_sysfw_print_ver();

	/* Perform board detection */
	do_board_detect();

#if defined(CONFIG_CPU_V7R) && defined(CONFIG_K3_AVS0)
	ret = uclass_get_device_by_driver(UCLASS_MISC, DM_DRIVER_GET(k3_avs),
					  &dev);
	if (ret)
		printf("AVS init failed: %d\n", ret);
#endif

#if defined(CONFIG_K3_J721E_DDRSS)
	ret = uclass_get_device(UCLASS_RAM, 0, &dev);
	if (ret)
		panic("DRAM init failed: %d\n", ret);
#endif
	spl_enable_dcache();
}

u32 spl_mmc_boot_mode(struct mmc *mmc, const u32 boot_device)
{
	switch (boot_device) {
	case BOOT_DEVICE_MMC1:
		return (spl_mmc_emmc_boot_partition(mmc) ? MMCSD_MODE_EMMCBOOT : MMCSD_MODE_FS);
	case BOOT_DEVICE_MMC2:
		return MMCSD_MODE_FS;
	default:
		return MMCSD_MODE_RAW;
	}
}

static u32 __get_backup_bootmedia(u32 main_devstat)
{
	u32 bkup_boot = (main_devstat & MAIN_DEVSTAT_BKUP_BOOTMODE_MASK) >>
			MAIN_DEVSTAT_BKUP_BOOTMODE_SHIFT;

	switch (bkup_boot) {
	case BACKUP_BOOT_DEVICE_USB:
		return BOOT_DEVICE_DFU;
	case BACKUP_BOOT_DEVICE_UART:
		return BOOT_DEVICE_UART;
	case BACKUP_BOOT_DEVICE_ETHERNET:
		return BOOT_DEVICE_ETHERNET;
	case BACKUP_BOOT_DEVICE_MMC2:
	{
		u32 port = (main_devstat & MAIN_DEVSTAT_BKUP_MMC_PORT_MASK) >>
			    MAIN_DEVSTAT_BKUP_MMC_PORT_SHIFT;
		if (port == 0x0)
			return BOOT_DEVICE_MMC1;
		return BOOT_DEVICE_MMC2;
	}
	case BACKUP_BOOT_DEVICE_SPI:
		return BOOT_DEVICE_SPI;
	case BACKUP_BOOT_DEVICE_I2C:
		return BOOT_DEVICE_I2C;
	}

	return BOOT_DEVICE_RAM;
}

static u32 __get_primary_bootmedia(u32 main_devstat, u32 wkup_devstat)
{

	u32 bootmode = (wkup_devstat & WKUP_DEVSTAT_PRIMARY_BOOTMODE_MASK) >>
			WKUP_DEVSTAT_PRIMARY_BOOTMODE_SHIFT;

	bootmode |= (main_devstat & MAIN_DEVSTAT_BOOT_MODE_B_MASK) <<
			BOOT_MODE_B_SHIFT;

	if (bootmode == BOOT_DEVICE_OSPI || bootmode ==	BOOT_DEVICE_QSPI)
		bootmode = BOOT_DEVICE_SPI;

	if (bootmode == BOOT_DEVICE_MMC2) {
		u32 port = (main_devstat &
			    MAIN_DEVSTAT_PRIM_BOOTMODE_MMC_PORT_MASK) >>
			   MAIN_DEVSTAT_PRIM_BOOTMODE_PORT_SHIFT;
		if (port == 0x0)
			bootmode = BOOT_DEVICE_MMC1;
	}

	return bootmode;
}

u32 spl_spi_boot_bus(void)
{
	u32 wkup_devstat = readl(CTRLMMR_WKUP_DEVSTAT);
	u32 main_devstat = readl(CTRLMMR_MAIN_DEVSTAT);
	u32 bootmode = ((wkup_devstat & WKUP_DEVSTAT_PRIMARY_BOOTMODE_MASK) >>
			WKUP_DEVSTAT_PRIMARY_BOOTMODE_SHIFT) |
			((main_devstat & MAIN_DEVSTAT_BOOT_MODE_B_MASK) << BOOT_MODE_B_SHIFT);

	return (bootmode == BOOT_DEVICE_QSPI) ? 1 : 0;
}

u32 spl_boot_device(void)
{
	u32 wkup_devstat = readl(CTRLMMR_WKUP_DEVSTAT);
	u32 main_devstat;

	if (wkup_devstat & WKUP_DEVSTAT_MCU_OMLY_MASK) {
		printf("ERROR: MCU only boot is not yet supported\n");
		return BOOT_DEVICE_RAM;
	}

	/* MAIN CTRL MMR can only be read if MCU ONLY is 0 */
	main_devstat = readl(CTRLMMR_MAIN_DEVSTAT);

	if (bootindex == K3_PRIMARY_BOOTMODE)
		return __get_primary_bootmedia(main_devstat, wkup_devstat);
	else
		return __get_backup_bootmedia(main_devstat);
}
