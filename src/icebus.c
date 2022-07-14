#include "icebus.h"
#include "ram.h"
#include "rom.h"

IceBusBank_t IceBusBanks[16];

int IceBusRead(uint64_t addr, uint64_t len, uint8_t* buf) {
    int bank = addr >> 31;
    if(IceBusBanks[bank].Used) {
        return IceBusBanks[bank].Read(addr & 0x7FFFFFFF, len, (void*)buf);
    } else {
        return -1;
    }
}

int IceBusWrite(uint64_t addr, uint64_t len, uint8_t* buf) {
    int bank = addr >> 31;
    if(IceBusBanks[bank].Used) {
        return IceBusBanks[bank].Write(addr & 0x7FFFFFFF, len, (void*)buf);
    } else {
        return -1;
    }
}

bool IceBusInit() {
    if(!RAMInit())
        return false;
    if(!ROMInit())
        return false;
    return true;
}