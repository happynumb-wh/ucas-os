include ../common.mk

ARCH 			?= riscv

MAIN_BUILD		= ./build
MAIN_TARGET		= ./build/libinit.a 


SRC_MAIN 		= $(wildcard ./*.c)
OBJ_MAIN		= $(addprefix build/, \
						$(addsuffix .o, $(basename $(notdir $(SRC_MAIN)))))

DRIVER_INCLUDE  = -I../drivers/screen -I../drivers/sdcard/include

MAIN_FLAGS		= $(CFLAGS)
MAIN_INCLUDE 	= -I../include -I../arch/$(ARCH)/include $(DRIVER_INCLUDE)



all: target


build:
	@mkdir -p $(MAIN_BUILD)


target: $(MAIN_TARGET)


$(MAIN_BUILD)/%.o: %.c | $(MAIN_BUILD)
	@echo + CC $<
	@$(CC) $(MAIN_FLAGS) $(MAIN_INCLUDE) -c $< -o $@


$(MAIN_TARGET): $(OBJ_MAIN)
	@$(AR) rcs $(MAIN_TARGET) $(OBJ_MAIN)
	@echo + AR $@
	@$(RANLIB) $(MAIN_TARGET)
	@echo + RANLIB $@

clean:
	@rm -rf ./build


# Dependencies
DEPS = $(addprefix $(MAIN_BUILD)/, \
						$(addsuffix .d, $(basename $(notdir $(SRC_MAIN)))))
-include $(DEPS)
