SHELL = /bin/sh

# PATH 
ifeq ($(DIR_QEMU),)
DIR_QEMU       ?= /home/wanghan/Workspace/QEMU-upgrade/upgrade-qemu/build/
else 
endif

DIR_OPENSBI		= /home/wanghan/riscv/opensbi


# Compiler
HOST_CC         = gcc
CROSS_PREFIX    = riscv64-unknown-linux-gnu-


CC              = $(CROSS_PREFIX)gcc
CLANG           = $(CROSS_PREFIX)clang
AR              = $(CROSS_PREFIX)ar
LD 				= $(CROSS_PREFIX)ld
OBJDUMP         = $(CROSS_PREFIX)objdump
OBJCOPY 		= $(CROSS_PREFIX)objcopy
GDB             = $(CROSS_PREFIX)gdb
RANLIB			= $(CROSS_PREFIX)ranlib
QEMU            = $(DIR_QEMU)qemu-system-riscv64



# FLAGS
CONFIG_DEF      = -DDASICS_DEBUG \
					-DNANHU_V3 \
					-DDASICS \
					-DRAMFS
CFLAGS          = -O0 -MMD -fno-builtin -nostdlib -nostdinc -Wall -mcmodel=medany -ggdb3 $(CONFIG_DEF) -Wno-main
KERNEL_CFLAGS   = CFLAGS
USER_CFLGAS     = CFLAGS


OBJCOPY_FLAGS 	= --set-section-flags .bss=alloc,contents --set-section-flags .sbss=alloc,contents -O binary

QEMU_OPTS       = -M virt -m 256M \
				  -nographic -kernel $(DIR_BUILD)/kernel.bin \
				  -drive file=$(PWD)/img/sd.img,if=none,format=raw,id=x0 \
				  -device virtio-blk-device,drive=x0 \
				  -bios $(DIR_OPENSBI)/build/platform/generic/firmware/fw_jump.bin
QEMU_DEBUG_OPT  = -s -S



# Entry point
KERNEL_ENTRYPOINT       = 0xffffffc080200000
USER_ENTRYPOINT         = 0x10000


# Dependencies
DEPS = $(addprefix $(DIR_BUILD)/, \
						$(addsuffix .d, $(basename $(BASENAME))))
-include $(DEPS)