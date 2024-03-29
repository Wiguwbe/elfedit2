# elfedit (2)

_A way to directly change values on an ELF file_

This tool (named `elfedit2` to not collied with binutil's `elfedit`), allows for
someone to change the value of a (global) constant on an elf file.

Considering you have a const variable:
```C
// `volatile` to avoid optimizations
const volative int CONFIG_INT = 6;
// ...
```

You can edit it's value with
```bash
elfedit2 <your-elf-file> CONFIG_INT=7
```

In effect setting its value to 7.

(See test.c for an example)

### Strings

Parsing the arguments, if the value is not a valid integer, it is assumed to be a string.

To update/set/override strings, you need to define the space for it:
```C
const char CONFIG_STR[] = "placeholder";
// or set the size
const char CONFIG_STR[32];
// or set size and default value
const char CONFIG_STR[32] = "/tmp";

// can't do pointers tho...
```

And the command interface is the same
```bash
elfedit2 <your-elf-file> CONFIG_STR=/tmp/my-program
```

The tool will complain if the provided string would not fit (including null terminator)
and will also pad the remaining of the space with zeroes.

(See test-string.c/sh for examples)

## Why ??

In one of my projects, being developed in C, I found myself needing to
have some configuration values coming from the outside,

As per usual, I didn't settle for the "normal" way (command line argument
or environment variable) as the need to parse that way would be ...
something I'd rather not do.

So I came up with this tool.

You create a "placeholder" for the configuration value/constant, and override
it externally.

## Build

Simply run `gcc elfedit2.c -o elfedit` and copy/move it wherever you want
(no need for makefiles or cmakes)
