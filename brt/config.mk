# Application name.
APPNAME = brt

# Project type parameter: all, sram or flash
PROJECT_TYPE = sram

# List of C source files.
CSRCS = main.c ui.c $(BUILD_DIR)/boot-bin.c flash-startup.c

# List of assembler source files.
ASSRCS = 

# Extra flags to use when compiling.
CFLAGS =

# Extra flags to use when preprocessing.
CPPFLAGS = -DNDEBUG

# Additional items to clean under BUILD_DIR.
CLEAN = boot-bin.c $(APPNAME).tsk $(APPNAME)-flash.*

# Bootloader binary image location.
BOOTLOADER ?= ../boot/$(PLATFORM)/boot.bin

include ../config.mk

# Additional targets for building and signing.
all sign: $(BUILD_DIR)/$(APPNAME).tsk $(BUILD_DIR)/$(APPNAME)-flash.bin

$(BUILD_DIR)/boot-bin.c: $(BOOTLOADER)
	ln -s $< boot.bin
	xxd -i boot.bin | sed '/=/s/^/const /' > $@
	rm boot.bin

# Flash wrapper.
$(BUILD_DIR)/$(APPNAME)-flash.elf: $(vectors) \
		$(BUILD_DIR)/$(APPNAME).a \
		$(linker_script) $(TOP)/$(PROJECT_TYPE).ld \
		Makefile config.mk \
		$(TOP)/config.mk $(TOP)/platforms/$(PLATFORM)/config.mk
	@echo $(MSG_LINKING)
	$(Q)$(CC) $(l_flags) -Wl,-T,$(TOP)/$(PROJECT_TYPE).ld \
		$(vectors) $(addprefix $(BUILD_DIR)/,$(ALWAYS_LINK)) \
		-Wl,--entry=flash_startup \
		-Wl,--start-group $(filter %.a,$^) -Wl,--end-group \
		$(libflags-gnu-y) -o $@

%.tsk: %.elf
	@echo $(MSG_BINARY_IMAGE)
	$(Q)$(OBJCOPY) -O binary $< $@
