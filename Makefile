#
# This file is part of Mycelium Entropy.
#
# Mycelium Entropy is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.  See file GPL in the source code
# distribution or <http://www.gnu.org/licenses/>.
#
# Mycelium Entropy is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
#
# The author has waived all copyright and related or neighbouring rights
# to this file and placed this file in public domain.
#

ifdef ASF

.PHONY: all clean tags

SIGN ?= 1
ifeq ($(SIGN),0)
XSGN =
else
XSGN = sign
endif


all:
	$(MAKE) -C lib  	# platform- and project-independent library
	$(MAKE) -C sam-ba-4l	# improved SAM-BA bootloader
	$(MAKE) -C sign		# signing tool
	$(MAKE) -C boot	$(XSGN)	# alternative bootloader (Mass Storage)
	$(MAKE) -C brt 	$(XSGN)	# bootloader replacement tool
	$(MAKE) -C hwtest	# Mycelium Entropy hardware test
	$(MAKE) -C factory	# initial factory firmware and self test
	$(MAKE) -C collect	# entropy collection tool
	$(MAKE) -C me	$(XSGN)	# Mycelium Entropy main firmware

clean:
	$(MAKE) -C me clean
	$(MAKE) -C collect clean
	$(MAKE) -C factory clean
	$(MAKE) -C hwtest clean
	$(MAKE) -C brt clean
	$(MAKE) -C boot clean
	$(MAKE) -C sign clean
	$(MAKE) -C sam-ba-4l clean
	$(MAKE) -C lib clean

tags:
	ctags -R $(ASF)/sam $(ASF)/thirdparty $(ASF)/common2 \
		$(ASF)/common .

else

# ASF environment variable must be defined.
# Show an error message.
.PHONY: all $(MAKECMDGOALS)
all $(MAKECMDGOALS):
	@echo "Please set ASF environment variable to the path to"
	@echo "Atmel Software Framework."
	@false

endif
