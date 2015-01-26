# Application name.
APPNAME = factory

# List of C source files.
CSRCS = main.c ui.c xflash.c hwtest/fs-1.c flashcalw.c \
	brt/$(PLATFORM)/boot-bin.c $(BUILD_DIR)/ifp-bin.c

# List of objects that should be linked explicitly, rather than taken from
# a library.  This is useful to override library implementation (e.g. from ASF)
# with a project-specific one without affecting other projects.
ALWAYS_LINK = flashcalw.o

# List of assembler source files.
ASSRCS = 

# Extra flags to use when compiling.
CFLAGS =

# Extra flags to use when preprocessing.
CPPFLAGS = -DNDEBUG -DSYNC_DIAGNOSTICS=1

# Additional items to clean under BUILD_DIR
CLEAN = ifp-bin.c

include ../config.mk

$(BUILD_DIR)/flashcalw.o: $(BUILD_DIR)/sam/drivers/flashcalw/flashcalw.o rensec.awk
	@echo OBJCOPY $@
	$(Q)$(OBJCOPY) `$(OBJDUMP) -h $< | awk -f rensec.awk` $< $@

$(BUILD_DIR)/ifp-bin.c: ifp/$(PLATFORM)/ifp.bin
	(cd $(dir $<); xxd -i $(notdir $<) | sed '/=/s/^/const /') > $@

clean: clean-ifp

clean-ifp:
	$(MAKE) -C ifp clean

ifneq ($(MAKECMDGOALS),clean)
-include recurse
endif

recurse:
	$(MAKE) -C ifp

.PHONY: ifp clean-ifp recurse
