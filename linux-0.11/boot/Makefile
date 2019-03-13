include ../Makefile.header

LDFLAGS	+= -Ttext 0

all: bootsect hello setup

bootsect: bootsect.s
	@$(AS) -o bootsect.o bootsect.s
	@$(LD) $(LDFLAGS) -o bootsect bootsect.o
	@objcopy -R .pdr -R .comment -R.note -S -O binary bootsect

hello: hello.s
	@$(AS) -o hello.o hello.s
	@$(LD) $(LDFLAGS) -o hello hello.o
	@objcopy -R .pdr -R .comment -R.note -S -O binary hello

setup: setup.s
	@$(AS) -o setup.o setup.s
	@$(LD) $(LDFLAGS) -o setup setup.o
	@objcopy -R .pdr -R .comment -R.note -S -O binary setup

head.o: head.s
	@$(AS) -o head.o head.s

clean:
	@rm -f bootsect bootsect.o hello hello.o setup setup.o head.o
