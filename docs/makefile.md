# Makefile

The cyanea-config requires a '*configs.in*' file which specifies the configuration variables and default values, see [language](https://github.com/amrzar/cyanea-uconfig/blob/master/docs/parser.md "Language"). cyanea-config generates '*.old.config*' file in a same directory as the '*configs.in*' for its internal use. It also generates '*sys.config.h*' file as output to be consumed by user.

## Makefile targets

- **defconfig** Generates '*.old.config*' file from input '*configs.in*' file.
**Note:**  after any modification to '*configs.in*', user should run `make defconfig`, to create '*.old.config*' form the new '*configs.in*', *all existing configuration will be lost*!
- **silentoldconfig** Generates '*sys.config.h*' file from the existing '*.old.config*'.
- **menuconfig** Opens a GUI, and generates '*sys.config.h*'.

## Makefile variables

- **I** path to input '*configs.in*' file.
- **OUT** path to output '*sys.config.h*' file.
- **HOSTCC** host compiler
- **HOSTCFLAGS** compiler flags

## Example

`make HOSTCC=gcc HOSTCFLAGS='-Wall -Wstrict-prototypes -Wno-unused-function -O2' I=configs.in OUT=sys.config.h menuconfig`
