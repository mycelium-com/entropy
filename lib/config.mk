#
# Platform-independent library.
#

# Path to top level ASF directory relative to this project directory.
PRJ_PATH = $(ABM)

BUILD_DIR = obj

# Application target name. Given with suffix .a for library and .elf for a
# standalone application.
TARGET_FLASH = $(BUILD_DIR)/lib.a

# Target CPU architecture: cortex-m3, cortex-m4
ARCH = cortex-m4

# Target part: none, sam3n4 or sam4l4aa
PART = sam4lc4c

# List of C source files.
CSRCS = \
	assert.c \
	base58enc.c \
	bignum.c \
	debug.c \
	ecdsa.c \
	fwsign.c \
	hex.c \
	printf.c \
	ripemd.c \
	rs-enc.c \
	secp256k1.c \
	sha256.c \
	sha512.c \
	stubs.c \
	xxtea.c

# List of assembler source files.
ASSRCS = 

# List of include paths.
INC_PATH =

# Additional search paths for libraries.
LIB_PATH =

# List of libraries to use during linking.
LIBS =

# Project type parameter: all, sram or flash
PROJECT_TYPE = flash

# Additional options for debugging. By default the common Makefile.in will
# add -g3.
DBGFLAGS = 

# Application optimization used during compilation and linking:
# -O0, -O1, -O2, -O3 or -Os
OPTIMIZATION = -Os -g0

# Extra flags to use when archiving.
ARFLAGS = 

# Extra flags to use when assembling.
ASFLAGS = 

# Extra flags to use when compiling.
CFLAGS = 

# Extra flags to use when preprocessing.
CPPFLAGS = -D NDEBUG -I.

# Extra flags to use when linking
LDFLAGS =
