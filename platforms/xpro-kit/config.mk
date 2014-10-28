# Platform-specific Makefile fragment.

# Target CPU architecture: cortex-m3, cortex-m4
ARCH = cortex-m4

# Target part: none, sam3n4 or sam4l4aa
PART = sam4lc4c

CSRCS += \
	platforms/$(PLATFORM)/lcd.c                        \
	platforms/$(PLATFORM)/temperature.c                \
	common/components/display/ssd1306/font.c           \
	common/components/display/ssd1306/ssd1306.c        \
	common/components/memory/sd_mmc/sd_mmc.c           \
	common/components/memory/sd_mmc/sd_mmc_mem.c       \
	common/components/memory/sd_mmc/sd_mmc_spi.c       \
	common/services/spi/sam_spi/spi_master.c           \
	sam/drivers/lcdca/lcdca.c                          \
	sam/drivers/spi/spi.c                              \
	sam/boards/sam4l_xplained_pro/init.c

INC_PATH += \
	common/components/memory/sd_mmc                    \
	common/components/display/ssd1306                  \
	sam/drivers/lcdca                                  \
	sam/drivers/spi                                    \
	sam/boards/sam4l_xplained_pro

# The most relevant symbols to define for the preprocessor are:
#   BOARD      Target board in use, see boards/board.h for a list.
#   EXT_BOARD  Optional extension board in use, see boards/board.h for a list.
CPPFLAGS += \
	-D ARM_MATH_CM4=true        \
	-D BOARD=SAM4L_XPLAINED_PRO \
	-D __SAM4LC4C__

override linker_script = $(TOP)/sam4l4.ld
