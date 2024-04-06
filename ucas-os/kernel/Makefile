include ../common.mk

ARCH 			?= riscv

KERNEL_BUILD	= ./build
KERNEL_TARGET	= ./build/libkernel.a 


SRC_KERNEL 		= $(wildcard ./*/*.c) ./user_programs.c
OBJ_KERNEL		= $(addprefix build/, \
						$(addsuffix .o, $(basename $(notdir $(SRC_KERNEL)))))

DRIVER_INCLUDE  = -I../drivers/screen -I../drivers/sdcard/include

KERNEL_FLAGS	= $(CFLAGS)
KERNEL_INCLUDE 	= -I../include -I../arch/$(ARCH)/include $(DRIVER_INCLUDE)



all: target


build:
	@mkdir -p $(KERNEL_BUILD)


target: $(KERNEL_TARGET)


$(KERNEL_BUILD)/%.o: ./*/%.c | $(KERNEL_BUILD)
	@echo + CC $<
	@$(CC) $(KERNEL_FLAGS) $(KERNEL_INCLUDE) -c $< -o $@


$(KERNEL_BUILD)/%.o: %.c | $(KERNEL_BUILD)
	@echo + CC $<
	@$(CC) $(KERNEL_FLAGS) $(KERNEL_INCLUDE) -c $< -o $@


$(KERNEL_TARGET): $(OBJ_KERNEL)
	@$(AR) rcs $(KERNEL_TARGET) $(OBJ_KERNEL)
	@echo + AR $@
	@$(RANLIB) $(KERNEL_TARGET)
	@echo + RANLIB $@

clean:
	@rm -rf ./build


# Dependencies
DEPS = $(addprefix $(KERNEL_BUILD)/, \
						$(addsuffix .d, $(basename $(notdir $(SRC_KERNEL)))))
-include $(DEPS)
