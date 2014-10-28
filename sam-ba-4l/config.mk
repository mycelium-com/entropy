#
# SAM4L-SAM-BA Bootloader
#

# Path to top level ASF directory relative to this project directory.
PRJ_PATH = $(ASF)

# Target CPU architecture: cortex-m3, cortex-m4
ARCH = cortex-m4

# Target part: none, sam3n4 or sam4l4aa
PART = sam4lc4c

BUILD_DIR = obj

# Application target name. Given with suffix .a for library and .elf for a
# standalone application.
TARGET_FLASH = $(BUILD_DIR)/samba.elf

# List of C source files.
CSRCS = \
	main.c                                              \
	sam_ba_monitor.c                                    \
	usart_sam_ba.c                                      \
	common/services/clock/sam4l/osc.c                   \
	common/services/clock/sam4l/pll.c                   \
	common/services/clock/sam4l/sysclk.c                \
	common/services/sleepmgr/sam4l/sleepmgr.c           \
	common/services/usb/class/cdc/device/udi_cdc.c      \
	common/services/usb/class/cdc/device/udi_cdc_desc.c \
	common/services/usb/udc/udc.c                       \
	common/utils/interrupt/interrupt_sam_nvic.c         \
	sam/drivers/bpm/bpm.c                               \
	sam/drivers/flashcalw/flashcalw.c                   \
	sam/drivers/usbc/usbc_device.c                      \
	sam/utils/cmsis/sam4l/source/templates/exceptions.c \
	sam/utils/cmsis/sam4l/source/templates/gcc/startup_sam4l.c

# List of assembler source files.
ASSRCS =

# List of include paths.
INC_PATH = \
	common/boards                                      \
	common/services/clock                              \
	common/services/ioport                             \
	common/services/sleepmgr                           \
	common/services/usb/class/cdc                      \
	common/services/usb/class/cdc/device               \
	common/services/usb                                \
	common/services/usb/udc                            \
	common/utils                                       \
	sam/boards                                         \
	sam/drivers/bpm                                    \
	sam/drivers/eic                                    \
	sam/drivers/flashcalw                              \
	sam/drivers/gpio                                   \
	sam/drivers/usbc                                   \
	sam/drivers/wdt                                    \
	sam/utils                                          \
	sam/utils/cmsis/sam4l/include                      \
	sam/utils/cmsis/sam4l/source/templates             \
	sam/utils/header_files                             \
	sam/utils/preprocessor                             \
	thirdparty/CMSIS/Include

# Additional search paths for libraries.
LIB_PATH =

# List of libraries to use during linking.
LIBS =

# Path relative to top level directory pointing to a linker script.
#LINKER_SCRIPT_FLASH = sam/utils/linker_scripts/sam4l/sam4l4/gcc/flash.ld

# Project type parameter: all, sram or flash
PROJECT_TYPE        = flash

# Additional options for debugging. By default the common Makefile.in will
# add -g3.
DBGFLAGS =

# Application optimization used during compilation and linking:
# -O0, -O1, -O2, -O3 or -Os
OPTIMIZATION = -Os

# Extra flags to use when archiving.
ARFLAGS =

# Extra flags to use when assembling.
ASFLAGS =

# Extra flags to use when compiling.
CFLAGS =

# Extra flags to use when preprocessing.
#
# Preprocessor symbol definitions
#   To add a definition use the format "-D name[=definition]".
#   To cancel a definition use the format "-U name".
#
# The most relevant symbols to define for the preprocessor are:
#   BOARD      Target board in use, see boards/board.h for a list.
#   EXT_BOARD  Optional extension board in use, see boards/board.h for a list.
CPPFLAGS = \
	-I .                        \
	-D BOARD=SAM4L_XPLAINED_PRO \
	-D __SAM4LC4C__             \
	-D printf=iprintf           \
	-D scanf=iscanf

# Extra flags to use when linking
LDFLAGS = -nostartfiles
override linker_script = samba.ld
