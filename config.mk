#
# Copyright (c) 2011 Atmel Corporation. All rights reserved.
#
# \asf_license_start
#
# \page License
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# 3. The name of Atmel may not be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# 4. This software may only be redistributed and used in connection with an
#    Atmel microcontroller product.
#
# THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
# EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
# STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
# ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#
# \asf_license_stop
#

# Adapted for Mycelium Entropy by Mycelium SA, Luxembourg.
# Mycelium SA have waived all copyright and related or neighbouring rights
# to their changes to this file and placed these changes in public domain.

# Path to top level ASF directory relative to this project directory.
PRJ_PATH = $(ASF)

BUILD_DIR = $(PLATFORM)

# Top level directory for all projects, relative to the directory where make
# is running.  Can be overridden in the project-platform config.mk.
TOP = ..

# Application target name. Given with suffix .a for library and .elf for a
# standalone application.
TARGET_FLASH = $(BUILD_DIR)/$(APPNAME).a
TARGET_SRAM  = $(BUILD_DIR)/$(APPNAME).a

# List of C source files.
CSRCS += \
	sys/stdio-uart.c                                   \
	sys/sync.c                                         \
	common/components/memory/virtual_mem/virtual_mem.c \
	common/services/clock/sam4l/dfll.c                 \
	common/services/clock/sam4l/osc.c                  \
	common/services/clock/sam4l/pll.c                  \
	common/services/clock/sam4l/sysclk.c               \
	common/services/delay/sam/cycle_counter.c          \
	common/services/serial/usart_serial.c              \
	common/services/sleepmgr/sam4l/sleepmgr.c          \
	common/services/storage/ctrl_access/ctrl_access.c  \
	common/services/usb/class/msc/device/udi_msc.c     \
	common/services/usb/class/msc/device/udi_msc_desc.c \
	common/services/usb/udc/udc.c                      \
	common/utils/interrupt/interrupt_sam_nvic.c        \
	sam/drivers/adcife/adcife.c                        \
	sam/drivers/ast/ast.c                              \
	sam/drivers/bpm/bpm.c                              \
	sam/drivers/eic/eic.c                              \
	sam/drivers/flashcalw/flashcalw.c                  \
	sam/drivers/freqm/freqm.c                          \
	sam/drivers/gpio/gpio.c                            \
	sam/drivers/pdca/pdca.c                            \
	sam/drivers/tc/tc.c                                \
	sam/drivers/twim/twim.c                            \
	sam/drivers/usart/usart.c                          \
	sam/drivers/usbc/usbc_device.c                     \
	sam/drivers/wdt/wdt_sam4l.c                        \
	sam/utils/cmsis/sam4l/source/templates/system_sam4l.c \
	sam/utils/syscalls/gcc/syscalls.c                  \
	thirdparty/fatfs/fatfs-port-r0.09/diskio.c         \
	thirdparty/fatfs/fatfs-port-r0.09/sam/fattime_ast.c \
	thirdparty/fatfs/fatfs-r0.09/src/ff.c              \
	thirdparty/fatfs/fatfs-r0.09/src/option/ccsbcs.c

# List of assembler source files.
ASSRCS += 

# List of include paths.
INC_PATH += \
	common/boards                                      \
	common/components/memory/virtual_mem               \
	common/services/clock                              \
	common/services/delay                              \
	common/services/ioport                             \
	common/services/spi                                \
	common/services/serial                             \
	common/services/serial/sam_uart                    \
	common/services/sleepmgr                           \
	common/services/storage/ctrl_access                \
	common/services/usb                                \
	common/services/usb/class/cdc                      \
	common/services/usb/class/cdc/device               \
	common/services/usb/class/msc                      \
	common/services/usb/class/msc/device               \
	common/services/usb/udc                            \
	common/utils                                       \
	sam/boards                                         \
	sam/drivers/adcife                                 \
	sam/drivers/ast                                    \
	sam/drivers/bpm                                    \
	sam/drivers/eic                                    \
	sam/drivers/flashcalw                              \
	sam/drivers/freqm                                  \
	sam/drivers/gpio                                   \
	sam/drivers/pdca                                   \
	sam/drivers/spi                                    \
	sam/drivers/tc                                     \
	sam/drivers/twim                                   \
	sam/drivers/usart                                  \
	sam/drivers/usbc                                   \
	sam/drivers/wdt                                    \
	sam/utils                                          \
	sam/utils/cmsis/sam4l/include                      \
	sam/utils/cmsis/sam4l/source/templates             \
	sam/utils/header_files                             \
	sam/utils/preprocessor                             \
	thirdparty/CMSIS/Include                           \
	thirdparty/CMSIS/Lib/GCC                           \
	thirdparty/fatfs/fatfs-port-r0.09/sam              \
	thirdparty/fatfs/fatfs-r0.09/src

