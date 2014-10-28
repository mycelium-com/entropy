# Application name.
APPNAME = sign

# List of C source files.
CSRCS = main.c me/rng.c oled.c devctrl.c

# List of assembler source files.
ASSRCS = 

# Extra flags to use when compiling.
CFLAGS =

# Extra flags to use when preprocessing.
CPPFLAGS = \
	-DRNG_NO_FLASH \
	-DNDEBUG

include ../config.mk
