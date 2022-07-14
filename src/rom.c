#include "icebus.h"
#include "rom.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define FIRMWARE_SIZE (128 * 1024)

uint8_t Firmware[FIRMWARE_SIZE];

int ROMRead(uint64_t addr, uint64_t len, void *buf) {
    if (addr+len > FIRMWARE_SIZE)
		return -1;
    memcpy(buf, &Firmware[addr], len);
    return 0;
}

int ROMWrite(uint64_t addr, uint64_t len, void *buf) {
    return 0;
}

bool ROMLoad(char *name) {
    FILE* rom = fopen(name, "r");
    if(!rom) {
        fprintf(stderr, "\x1b[31mUnable to open ROM file \"%s\"\x1b[0m\n", name);
        return false;
    }
    fread(&Firmware, FIRMWARE_SIZE, 1, rom);
    fclose(rom);
    return true;
}

bool ROMInit() {
    IceBusBanks[1].Used = true;
    IceBusBanks[1].Write = ROMRead;
	IceBusBanks[1].Read = ROMWrite;
    return ROMLoad("firmware.bin");
}