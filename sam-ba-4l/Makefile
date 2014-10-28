#
# SAM4L-SAM-BA Bootloader
#

ifdef ASF

# Include the common Makefile, which will also include the project specific
# config.mk file.
MAKEFILE_PATH = $(ASF)/sam/utils/make/Makefile.sam.in
include $(MAKEFILE_PATH)
c_flags += -Wno-inline -Wall -Werror
LD := $(CC)

ifeq ($(strip $(shell uname)),Darwin)
RMDIR := rmdirnf() { rmdir 2>/dev/null "$$@" || true; }; rmdirnf -p
endif

else

# ASF environment variable must be defined.
# Show an error message.
.PHONY: all $(MAKECMDGOALS)
all $(MAKECMDGOALS):
	@echo "Please set ASF environment variable to the path to"
	@echo "Atmel Software Framework."
	@false

endif
