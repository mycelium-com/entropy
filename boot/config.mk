#
# SAM4L Bootloader with USB MSC firmware update
#

# Application name.
APPNAME = boot

# List of C source files.
CSRCS = startup.c main.c fw-access.c at25dfx_mem.c ui.c \
	updfs-128.c updfs-256.c layfs-256.c readme.c

# List of assembler source files.
ASSRCS =

# Project type parameter: all, sram or flash
PROJECT_TYPE = sram

# Extra flags to use when preprocessing.
CPPFLAGS = -DNDEBUG

# Extra flags to use when linking
LDFLAGS = -nostartfiles -Wl,--entry=flash_startup,--undef,flash_exception_table $(PLATFORM)/main.o

include ../config.mk

OPTIMIZATION += -g0 -frename-registers -fsee -fno-function-cse -fomit-frame-pointer

override linker_script = boot.ld
