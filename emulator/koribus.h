#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

typedef int (*KoriBusDevWrite)(uint32_t addr, uint32_t len, void *buf);
typedef int (*KoriBusDevRead)(uint32_t addr, uint32_t len, void *buf);

typedef struct {
    bool Used;
    KoriBusDevRead Read;
    KoriBusDevRead Write;
} KoriBusBank;

extern KoriBusBank KoriBusBanks[16];

static inline int KoriBusRead(uint32_t addr, uint32_t len, uint8_t* buf) {
    int bank = addr >> 25;
    if(KoriBusBanks[bank].Used) {
        return KoriBusBanks[bank].Read(addr & 0x1FFFFFF, len, (void*)buf);
    }
    return 0;
}


static inline int KoriBusWrite(uint32_t addr, uint32_t len, uint8_t* buf) {
    int bank = addr >> 25;
    if(KoriBusBanks[bank].Used) {
        return KoriBusBanks[bank].Write(addr & 0x1FFFFFF, len, (void*)buf);
    }
    return 0;
}

bool KoriBusInit();