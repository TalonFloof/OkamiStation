FSTOOL := ../okamisdk/fennecfstool

build:
	$(FSTOOL) fennec.hd 4 newimage 20480
	cd bootloader/okami1041; lua MakeBootloader.lua; cd ../..
	$(FSTOOL) fennec.hd 4 bootldr bootloader/okami1041/Bootloader.bin
	../okamisdk/okameronc -arch=okami1041 -startup=kernel/HAL/okami/startup.s $(wildcard kernel/*.okm) $(wildcard kernel/IO/*.okm) $(wildcard kernel/HAL/*.okm) $(wildcard kernel/HAL/okami/*.okm) Kernel.o
	../okamisdk/okrotool reloc -noalign 0x80005000 0 Kernel.o
	../okamisdk/okrotool strip Kernel.o
	../okamisdk/okrotool dump Kernel.o Kernel.bin
	rm -f Kernel.o
	$(FSTOOL) fennec.hd 4 copy Kernel.bin /FENNECKernel
	rm -f bootloader/okami1041/Bootloader.bin bootloader/okami1041/Bootloader.o Kernel.bin

run: build
	../emulator/okamistation -scsihd fennec.hd

.PHONY: build run