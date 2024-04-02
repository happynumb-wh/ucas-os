# -----------------------------------------------------------------------
# Project Information
# -----------------------------------------------------------------------

PROJECT_IDX	= 

# -----------------------------------------------------------------------
# Host Linux Variables
# -----------------------------------------------------------------------

SHELL       = /bin/sh
TTYUSB1     = /dev/ttyUSB1
DIR_OSLAB   = $(HOME)/OSLab/OSLab-RISC-V
DIR_QEMU    = /home/wanghan/Workspace/QEMU-upgrade/upgrade-qemu/build/
DIR_UBOOT   = $(DIR_OSLAB)/u-boot

# -----------------------------------------------------------------------
# Build and Debug Tools
# -----------------------------------------------------------------------

HOST_CC         = gcc
CROSS_PREFIX    = riscv64-unknown-linux-gnu-
CC              = $(CROSS_PREFIX)gcc
AR              = $(CROSS_PREFIX)ar
OBJDUMP         = $(CROSS_PREFIX)objdump
GDB             = $(CROSS_PREFIX)gdb
QEMU            = $(DIR_QEMU)qemu-system-riscv64
UBOOT           = $(DIR_UBOOT)/u-boot
MINICOM         = minicom

# -----------------------------------------------------------------------
# Build/Debug Flags and Variables
# -----------------------------------------------------------------------

CFLAGS          = -O2 -fno-builtin -nostdlib -nostdinc -Wall -mcmodel=medany -ggdb3 -w


KERNEL_INCLUDE  = -I$(DIR_ARCH)/include -Iinclude -Idrivers/screen -Idrivers/sdcard/include 
KERNEL_CFLAGS   = $(CFLAGS) $(KERNEL_INCLUDE) -Wl,--defsym=TEXT_START=$(KERNEL_ENTRYPOINT) -T riscv.lds -DDASICS_DEBUG
# -DQEMU

USER_INCLUDE    = -I$(DIR_TINYLIBC)/include
USER_CFLAGS     = $(CFLAGS) $(USER_INCLUDE)
USER_LDFLAGS    = -L$(DIR_BUILD) -ltinyc


QEMU_OPTS       =  -M virt -m 256M \
				   -nographic -kernel ./target/bbl \
				   -drive file=./img/sd.img,if=none,format=raw,id=x0 \
				   -device virtio-blk-device,drive=x0 \
				   -bios none

QEMU_DEBUG_OPT  = -s -S
# QEMU_SMP_OPT	= -smp 2

# -----------------------------------------------------------------------
# UCAS-OS Entrypoints and Variables
# -----------------------------------------------------------------------

DIR_ARCH        = ./arch/riscv
DIR_BUILD       = ./build
DIR_DRIVERS     = ./drivers
DIR_INIT        = ./init
DIR_KERNEL      = ./kernel
DIR_LIBS        = ./libs
DIR_TINYLIBC    = ./tiny_libc
DIR_TEST        = ./test

KERNEL_ENTRYPOINT       = 0xffffffc080200000
USER_ENTRYPOINT         = 0x10000

# -----------------------------------------------------------------------
# UCAS-OS Kernel Source Files
# -----------------------------------------------------------------------

