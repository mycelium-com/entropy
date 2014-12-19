#!/bin/bash
#
# Initial Flash Programming script for Ubuntu.
# This works as a udev handler according to 80-entropy.rules.
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

cd `dirname $0`
LOG=log.txt
USB=`echo $ID_PATH | sed 's/^.*-\(usb-[^-]*\)-.*$/\1/'`
ON_CHIP=$(($1 & 1023))
echo "<" $USB $DEVNAME $1 $ID_MODEL >> $LOG
if [ $ON_CHIP -lt 500 ]; then
	dd if=factory.bin of=$DEVNAME seek=32
else
	dd if=me-1.0.bin of=$DEVNAME seek=32
fi
./ifp.py $DEVNAME $1 $ID_MODEL
echo "  $USB" $DEVNAME ">" >> $LOG
