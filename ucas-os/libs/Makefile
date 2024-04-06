include ../common.mk

ARCH 			?= riscv

LIBS_BUILD		= ./build
LIBS_TARGET		= ./build/liblibs.a 


SRC_LIBS 		= $(wildcard ./*.c)
OBJ_LIBS		= $(addprefix build/, \
						$(addsuffix .o, $(basename $(notdir $(SRC_LIBS)))))

DRIVER_INCLUDE  = -I../drivers/screen -I../drivers/sdcard/include

LIBS_FLAGS		= $(CFLAGS)
LIBS_INCLUDE 	= -I../include -I../arch/$(ARCH)/include $(DRIVER_INCLUDE)



all: target


build:
	@mkdir -p $(LIBS_BUILD)


target: $(LIBS_TARGET)


$(LIBS_BUILD)/%.o: %.c | $(LIBS_BUILD)
	@echo + CC $<
	@$(CC) $(LIBS_FLAGS) $(LIBS_INCLUDE) -c $< -o $@


$(LIBS_TARGET): $(OBJ_LIBS)
	@$(AR) rcs $(LIBS_TARGET) $(OBJ_LIBS)
	@echo + AR $@
	@$(RANLIB) $(LIBS_TARGET)
	@echo + RANLIB $@

clean:
	@rm -rf ./build


# Dependencies
DEPS = $(addprefix $(LIBS_BUILD)/, \
						$(addsuffix .d, $(basename $(notdir $(SRC_LIBS)))))
-include $(DEPS)
