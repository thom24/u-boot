# SPDX-License-Identifier: GPL-2.0

spinand-objs := core.o esmt.o gigadevice.o macronix.o micron.o paragon.o
spinand-objs += toshiba.o winbond.o
obj-$(CONFIG_$(SPL_TPL_)NAND_SPI_SUPPORT) += spinand.o
obj-$(CONFIG_MTD_SPI_NAND) += spinand.o
