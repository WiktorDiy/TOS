LOC = $(HOME)/os/rev1/build

DISK = disk.img

disk:
	dd if=/dev/zero of=$(LOC)/$(DISK) bs=1M count=4096                   #CREATE A 4GB blank
	
	dd if=$(LOC)/MBR.bin of=$(LOC)/$(DISK) bs=512 count=1 conv=notrunc	 #LOAD THE MBR



	parted -s --align=none $(LOC)/$(DISK) unit s mkpart primary 1 63
	
	dd if=$(LOC)/BOOTLOADER.bin of=$(LOC)/$(DISK) bs=512 seek=1 count=3 conv=notrunc
	dd if=$(LOC)/BOOTLOADER_32.bin of=$(LOC)/$(DISK) bs=512 seek=4 count=20 conv=notrunc
	
	parted -s --align=none $(LOC)/$(DISK) unit s mkpart primary fat32 64 2097215
