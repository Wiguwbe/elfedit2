#!/bin/sh

set -e

# compile
gcc elfedit2.c -o elfedit2
gcc test-string.c -o test-string.out

# default (assuming linker zeroes-out global unitialized memory)
test "$(./test-string.out)" = "'Hello world!':''"

# override hello world
./elfedit2 test-string.out CONFIG_STR_VALUE="yes, hi"
test "$(./test-string.out)" = "'yes, hi':''"

# set value on placeholder
./elfedit2 test-string.out CONFIG_STR_EMPTY="this now has a value!"
test "$(./test-string.out)" = "'yes, hi':'this now has a value!'"

# set too big
./elfedit2 test-string.out CONFIG_STR_VALUE="this string is way bigger than the expected value" \
2>/dev/null || true

echo "All good!"

