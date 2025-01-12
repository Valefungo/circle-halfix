#
# Makefile
#

include ../../Config.mk

CIRCLEHOME = ../../libs/circle
NEWLIBDIR = ../../install/$(NEWLIB_ARCH)

PNG = \
  extra/minipng/pngread.o extra/minipng/pngmem.o extra/minipng/pngpread.o extra/minipng/pngrutil.o \
  extra/minipng/pngwio.o extra/minipng/pngrio.o extra/minipng/pngtrans.o extra/minipng/pngget.o \
  extra/minipng/png.o extra/minipng/pngwtran.o extra/minipng/pngrtran.o extra/minipng/pngwrite.o \
  extra/minipng/pngwutil.o extra/minipng/pngerror.o extra/minipng/pngset.o

ZIP = \
  extra/minizip/gzlib.o extra/minizip/crc32.o extra/minizip/gzread.o extra/minizip/adler32.o \
  extra/minizip/inffast.o extra/minizip/inftrees.o extra/minizip/trees.o extra/minizip/uncompr.o \
  extra/minizip/deflate.o extra/minizip/gzwrite.o extra/minizip/gzclose.o extra/minizip/zutil.o \
  extra/minizip/compress.o extra/minizip/inflate.o extra/minizip/infback.o

# 
#

O2 = src/main.o src/host/net-none.o src/pc.o src/io.o  \
src/display.o src/drive.o src/ini.o src/util.o src/state.o \
src/cpu/access.o src/cpu/trace.o src/cpu/seg.o src/cpu/cpu.o src/cpu/mmu.o src/cpu/ops/ctrlflow.o src/cpu/smc.o src/cpu/decoder.o src/cpu/eflags.o src/cpu/prot.o src/cpu/opcodes.o \
src/cpu/ops/arith.o src/cpu/ops/io.o src/cpu/ops/string.o src/cpu/ops/stack.o src/cpu/ops/misc.o src/cpu/ops/bit.o src/cpu/ops/simd.o \
src/cpu/softfloat.o src/cpu/fpu.o \
src/hardware/dma.o src/hardware/cmos.o src/hardware/pit.o src/hardware/pic.o src/hardware/kbd.o src/hardware/vga.o src/hardware/ide.o src/hardware/pci.o src/hardware/apic.o src/hardware/ioapic.o src/hardware/fdc.o src/hardware/acpi.o

OBJS    = main.o kernel.o oscillator.o $(O2) $(ZIP) $(PNG)

include $(CIRCLEHOME)/Rules.mk

# CFLAGS += -I "$(NEWLIBDIR)/include" -I $(STDDEF_INCPATH) -I. -Iinclude -Iextra/minizip -Iextra/minipng -I ../../include -DLOGGING_DISABLED -DNATIVE_BUILD -DPNG_ARM_NEON_OPT=0 -O3
CFLAGS += -I "$(NEWLIBDIR)/include" -I $(STDDEF_INCPATH) -I. -Iinclude -Iextra/minizip -Iextra/minipng -I ../../include -DLOGGING_DISABLED -DNATIVE_BUILD -DPNG_ARM_NEON_OPT=0 -ggdb -O0
LIBS := "$(NEWLIBDIR)/lib/libm.a" "$(NEWLIBDIR)/lib/libc.a" "$(NEWLIBDIR)/lib/libcirclenewlib.a" \
 	$(CIRCLEHOME)/addon/SDCard/libsdcard.a \
  	$(CIRCLEHOME)/lib/usb/libusb.a \
 	$(CIRCLEHOME)/lib/input/libinput.a \
 	$(CIRCLEHOME)/addon/fatfs/libfatfs.a \
 	$(CIRCLEHOME)/lib/fs/libfs.a \
  	$(CIRCLEHOME)/lib/net/libnet.a \
  	$(CIRCLEHOME)/lib/sched/libsched.a \
	$(CIRCLEHOME)/lib/sound/libsound.a \
  	$(CIRCLEHOME)/lib/libcircle.a

-include $(DEPS)

depclean:
	find . -name "*.d" -exec rm {} \;

mrproper: clean depclean
	rm -f $(OBJS)


