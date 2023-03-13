build:
	../okamisdk/vosc Main.vos Firmware.o
	../okamisdk/okrotool fwdump 0x9ff00000 0x80000000 Firmware.o ../emulator/Firmware.bin
	# rm -f a.out