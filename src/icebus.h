#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef int (*IceBusDevWrite)(uint64_t addr, uint64_t len, void *buf);
typedef int (*IceBusDevRead)(uint64_t addr, uint64_t len, void *buf);

typedef struct IceBusBank {
    bool Used;
    IceBusDevRead Read;
    IceBusDevRead Write;
} IceBusBank_t;

extern IceBusBank_t IceBusBanks[16];

int IceBusRead(uint64_t addr, uint64_t len, uint8_t* buf);
int IceBusWrite(uint64_t addr, uint64_t len, uint8_t* buf);
bool IceBusInit();