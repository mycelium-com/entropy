# The author has waived all copyright and related or neighbouring rights
# to this file and placed it in public domain.

# Application name.
APPNAME = me

# List of C source files.
CSRCS = main.c ui.c keygen.c hd.c jpeg.c sss.c layout.c qr.c rng.c data.c \
	at25dfx_mem.c xflash.c blkbuf.c xflash_buf_mem.c me-access.c \
	fs.c update.c settings.c diskio.c ctrl_access.c \
	jpeg-data.c jpeg-data-ext.c

# List of objects that should be linked explicitly, rather than taken from
# a library.  This is useful to override library implementation (e.g. from ASF)
# with a project-specific one without affecting other projects.
ALWAYS_LINK = diskio.o ctrl_access.o

# List of assembler source files.
ASSRCS = 

# Extra flags to use when compiling.
CFLAGS =

# Extra flags to use when preprocessing.
CPPFLAGS = \
	-DNDEBUG \
	$(shell LANG=C date -u +"-DBUILD_TIME=%sULL \
		-DBUILD_DAY=%-d -DBUILD_MONTH=%-m -DBUILD_YEAR=%Y \
		-DBUILD_HOUR=%-H -DBUILD_MIN=%-M -DBUILD_SEC=%-S")

LDFLAGS = $(BUILD_DIR)/$(PLATFORM)/applet-bin.o

# Additional items to clean under BUILD_DIR
CLEAN = $(PLATFORM)/applet-bin.c

include ../config.mk

$(BUILD_DIR)/me.elf: $(BUILD_DIR)/$(PLATFORM)/applet-bin.o

$(BUILD_DIR)/applet.elf: $(BUILD_DIR)/applet.o $(BUILD_DIR)/$(APPNAME).a \
		$(linker_script) applet.ld Makefile config.mk \
		$(TOP)/config.mk $(TOP)/platforms/$(PLATFORM)/config.mk
	@echo $(MSG_LINKING)
	$(Q)$(CC) -mcpu=cortex-m4 -mthumb -Wl,--gc-sections \
		-Wl,-e,applet,--defsym,__stack_size__=128 \
		-Wl,-T,$(linker_script) -Wl,-Map=$(@:.elf=.map),--cref \
		-nostdlib -Wl,-T,applet.ld \
		$< $(BUILD_DIR)/$(APPNAME).a -lc_s -lgcc -o $@
	$(Q)$(SIZE) $@

$(BUILD_DIR)/applet-bin.c: $(BUILD_DIR)/applet.elf $(BUILD_DIR)/applet.bin applet.awk
	@echo "XXD     $@"
	$(Q)$(NM) $< | awk -f applet.awk > $@
	$(Q)(cd $(dir $<); xxd -i $(notdir $(<:.elf=.bin)) | sed '/=/s/^/const /') >> $@

# Generate JPEG data if Python Imaging Library is installed
ifeq ($(shell cat $(BUILD_DIR)/jpeg-check || \
	python tools/gen.py --check 2>&1 | tee $(BUILD_DIR)/jpeg-check),OK)

# tools/gen.py generates all jpeg-data* files in one go;
# we pick .h as the target of the main rule and make everything else depend on it.
jpeg-data.h: tools/gen.py
	python $^ $(basename $@)

jpeg-data.c jpeg-data-ext.c: jpeg-data.h

CLEAN += ../jpeg-data.bin ../jpeg-data.jpg

endif
