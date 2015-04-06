##
# @brief Makefile for the ofs (octagram filesystem, outlandish filesystem, or
#        odd filesystem) testing kernel module
# @author Octagram Sun <octagram@qq.com>
# @version 0.1.0
# @date 2015
# @copyright Octagram Sun <octagram@qq.com>
#
# @note
# This file is part of ofs, as available from
# * https://gitcafe.com/octagram/ofs
# * https://github.com/octagram-xuanwu/ofs
# This file is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License (GPL) as published by the Free
# Software Foundation, in version 2. The ofs is distributed in the hope
# that it will be useful, but <b>WITHOUT ANY WARRANTY</b> of any kind.
#

KDIR ?= /lib/modules/`uname -r`/build
KRN_VER ?= `uname -r`
CROSS_COMPILE ?=

########################## +++ config +++ ############################
CONFIG_OFS_TEST := m
CONFIG_OFS_TEST_DEBUG := y
########################## --- config --- ############################
EXTRA_CFLAGS := -DCONFIG_OFS_TEST=1

ifeq ($(CONFIG_OFS_TEST_DEBUG), y)
EXTRA_CFLAGS += -DCONFIG_OFS_TEST_DEBUG=1
endif

MODULE_NAME := ofstest
PWD := $(shell pwd)
CC := $(CROSS_COMPILE)gcc
STRIP := $(CROSS_COMPILE)strip

KBUILD_EXTRA_SYMBOLS := ${OFS_CORE_DIR}/ofs.symvers
EXTRA_CFLAGS += -I${OFS_CORE_DIR}

obj-$(CONFIG_OFS_TEST) := $(MODULE_NAME).o
$(MODULE_NAME)-objs := test.o

all: modules

modules: *.c ${OFS_CORE_DIR}/ofs.h ${OFS_CORE_DIR}/ofs.symvers
	$(MAKE) -C $(KDIR) M=$(PWD) modules

strip: modules
	$(STRIP) $(MODULE_NAME).ko --strip-unneeded

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions *.markers *.symvers *.order

.PHONY: modules clean strip
