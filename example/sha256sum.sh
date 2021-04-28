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
# Calc sha256 sum of filesystem 3 in foo.bin
# $ ./sha256sum.sh 3 foo.bin

SCRIPTPATH="$(cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P)"

if [ $# -ne 2 ]; then
    echo "Usage: $0 FILESYSTEM IMAGE"
    exit 1
fi
filesys="$1"
image="$2"

offset="$(${SCRIPTPATH}/../sqsq-image -n "$filesys" offset "$image")"
size="$(${SCRIPTPATH}/../sqsq-image -n "$filesys" size "$image")"

dd if="$image" \
   bs=4096 \
   skip="$offset" \
   count="$size" \
   iflag=skip_bytes,count_bytes 2>/dev/null | sha256sum
