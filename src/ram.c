#include "icebus.h"
#include "ram.h"
#include "icewolf.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

uint8_t *RAMBanks[8];
uint32_t RAMBankCapacity[8];

int RAMRead(uint64_t addr, uint64_t len, void *buf) {
    int bank = addr >> 26;
    int offset = addr & ((64*1024*1024)-1);
    if (offset+len > RAMBankCapacity[bank])
		return -1;
    memcpy(buf, RAMBanks[bank]+offset, len);
    return 0;
}

int RAMWrite(uint64_t addr, uint64_t len, void *buf) {
    int bank = addr >> 26;
    int offset = addr & ((64*1024*1024)-1);
    if (offset+len > RAMBankCapacity[bank])
		return -1;
    memcpy(RAMBanks[bank]+offset, buf, len);
    return 0;
}

bool RAMInit() {
    if(MEMORY > 512*1024*1024) {
        fprintf(stderr, "\x1b[31m512 MiB Maximum Exceeded (Wanted %d MiB)\x1b[0m\n", MEMORY/1024/1024);
        return false;
    }
    IceBusBanks[0].Used = true;
    IceBusBanks[0].Write = RAMWrite;
	IceBusBanks[0].Read = RAMRead;
    int i = 0;
    int ram_size = MEMORY;
	while (ram_size >= 0x400000) {
		if (i >= 8)
			i = 0;

		RAMBankCapacity[i] += 0x400000;
		i += 1;
		ram_size -= 0x400000;
	}
    RAMBankCapacity[0] += ram_size;
    for (i = 0; i < 8; i++) {
		if (RAMBankCapacity[i])
            RAMBanks[i] = malloc(RAMBankCapacity[i]);
    }
    return true;
}