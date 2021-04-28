#!/bin/bash

# Copyright (c) 2021  Westermo Network Technologies AB
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

# Example:
# Mount filesystem 3 in foo.bin
# $ ./mount.sh 3 foo.bin mnt/

SCRIPTPATH="$(cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P)"

if [ "$EUID" -ne 0 ]; then
    echo "Needs to be executed by root (for mount -o)"
    exit 1
fi

if [ $# -ne 3 ]; then
    echo "Usage: $0 FILESYSTEM IMAGE MOUNT-POINT"
    exit 1
fi
filesys="$1"
image="$2"
mp="$3"

if [ ! -d "$mp" ]; then
    echo "Error, mount point $mp not found"
    exit 1
fi

offset="$(${SCRIPTPATH}/../sqsq-image -n "$filesys" offset "$image")"

mount -o loop,offset="$offset" "$image" "$mp"
if [ $? -ne 0 ]; then
    echo "Error, mounting filesystem $filesys in $mp"
    exit 1
fi
echo "Successfully mounted filesystem $filesys in $mp"
