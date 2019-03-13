all:
	@make -C linux-0.11
	@make -C seabios

clean:
	@make clean -C linux-0.11
	@make clean -C seabios

cm: clean all

qemu-studentID:
	@qemu-system-i386 -m 16M -boot a \
	-fda ./linux-0.11/Image -hda ./osdi.img

qemu-bootsplash:
	@qemu-system-i386 -m 16M \
	-boot order=a,menu=on,splash=./bootsplash2.jpg,splash-time=3500 \
	-fda ./linux-0.11/Image -hda ./osdi.img -bios ./seabios/out/bios.bin
