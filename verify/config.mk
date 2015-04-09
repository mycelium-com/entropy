# The author has waived all copyright and related or neighbouring rights
# to this file and placed it in public domain.

# Application name.
APPNAME = verify

# Project type parameter: all, sram or flash
PROJECT_TYPE = sram

# List of C source files.
CSRCS = main.c ui.c xflash.c fw-access.c disk.c

# List of assembler source files.
ASSRCS = 

# Extra flags to use when compiling.
CFLAGS =

# Extra flags to use when preprocessing.
CPPFLAGS = -DNDEBUG

# Additional items to clean under BUILD_DIR.
CLEAN = $(APPNAME).tsk ../disk.img ../disk.c

include ../config.mk

# Additional targets for building and signing.
all sign: $(BUILD_DIR)/$(APPNAME).tsk

%.tsk: %.elf
	@echo $(MSG_BINARY_IMAGE)
	$(Q)$(OBJCOPY) -O binary $< $@

disk.c: disk.mk disk.conf
	$(MAKE) -f $< $@
