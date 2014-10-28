# Application name.
APPNAME = factory

# List of C source files.
CSRCS = main.c ui.c xflash.c hwtest/fs-1.c brt/$(PLATFORM)/boot-bin.c flashcalw.c

# List of objects that should be linked explicitly, rather than taken from
# a library.  This is useful to override library implementation (e.g. from ASF)
# with a project-specific one without affecting other projects.
ALWAYS_LINK = flashcalw.o

# List of assembler source files.
ASSRCS = 

# Extra flags to use when compiling.
CFLAGS =

# Extra flags to use when preprocessing.
CPPFLAGS = -DNDEBUG

include ../config.mk

$(BUILD_DIR)/flashcalw.o: $(BUILD_DIR)/sam/drivers/flashcalw/flashcalw.o rensec.awk
	@echo OBJCOPY $@
	$(Q)$(OBJCOPY) `$(OBJDUMP) -h $< | awk -f rensec.awk` $< $@
