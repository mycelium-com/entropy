# Application name.
APPNAME = ifp

# Top level directory, relative to the directory where make is running.
# The default value .. is overridden here.
TOP = ../..

# Project type parameter: all, sram or flash
PROJECT_TYPE = sram

# List of C source files.
CSRCS = main.c ui.c fw-access.c

# List of assembler source files.
ASSRCS = 

# Extra flags to use when compiling.
CFLAGS =

# Extra flags to use when preprocessing.
CPPFLAGS = -DNDEBUG

# Additional items to clean under BUILD_DIR
CLEAN =

include $(TOP)/config.mk
