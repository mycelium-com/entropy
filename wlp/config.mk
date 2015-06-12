# Application name.
APPNAME = wlp

# Project type parameter: all, sram or flash
PROJECT_TYPE = sram

# List of C source files.
CSRCS = main.c ui.c xflash.c $(BUILD_DIR)/en-us-b39.c

# List of assembler source files.
ASSRCS =

# Extra flags to use when compiling.
CFLAGS =

# Extra flags to use when preprocessing.
CPPFLAGS = -DNDEBUG

# Additional items to clean under BUILD_DIR.
CLEAN = en-US-b39.c $(APPNAME).tsk

include ../config.mk

# Additional targets for building and signing.
all sign: $(BUILD_DIR)/$(APPNAME).tsk

# BIP-39 word list.
$(BUILD_DIR)/en-us-b39.c: ../factory/tools/en-US.b39
	ln -s $< .
	xxd -i $(notdir $<) | sed '/=/s/^/const /' > $@
	rm $(notdir $<)

%.tsk: %.elf
	@echo $(MSG_BINARY_IMAGE)
	$(Q)$(OBJCOPY) -O binary $< $@
