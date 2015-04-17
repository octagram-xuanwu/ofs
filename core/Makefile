##
# @brief Makefile for the ofs (octagram filesystem, outlandish filesystem, or
#        odd filesystem) kernel module
# @author Octagram Sun <octagram@qq.com>
# @version 0.1.0
# @date 2015
# @copyright Octagram Sun <octagram@qq.com>
#
# @node
# This file is a part of ofs, as available from
# * https://gitcafe.com/octagram/ofs
# * https://github.com/octagram-xuanwu/ofs
# This file is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License (GPL) as published by the Free
# Software Foundation, in version 2. The ofs is distributed in the hope
# that it will be useful, but <b>WITHOUT ANY WARRANTY</b> of any kind.
#

# env
KDIR ?= /lib/modules/`uname -r`/build
KRN_VER ?= `uname -r`
CROSS_COMPILE ?=
SHELL := bash
CC := $(CROSS_COMPILE)gcc
STRIP := $(CROSS_COMPILE)strip

-include $(M)/ofs_config.mk

# kernel info
KRN_VER_NUM := $(shell sed -n '/LINUX_VERSION_CODE/p' \
                 ${KDIR}/include/generated/uapi/linux/version.h \
                 | gawk '{print $$3}')
KRN_VER_NUM_H := $(shell TMP=`echo ${KRN_VER_NUM}`; \
                   TMP=$$[TMP>>16]; echo $$TMP)
KRN_VER_NUM_M := $(shell TMP=`echo ${KRN_VER_NUM}`; \
                   TMP=$$[(TMP&0xFFFF) >> 8]; echo $$TMP)
KRN_VER_NUM_L := $(shell TMP=`echo ${KRN_VER_NUM}`; \
                   TMP=$$[TMP&0xFF]; echo $$TMP)
KRN_BITS := $(shell sed -n '/CONFIG_64BIT=y/p' ${KDIR}/.config)
KRN_BITS := $(shell TMP=`echo ${KRN_BITS}`; [ x$$TMP = x"" ] && \
              TMP="32-bits" || TMP="64-bits"; echo $$TMP)

# module info
CONFIG_OFS := m
MODULE_NAME := ofs
obj-$(CONFIG_OFS) := $(MODULE_NAME).o
$(MODULE_NAME)-objs := module.o fs.o magic.o normal.o dir.o regfile.o \
		       symlink.o singularity.o ksym.o
ifeq ($(CONFIG_OFS_SYSFS), y)
$(MODULE_NAME)-objs += omsys.o
endif
ifeq ($(CONFIG_OFS_DEBUGFS), y)
$(MODULE_NAME)-objs += debugfs.o
endif

all: krninfo ko

cfg: ofs_config.h
	sed -n "/^#define.*[10]/p" ofs_config.h | \
	gawk \
	'{ \
		if ($$3=="1") \
			print $$2" := y"; \
		else \
			print "# "$$2" is not set"; \
	}' > ofs_config.mk

krninfo:
	@printf '\e[38;5;051m\e[48;5;004m[kernel info] %s\e[0m\n'
	@printf '\e[38;5;051m\e[48;5;004mkernel bits: %s\e[0m\n' ${KRN_BITS}
	@printf '\e[38;5;051m\e[48;5;004mkernel version: %d.%d.%d\e[0m\n' \
		${KRN_VER_NUM_H} ${KRN_VER_NUM_M} ${KRN_VER_NUM_L}

ko: *.c *.h cfg
	$(MAKE) -C $(KDIR) M=$(shell pwd) modules
	cp Module.symvers $(MODULE_NAME).symvers

strip: ko
	$(STRIP) $(MODULE_NAME).ko --strip-unneeded

doc: ofs.doxygen
	doxygen $<

gtags:
	gtags -v

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c \
		.tmp_versions *.markers *.symvers *.order
	rm -rf doc *.mk

clean-gtags:
	rm -f GPATH GRTAGS GTAGS

.PHONY: all ko cfg strip doc gtags krninfo clean clean-gtags
