build:
	/opt/riscv-c/bin/riscv64-unknown-elf-gcc -Wall -Wextra -march=rv32ima_zicsr -mabi=ilp32 src/*.c src/*.S -T link.ld -nostdlib -Wl,--gc-sections -o EmberWolfFirmware.elf
	/opt/riscv-c/bin/riscv64-unknown-elf-objcopy EmberWolfFirmware.elf -O binary ../resources/EmberWolfFirmware
	# rm EmberWolfFirmware.elf