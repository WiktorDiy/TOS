A = nasm

all:
	$A src/16bit/boot_s.asm -f bin -o build/MBR.bin -Werror
	$A src/16bit/bootloader_16bit.asm -f bin -o build/BOOTLOADER.bin -Werror
	$A src/32bit/bootloader_32bit.asm -f bin -o build/BOOTLOADER_32.bin -Werror

	./src/s_disp.sh build/MBR.bin
	./src/s_disp.sh build/BOOTLOADER.bin
	./src/s_disp.sh build/BOOTLOADER_32.bin
	#cp build/MBR.bin build/img.img
	$(MAKE) -C src/img
	qemu-system-x86_64 \
    -m 2G \
    -cpu host \
    -enable-kvm \
    -monitor stdio \
    -device ahci,id=ahci0 \
    -drive id=disk0,file=build/disk.img,format=raw,if=none \
    -device ide-hd,drive=disk0,bus=ahci0.2
	