SRC_BOOT    = $(wildcard $(DIR_ARCH)/boot/*.S)
SRC_ARCH    = $(wildcard $(DIR_ARCH)/kernel/*.S)
SRC_BIOS    = $(wildcard $(DIR_ARCH)/sbi/*.c)
SRC_DRIVER  = $(wildcard $(DIR_DRIVERS)/*/*.c)
SRC_INIT    = $(wildcard $(DIR_INIT)/*.c)
SRC_KERNEL  = $(wildcard $(DIR_KERNEL)/*/*.c) $(wildcard $(DIR_KERNEL)/*/*.S) $(DIR_KERNEL)/user_programs.c
SRC_LIBS    = $(wildcard $(DIR_LIBS)/*.c)
SRC_START   = $(wildcard $(DIR_ARCH)/kernel/*.c)

SRC_MAIN    = $(SRC_ARCH) $(SRC_INIT) $(SRC_BIOS) $(SRC_DRIVER) $(SRC_KERNEL) $(SRC_LIBS) $(SRC_START)

ELF_MAIN    = $(DIR_BUILD)/main
ELF_IMAGE   = $(DIR_BUILD)/image


# -----------------------------------------------------------------------
# UCAS-OS User Source Files
# -----------------------------------------------------------------------

SRC_CRT0    = $(wildcard $(DIR_ARCH)/crt0/*.S)
OBJ_CRT0    = $(DIR_BUILD)/$(notdir $(SRC_CRT0:.S=.o))

SRC_LIBC    = $(wildcard ./tiny_libc/*.c)
OBJ_LIBC    = $(patsubst %.c, %.o, $(foreach file, $(SRC_LIBC), $(DIR_BUILD)/$(notdir $(file))))
LIB_TINYC   = $(DIR_BUILD)/libtinyc.a

SRC_SHELL	= $(DIR_TEST)/shell.c
SRC_USER    = $(wildcard $(DIR_TEST)/*.c)
ELF_USER    = $(patsubst %.c, %, $(foreach file, $(SRC_USER), $(DIR_BUILD)/$(notdir $(file)))) \
  $(RISCV_ROOTFS_HOME)/rootfsimg/build/dasics-test-free \
  $(RISCV_ROOTFS_HOME)/rootfsimg/build/dasics-test-jump \
  $(RISCV_ROOTFS_HOME)/rootfsimg/build/dasics-test-rwx \
  $(RISCV_ROOTFS_HOME)/rootfsimg/build/dasics-test-syscall \
  $(RISCV_ROOTFS_HOME)/rootfsimg/build/dasics-test-ofb \
  $(RISCV_ROOTFS_HOME)/rootfsimg/build/attack-case


USER_PROG   = $(DIR_KERNEL)/user_programs.c include/user_programs.h

# -----------------------------------------------------------------------
# Host Linux Tools Source Files
# -----------------------------------------------------------------------

SRC_CREATEIMAGE = ./tools/createimage.c
ELF_CREATEIMAGE = $(DIR_BUILD)/$(notdir $(SRC_CREATEIMAGE:.c=))

SRC_ELF2CHAR = ./tools/elf2char.c
ELF_ELF2CHAR = $(DIR_BUILD)/$(notdir $(SRC_ELF2CHAR:.c=))

SRC_GENERATEMAPPING = ./tools/generateMapping.c
ELF_GENERATEMAPPING = $(DIR_BUILD)/$(notdir $(SRC_GENERATEMAPPING:.c=))

# -----------------------------------------------------------------------
# Top-level Rules
# -----------------------------------------------------------------------

all: dirs elf image asm # floppy

dirs:
	@mkdir -p $(DIR_BUILD)

clean:
	rm -rf $(DIR_BUILD)
	rm kernel/user_programs.c 
	rm include/user_programs.h

asm:  $(ELF_MAIN) $(ELF_USER) #$(ELF_BOOT)
	@for elffile in $^; do $(OBJDUMP) -d $$elffile > $(notdir $$elffile).txt; done
	

gdb:
	$(GDB) $(ELF_MAIN) -ex "target remote:1234" -x .gdbinit

run:
	$(QEMU) $(QEMU_OPTS)

run-smp:
	$(QEMU) $(QEMU_OPTS) $(QEMU_SMP_OPT)

debug:
	$(QEMU) $(QEMU_OPTS) $(QEMU_DEBUG_OPT)

debug-smp:
	$(QEMU) $(QEMU_OPTS) $(QEMU_SMP_OPT) $(QEMU_DEBUG_OPT)


minicom:
	sudo $(MINICOM) -D $(TTYUSB1)

.PHONY: all dirs clean asm gdb run debug minicom $(USER_PROG)

# -----------------------------------------------------------------------
# UCAS-OS Rules
# -----------------------------------------------------------------------

$(USER_PROG): $(ELF_USER) ${ELF_ELF2CHAR} ${ELF_GENERATEMAPPING}
	if [ -e kernel/user/user_programs.c ]; then rm kernel/user/user_programs.c include/user_programs.h;fi
	echo "" > user_programs.c
	echo "" > user_programs.h
	@for prog in $(ELF_USER); \
	do \
		./${ELF_ELF2CHAR} --header-only $$prog >> user_programs.h; \
	done
	@for prog in $(ELF_USER); \
	do \
		./${ELF_ELF2CHAR} $$prog >> user_programs.c; \
	done
	./${ELF_GENERATEMAPPING} user_programs
	mv user_programs.h include/
	mv user_programs.c kernel/


$(ELF_MAIN): $(SRC_MAIN) riscv.lds 
	$(CC) $(KERNEL_CFLAGS) -o $@ $(SRC_MAIN) -e _boot

$(OBJ_CRT0): $(SRC_CRT0)
	@$(CC) $(USER_CFLAGS) -I$(DIR_ARCH)/include -c $< -o $@

$(LIB_TINYC): $(OBJ_LIBC)
	$(AR) rcs $@ $^

$(DIR_BUILD)/%.o: $(DIR_TINYLIBC)/%.c
	@$(CC) $(USER_CFLAGS) -c $< -o $@


$(DIR_BUILD)/%: $(DIR_TEST)/%.c $(OBJ_CRT0) $(LIB_TINYC) riscv.lds
	$(CC) $(USER_CFLAGS) -o $@ $(OBJ_CRT0) $< $(USER_LDFLAGS) -Wl,--defsym=TEXT_START=$(USER_ENTRYPOINT) -T riscv.lds

elf: $(ELF_MAIN) $(LIB_TINYC) $(ELF_USER) #$(ELF_BOOT)

.PHONY: elf

# -----------------------------------------------------------------------
# Host Linux Rules
# -----------------------------------------------------------------------

# $(ELF_CREATEIMAGE): $(SRC_CREATEIMAGE)
# 	$(HOST_CC) $(SRC_CREATEIMAGE) -o $@ -ggdb -Wall

$(ELF_ELF2CHAR): $(SRC_ELF2CHAR)
	$(HOST_CC) $(SRC_ELF2CHAR) -o $@ -ggdb -Wall

$(ELF_GENERATEMAPPING): $(SRC_GENERATEMAPPING)
	$(HOST_CC) $(SRC_GENERATEMAPPING) -o $@ -ggdb -Wall

# image: $(ELF_CREATEIMAGE) $(ELF_BOOT) $(ELF_MAIN) $(ELF_USER)
# 	cd $(DIR_BUILD) && ./$(<F) --extended $(filter-out $(<F), $(^F)) 
# && dd if=/dev/zero of=image oflag=append conv=notrunc bs=512MB count=2

.PHONY: image
