# Platform-specific Makefile fragment.

# Target CPU architecture: cortex-m3, cortex-m4
ARCH = cortex-m4

# Target part: none, sam3n4 or sam4l4aa
PART = sam4ls4a

CSRCS += \
	platforms/$(PLATFORM)/init.c \
	platforms/$(PLATFORM)/usart_spi.c \
	platforms/$(PLATFORM)/at25dfx.c \
	common/components/memory/serial_flash/at25dfx/at25dfx_hal_spi.c

INC_PATH += \
	common/components/memory/serial_flash/at25dfx   \
	common/services/spi/sam_usart_spi

# The most relevant symbols to define for the preprocessor are:
#   BOARD      Target board in use, see boards/board.h for a list.
#   EXT_BOARD  Optional extension board in use, see boards/board.h for a list.
CPPFLAGS += \
	-D ARM_MATH_CM4=true \
	-D BOARD=USER_BOARD  \
	-D __SAM4LS4A__ #__SAM4LS2A__

override linker_script = $(TOP)/sam4l4.ld
