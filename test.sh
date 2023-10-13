#!/bin/sh

set -e

# compile
gcc test.c -o test.out
gcc elfedit2.c -o elfedit2

# default values

test "$(./test.out)" = "616 4 -2 66"

# set byte
./elfedit2 test.out CONFIG_BYTE=67
test "$(./test.out)" = "616 4 -2 67"

# set short
./elfedit2 test.out CONFIG_NEG=2
test "$(./test.out)" = "616 4 2 67"

# set int (negative just because)
./elfedit2 test.out CONFIG_INT=-7
test "$(./test.out)" = "616 -7 2 67"

# set long
./elfedit2 test.out CONFIG_LONG=420
test "$(./test.out)" = "420 -7 2 67"

echo "All good!"
