include common.mk
# DIR
ARCH 		   ?= riscv

DIR_ARCH        = ./arch/$(ARCH)
DIR_BUILD       = ./build
DIR_DRIVERS     = ./drivers
DIR_INIT        = ./init
DIR_KERNEL      = ./kernel
DIR_LIBS        = ./libs
DIR_TINYLIBC    = ./tiny_libc
DIR_TEST        = ./test
DIR_TOOLS		= ./tools
DIR_SCRIPT		= ./script

# -----------------------------------------------------------------------
# Build/Debug Flags and Variables
# -----------------------------------------------------------------------

KERNEL_INCLUDE  = -I$(DIR_ARCH)/include -Iinclude -Idrivers/screen -Idrivers/sdcard/include 
KERNEL_CFLAGS   = $(CFLAGS) $(KERNEL_INCLUDE) -Wl,--defsym=TEXT_START=$(KERNEL_ENTRYPOINT) -T $(DIR_SCRIPT)/kernel.lds

USER_INCLUDE    = -I$(DIR_TINYLIBC)/include
USER_CFLAGS     = $(CFLAGS) $(USER_INCLUDE)
USER_LDFLAGS    = -L$(DIR_BUILD) -ltinyc


# -----------------------------------------------------------------------
# UCAS-OS Kernel target
# -----------------------------------------------------------------------
MODULE_ARCH		= $(DIR_ARCH)/build/*.o
MODULE_DRIVERS	= $(DIR_DRIVERS)/build/*.o
MODULE_INIT		= $(DIR_INIT)/build/*.o
MODULE_KERNEL	= $(DIR_KERNEL)/build/*.o
MODULE_LIBS		= $(DIR_LIBS)/build/*.o

MODULES 		= $(DIR_ARCH)/build/*.o \
					$(DIR_DRIVERS)/build/*.o \
					$(DIR_INIT)/build/*.o \
					$(DIR_KERNEL)/build/*.o \
					$(DIR_LIBS)/build/*.o
# Final target
ELF_KERNEL  = $(DIR_BUILD)/kernel
ELF_IMAGE   = $(DIR_BUILD)/image
KERNEL_BIN	= $(DIR_BUILD)/kernel.bin


# -----------------------------------------------------------------------
# UCAS-OS User Source Files
# -----------------------------------------------------------------------
SRC_CRT0    = $(wildcard $(DIR_TINYLIBC)/crt0/*.S)
OBJ_CRT0    = $(DIR_BUILD)/$(notdir $(SRC_CRT0:.S=.o))

SRC_LIBC    = $(wildcard ./tiny_libc/*.c)
OBJ_LIBC    = $(patsubst %.c, %.o, $(foreach file, $(SRC_LIBC), $(DIR_BUILD)/$(notdir $(file))))
LIB_TINYC   = $(DIR_BUILD)/libtinyc.a

SRC_SHELL	= $(DIR_TEST)/shell.c
SRC_USER    = $(wildcard $(DIR_TEST)/*.c)
ELF_USER    = $(patsubst %.c, %, $(foreach file, $(SRC_USER), $(DIR_BUILD)/$(notdir $(file))))

USER_PROG   = $(DIR_KERNEL)/user_programs.c

# -----------------------------------------------------------------------
# Host Linux Tools Source Files
# -----------------------------------------------------------------------
SRC_CREATEIMAGE = $(DIR_TOOLS)/createimage.c
ELF_CREATEIMAGE = $(DIR_BUILD)/$(notdir $(SRC_CREATEIMAGE:.c=))

SRC_ELF2CHAR = $(DIR_TOOLS)/elf2char.c
ELF_ELF2CHAR = $(DIR_BUILD)/$(notdir $(SRC_ELF2CHAR:.c=))

SRC_GENERATEMAPPING = $(DIR_TOOLS)/generateMapping.c
ELF_GENERATEMAPPING = $(DIR_BUILD)/$(notdir $(SRC_GENERATEMAPPING:.c=))

# -----------------------------------------------------------------------
# Top-level Rules
# -----------------------------------------------------------------------

all: dirs tools elf kbin asm

dirs:
	@mkdir -p $(DIR_BUILD)

clean:
	rm -rf $(DIR_BUILD)
	$(MAKE) -C $(DIR_ARCH) clean
	$(MAKE) -C $(DIR_DRIVERS) clean
	$(MAKE) -C $(DIR_INIT) clean
	$(MAKE) -C $(DIR_KERNEL) clean
	$(MAKE) -C $(DIR_LIBS) clean
	rm kernel/user_programs.c
	rm include/user_programs.h

asm:  $(ELF_KERNEL) $(ELF_USER) #$(ELF_BOOT)
	@for elffile in $^; do $(OBJDUMP) -d $$elffile > $(notdir $$elffile).txt; done
	
	
gdb:
	$(GDB) $(ELF_KERNEL) -ex "target remote:1234" -x .gdbinit

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

.PHONY: dirs clean asm gdb run debug minicom


# -----------------------------------------------------------------------
# UCAS-OS Rules
# -----------------------------------------------------------------------
$(USER_PROG): $(ELF_USER) $(ELF_ELF2CHAR) $(ELF_GENERATEMAPPING)
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

$(MODULE_ARCH):
	$(MAKE) -C $(DIR_ARCH)

$(MODULE_DRIVERS):
	$(MAKE) -C $(DIR_DRIVERS)

$(MODULE_INIT):
	$(MAKE) -C $(DIR_INIT)

$(MODULE_KERNEL): $(USER_PROG)
	$(MAKE) -C $(DIR_KERNEL)

$(MODULE_LIBS):
	$(MAKE) -C $(DIR_LIBS)

$(ELF_KERNEL): $(MODULES)
	@echo LD + $@
	@$(CC) $(KERNEL_CFLAGS) -o $@ $(MODULES) 

$(OBJ_CRT0): $(SRC_CRT0)
	@echo CC + $<
	@$(CC) $(USER_CFLAGS) -I$(DIR_ARCH)/include -c $< -o $@

$(LIB_TINYC): $(OBJ_LIBC)
	@echo AR + $@
	@$(AR) rcs $@ $^

$(DIR_BUILD)/%.o: $(DIR_TINYLIBC)/%.c
	@echo CC +  $<
	@$(CC) $(USER_CFLAGS) -c $< -o $@


$(DIR_BUILD)/%: $(DIR_TEST)/%.c $(OBJ_CRT0) $(LIB_TINYC)
	@echo CC + $<
	@$(CC) $(USER_CFLAGS) -o $@ $(OBJ_CRT0) $< $(USER_LDFLAGS) -Wl,--defsym=TEXT_START=$(USER_ENTRYPOINT) -T $(DIR_SCRIPT)/user.lds

elf: $(ELF_KERNEL) $(LIB_TINYC) $(ELF_USER)

$(KERNEL_BIN): $(ELF_KERNEL)
	@echo OBJCPY + $<
	@$(OBJCOPY) $(OBJCOPY_FLAGS) $< $@ 

# image: $(ELF_IMAGE)
kbin: $(KERNEL_BIN)


$(ELF_ELF2CHAR): $(SRC_ELF2CHAR)
	@echo HOST_CC + $<
	@$(HOST_CC) $(SRC_ELF2CHAR) -o $@ -ggdb -Wall

$(ELF_GENERATEMAPPING): $(SRC_GENERATEMAPPING)
	@echo HOST_CC + $<
	@$(HOST_CC) $(SRC_GENERATEMAPPING) -o $@ -ggdb -Wall