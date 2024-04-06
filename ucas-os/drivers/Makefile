include ../common.mk

ARCH 			?= riscv

DRIVER_SCREEN 	= ./screen
DRIVER_SDCARD	= ./sdcard
DRIVER_BUILD	= ./build

DRIVER_TARGET	= ./build/libdrivers.a 


SRC_DRIVER 		= $(wildcard $(DRIVER_SCREEN)/*.c) $(wildcard $(DRIVER_SDCARD)/*.c)

OBJ_DRIVER		= $(addprefix build/, \
						$(addsuffix .o, $(basename $(notdir $(SRC_DRIVER)))))

DRIVER_FLAGS	= $(CFLAGS)
DRIVER_INCLUDE 	= -I$(DRIVER_SCREEN) -I$(DRIVER_SDCARD)/include -I../include -I../arch/$(ARCH)/include

DRIVER_SDCARD_INCLUDE = -I$(DRIVER_SDCARD)/include -I../include
DRIVER_SCREEN_INCLUDE = -I$(DRIVER_SCREEN) -I../include -I../arch/$(ARCH)/include


all: target


build:
	@mkdir -p $(DRIVER_BUILD)


target: $(DRIVER_TARGET)


$(DRIVER_BUILD)/%.o: $(DRIVER_SCREEN)/%.c | $(DRIVER_BUILD)
	@echo + CC $<
	@$(CC) $(DRIVER_FLAGS) $(DRIVER_INCLUDE) -c $< -o $@


$(DRIVER_BUILD)/%.o: $(DRIVER_SDCARD)/%.c | $(DRIVER_BUILD)
	@echo + CC $<
	@$(CC) $(DRIVER_FLAGS) $(DRIVER_INCLUDE) -c $< -o $@


$(DRIVER_TARGET): $(OBJ_DRIVER)
	@$(AR) rcs $(DRIVER_TARGET) $(OBJ_DRIVER)
	@echo + AR $@
	@$(RANLIB) $(DRIVER_TARGET)
	@echo + RANLIB $@

clean:
	@rm -rf ./build


# Dependencies
DEPS = $(addprefix $(DRIVER_BUILD)/, \
						$(addsuffix .d, $(basename $(notdir $(SRC_DRIVER)))))
-include $(DEPS)
