#include "koribus.h"
#include <string.h>

uint8_t RAM[4*1024*1024];

int RAMRead(uint32_t addr, uint32_t len, void *buf) {
    if(addr+len >= 4*1024*1024)
        return 1;
    memcpy((uint8_t*)buf, ((uint8_t*)&RAM)+addr, len);
    return 0;
}

int RAMWrite(uint32_t addr, uint32_t len, void *buf) {
    if(addr+len >= 4*1024*1024)
        return 1;
    memcpy(((uint8_t*)&RAM)+addr, (uint8_t*)buf, len);
    return 0;
}

void RAMInit() {
    KoriBusBanks[0].Used = true;
    KoriBusBanks[0].Write = RAMWrite;
	KoriBusBanks[0].Read = RAMRead;
}