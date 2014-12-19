#!/bin/bash
#
# Setup script for the Initial Flash Programming suite for Ubuntu.
#
# Copyright 2014 Mycelium SA, Luxembourg.
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

set +e

SRC=80-entropy.rules
DEST=/etc/udev/rules.d/$SRC

awk '{ gsub(/CWD/, "'`pwd`'"); print; }' $SRC | sudo tee $DEST >/dev/null
echo "Rules written to $DEST."
touch log.txt
echo "Log will be written to log.txt."
echo "You can monitor it with this command:"
echo "    tail -f log.txt"
