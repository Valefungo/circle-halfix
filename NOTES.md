Building and Running notes

First: clone circle-stdlib

Second: read its README and INSTALL, you'll need the aarch64 ARM compiler

Circle-stdlib configure

** ./configure -p aarch64-none-elf- -r 3 --qemu -o DEPTH=32 -o KERNEL_MAX_SIZE=0x8000000 -d **

-r 3 : raspberry version, use 3 or 4

--qemu: QEMU support, will not work with rpi4

-o K...: we need a larger kernel size to fit all

-d: enable debugging

