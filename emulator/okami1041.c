#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "koribus.h"

uint32_t registers[32];
uint32_t PC;

uint32_t extRegisters[0x14];

uint64_t TLB[64];

uint32_t iCacheTags[1024];
uint32_t dCacheTags[1024];
uint8_t iCacheLines[1024][16];
uint8_t dCacheLines[1024][4];

int stallTicks = 0;

uint32_t getRegister(int index) {
    if(index == 0) {
        return 0;
    } else {
        return registers[index];
    }
}

void setRegister(int index, uint32_t value) {
    if(index != 0) {
        registers[index] = value;
    }
}

void flushICacheLine(int index) {

}

void flushDCacheLine(int index) {
    
}

bool memAccess(uint32_t addr, uint8_t* buf, uint32_t len, bool write, bool fetch) {
    uint32_t kaddr = addr & 0x3FFFFFFF;
    if(addr < 0x80000000) { // user segment

    } else if(addr >= 0x80000000 && addr <= 0xbfffffff) { // kernel1 segment
        if(fetch) {
            int index = ((addr & 0x3FF0) >> 4);
            int offset = addr & 0xF;

            if((iCacheTags[index] & 1) && ((iCacheTags[index] & 0xFFFFFFF0) == (addr & 0xFFFFFFF0))) {
                if(write) {
                    memcpy((&iCacheLines[index][offset]),buf,len);
                } else {
                    memcpy(buf,(&iCacheLines[index][offset]),len);
                }
                return true;
            } else { // Cache Miss
                stallTicks = 4;
            }
        } else {
            int index = ((addr & 0xFFC) >> 2);
            int offset = addr & 0x3;
            if((dCacheTags[index] & 1) && ((dCacheTags[index] & 0xFFFFFFFC) == (addr & 0xFFFFFFFC))) {
                if(write) {
                    memcpy((&dCacheLines[index][offset]),buf,len);
                } else {
                    memcpy(buf,(&dCacheLines[index][offset]),len);
                }
                return true;
            } else { // Cache Miss
                stallTicks = 4;
                dCacheTags[index] = (addr & 0xFFFFFFFC) | 1;
                dCacheLines[index][0]
            }
        }
    } else if(addr >= 0xc0000000 && addr <= 0xffffffff) { // kernel2 segment
        stallTicks = 3; // Uncached Stall
        if(write) {
            return (bool)KoriBusWrite(addr,len,buf);
        } else {
            return (bool)KoriBusRead(addr,len,buf);
        }
    }
}

void next() {
    if(stallTicks > 0) {
        stallTicks--;
        return;
    }
    uint32_t instr = 0;
    if(!MemAccess(PC,&instr,4,false,true)) {
        return;
    }
    PC += 4;
    uint32_t opcode = (instr & 0xFC000000) >> 26;
    switch(opcode) {
        case 0: { // ADD

            break;
        }
    }
}