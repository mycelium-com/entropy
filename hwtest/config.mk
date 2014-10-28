# Application name.
APPNAME = hwtest

# List of C source files.
CSRCS = main.c ui.c xflash.c fs-1.c

# List of assembler source files.
ASSRCS = 

# Extra flags to use when compiling.
CFLAGS =

# Extra flags to use when preprocessing.
CPPFLAGS = -DNDEBUG

include ../config.mk
