
include ../../common.mk

ARCH_KERNEL 	= ./kernel
ARCH_SBI		= ./sbi
ARCH_LIBS		= ./libs
ARCH_BUILD		= ./build

ARCH_TARGET		= ./build/libarch.a 


SRC_ARCH 		= $(wildcard $(ARCH_SBI)/*) $(wildcard $(ARCH_KERNEL)/*) $(wildcard $(ARCH_LIBS)/*)

OBJ_ARCH		= $(addprefix build/, \
						$(addsuffix .o, $(basename $(notdir $(SRC_ARCH)))))

ARCH_FLAGS		= $(CFLAGS)
ARCH_INCLUDE 	= -I./include -I../../include

all: target



build:
	@mkdir -p $(ARCH_BUILD)


target: $(ARCH_TARGET)

$(ARCH_BUILD)/%.o: $(ARCH_KERNEL)/%.* | $(ARCH_BUILD)
	@echo + CC $<
	@$(CC) $(ARCH_FLAGS) $(ARCH_INCLUDE) -c $< -o $@

$(ARCH_BUILD)/%.o: $(ARCH_LIBS)/%.* | $(ARCH_BUILD)
	@echo + CC $<
	@$(CC) $(ARCH_FLAGS) $(ARCH_INCLUDE) -c $< -o $@


$(ARCH_BUILD)/%.o: $(ARCH_SBI)/%.c | $(ARCH_BUILD)
	@echo + CC $<
	@$(CC) $(ARCH_FLAGS) $(ARCH_INCLUDE) -c $< -o $@	


$(ARCH_TARGET): $(OBJ_ARCH)
	@$(AR) rcs $(ARCH_TARGET) $(OBJ_ARCH)
	@echo + AR $@
	@$(RANLIB) $(ARCH_TARGET)
	@echo + RANLIB $@

clean:
	@rm -rf ./build


# Dependencies
DEPS = $(addprefix $(ARCH_BUILD)/, \
						$(addsuffix .d, $(basename $(notdir $(SRC_ARCH)))))
-include $(DEPS)











