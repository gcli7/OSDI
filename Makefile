# Makefile for the simple kernel.
CC	=gcc
AS	=as
LD	=ld
OBJCOPY = objcopy
OBJDUMP = objdump
NM = nm

CFLAGS = -m32 -Wall -O -fstrength-reduce -fomit-frame-pointer -finline-functions -nostdinc -fno-builtin 

# Add debug symbol
CFLAGS += -g

CFLAGS += -I.

OBJDIR = .


include boot/Makefile
include kernel/Makefile

all: clean boot/boot kernel/system
	dd if=/dev/zero of=$(OBJDIR)/kernel.img count=10000 2>/dev/null
	dd if=$(OBJDIR)/boot/boot of=$(OBJDIR)/kernel.img conv=notrunc 2>/dev/null
	dd if=$(OBJDIR)/kernel/system of=$(OBJDIR)/kernel.img seek=1 conv=notrunc 2>/dev/null

run:
	qemu-system-i386 -hda kernel.img --curses

debug:
	qemu-system-i386 -hda kernel.img --curses -s -S

clean:
	rm -f $(OBJDIR)/boot/*.o $(OBJDIR)/boot/boot.out $(OBJDIR)/boot/boot $(OBJDIR)/boot/boot.asm
	rm -f $(OBJDIR)/kernel/*.o $(OBJDIR)/kernel/system* kernel.*
	rm -f $(OBJDIR)/lib/*.o
