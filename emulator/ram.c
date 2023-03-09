#include "koribus.h"
#include <stdlib.h>
#include <string.h>

int RAMSize = 4*1024*1024;

uint8_t* RAM;

int RAMRead(uint32_t addr, uint32_t len, void *buf) {
    if(addr+len > RAMSize)
        return 0;
    memcpy((uint8_t*)buf, (RAM)+addr, len);
    return 1;
}

int RAMWrite(uint32_t addr, uint32_t len, void *buf) {
    if(addr+len > RAMSize)
        return 0;
    memcpy((RAM)+addr, (uint8_t*)buf, len);
    return 1;
}

void RAMInit() {
    RAM = malloc(RAMSize);
    memset(RAM,0,RAMSize);
    KoriBusBanks[0].Used = true;
    KoriBusBanks[0].Write = RAMWrite;
	KoriBusBanks[0].Read = RAMRead;
}