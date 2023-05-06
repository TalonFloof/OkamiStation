FSTOOL := ../okamisdk/fennecfstool

build:
	$(FSTOOL) fennec.hd 64 newimage 20480
	cd bootloader/okami1041; lua MakeBootloader.lua; cd ../..
	$(FSTOOL) fennec.hd 64 bootldr bootloader/okami1041/Bootloader.bin
	rm -f bootloader/okami1041/Bootloader.bin bootloader/okami1041/Bootloader.o

run: build
	../emulator/okamistation -scsihd fennec.hd

.PHONY: build run