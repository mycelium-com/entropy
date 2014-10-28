#
# SAM4L Bootloader with USB MSC firmware update
#
# The author has waived all copyright and related or neighbouring rights
# to this file and placed it in public domain.

ifdef ASF

#PLATFORM = xpro-kit
PLATFORM = entropy-1.0

# Include the common Makefile, which will also include the project specific
# config.mk file.
MAKEFILE_PATH = $(ASF)/sam/utils/make/Makefile.sam.in
include $(MAKEFILE_PATH)
VPATH += $(TOP)
c_flags += -Wno-inline -Wno-aggregate-return
LD := $(CC)

else

# ASF environment variable must be defined.
# Show an error message.
.PHONY: all $(MAKECMDGOALS)
all $(MAKECMDGOALS):
	@echo "Please set ASF environment variable to the path to"
	@echo "Atmel Software Framework."
	@false

endif
