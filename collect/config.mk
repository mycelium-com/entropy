# Application name.
APPNAME = collect

# List of C source files.
CSRCS = entropy.c

# List of assembler source files.
ASSRCS = 

# Extra flags to use when compiling.
CFLAGS =

# Extra flags to use when preprocessing.
CPPFLAGS = \
	-DNDEBUG

include ../config.mk
