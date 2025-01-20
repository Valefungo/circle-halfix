# Building and Running notes

Steps:
- clone circle-stdlib
- read its README and INSTALL, you'll need the aarch64 ARM compiler so follow all instructions to build it
- the configure script needs to be modified to add -fpermissive otherwise the libm will not build
- as per Circle doc https://github.com/rsta2/circle/blob/master/doc/multicore.txt ARM_ALLOW_MULTI_CORE needs to be enabled in circle-stdlib/libs/circle/include/circle/sysconfig.h
- build everything
- finally circle-halfix can be built

Circle-stdlib configure

```
./configure -p aarch64-none-elf- -r 3 --qemu -o DEPTH=32 -o KERNEL_MAX_SIZE=0x8000000 -d

 -r 3 : raspberry version, use 3 or 4
 --qemu: QEMU support, will not work with rpi4
 -o K...: we need a larger kernel size to fit all
 -d: enable debugging
```