# Additional search paths for libraries.
LIB_PATH += \
	thirdparty/CMSIS/Lib/GCC                          

# List of libraries to use during linking.
LIBS += \
	arm_cortexM4l_math                                 \
	m                                                 

# Project type parameter: all, sram or flash
PROJECT_TYPE ?= flash

# Additional options for debugging. By default the common Makefile.in will
# add -g3.
DBGFLAGS +=

# Application optimization used during compilation and linking:
# -O0, -O1, -O2, -O3 or -Os
OPTIMIZATION = -Os -g0

# Extra flags to use when archiving.
# Erase the old value, which make sets to 'rv'.
ARFLAGS = 

# Extra flags to use when assembling.
ASFLAGS += 

# Extra flags to use when compiling.
CFLAGS += -Wall -Werror

# Extra flags to use when preprocessing.
#
# Preprocessor symbol definitions
#   To add a definition use the format "-D name[=definition]".
#   To cancel a definition use the format "-U name".
#
CPPFLAGS += \
	-I . \
	-I $(PLATFORM) \
	-I $(TOP)/platforms/$(PLATFORM) \
	-I $(TOP)/lib \
	-I $(TOP) \
	-D scanf=iscanf

# Extra flags to use when linking
LDFLAGS +=

# Platform-specific definitions
include $(TOP)/platforms/$(PLATFORM)/config.mk

all: $(BUILD_DIR)/$(APPNAME).elf $(BUILD_DIR)/$(APPNAME).bin

# Additional dependencies not accounted for by the ASF's Makefile.sam.in.
%.o: Makefile $(TOP)/config.mk $(TOP)/platforms/$(PLATFORM)/config.mk

vectors = $(BUILD_DIR)/sam/utils/cmsis/sam4l/source/templates/gcc/startup_sam4l.o \
	$(BUILD_DIR)/sam/utils/cmsis/sam4l/source/templates/exceptions.o

# Link the object files into an ELF file. Also make sure the target is rebuilt
# if Makefile or project config.mk files are changed.
# LD is set to g++ in ASF, but we have no C++ code in this project, so we use
# $(CC) instead of $(LD) for linking.
$(BUILD_DIR)/$(APPNAME).elf: $(vectors) \
		$(BUILD_DIR)/$(APPNAME).a $(TOP)/lib/obj/lib.a \
		$(linker_script) $(TOP)/$(PROJECT_TYPE).ld \
		Makefile config.mk \
		$(TOP)/config.mk $(TOP)/platforms/$(PLATFORM)/config.mk
	@echo $(MSG_LINKING)
	$(Q)$(CC) $(l_flags) -Wl,-T,$(TOP)/$(PROJECT_TYPE).ld \
		$(vectors) $(addprefix $(BUILD_DIR)/,$(ALWAYS_LINK)) \
		-Wl,--start-group $(filter %.a,$^) -Wl,--end-group \
		$(libflags-gnu-y) -o $@
	@echo $(MSG_SIZE)
	$(Q)$(SIZE) -Ax $@
	$(Q)$(SIZE) -Bx $@

# Create binary image from ELF output file.
%.bin: %.elf
	@echo $(MSG_BINARY_IMAGE)
	$(Q)$(OBJCOPY) -O binary $< $@

# Attach signature block to the image.
sign: $(BUILD_DIR)/$(APPNAME).bin
	$(TOP)/sign/sign.py $^

# Additional clean actions.
clean: clean-more

more-to-clean = $(wildcard $(BUILD_DIR)/$(APPNAME).elf \
			   $(vectors:.o=.[od]) \
			   $(addprefix $(BUILD_DIR)/,$(CLEAN)))

clean-more:
	$(if $(more-to-clean),$(Q)$(RM) $(more-to-clean))
	$(if $(wildcard $(dir $(vectors))),$(Q)$(RMDIR) $(dir $(vectors)))

.PHONY: clean-more sign

ifeq ($(strip $(shell uname)),Darwin)
override RMDIR := rmdirnf() { rmdir 2>/dev/null "$$@" || true; }; rmdirnf -p
endif
