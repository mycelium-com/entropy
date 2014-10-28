#
# Platform- and project-independent library.
#
# This is for all generally useful stuff that does not depend on what
# platform we are building for, nor on project specific settings.
# This includes crypto, libc replacements etc.
#
# The author has waived all copyright and related or neighbouring rights
# to this file and placed it in public domain.

ifdef ASF

# Include the common Makefile, which will also include the project specific
# config.mk file.
MAKEFILE_PATH = $(ASF)/sam/utils/make/Makefile.sam.in
include $(MAKEFILE_PATH)
cflags-gnu-y += -Wno-inline -Wno-long-long

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
