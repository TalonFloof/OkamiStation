#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "koribus.h"

uint32_t registers[32];
uint32_t PC = 0x;

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
                flushDCacheLine(index);
                KoriBusRead(&dCacheLines[index][0]);
            }
        }
    } else if(addr >= 0xc0000000 && addr <= 0xffffffff) { // kernel2 segment
        stallTicks = 3; // Uncached Stall
        if(write) {
            return (bool)KoriBusWrite(addr-0xc0000000,len,buf);
        } else {
            return (bool)KoriBusRead(addr-0xc0000000,len,buf);
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
    switch((opcode & 0b110000) >> 4) {
        case 0: {
            uint32_t rd = (instr & 0xf800) >> 11;
            uint32_t rs1 = (instr & 0x1F0000) >> 16;
            uint32_t rs2 = (instr & 0x3E00000) >> 21;
            switch((opcode & 0b1111)) {
                case 0: { // ADD
                    setRegister(rd,getRegister(rs1)+getRegister(rs2));
                    break;
                }
                case 1: { // SUB
                    setRegister(rd,getRegister(rs1)-getRegister(rs2));
                    break;
                }
                case 2: { // AND
                    setRegister(rd,getRegister(rs1)&getRegister(rs2));
                    break;
                }
                case 3: { // OR
                    setRegister(rd,getRegister(rs1)|getRegister(rs2));
                    break;
                }
                case 4: { // XOR
                    setRegister(rd,getRegister(rs1)^getRegister(rs2));
                    break;
                }
                case 5: { // SLL
                    setRegister(rd,getRegister(rs1)<<getRegister(rs2));
                    break;
                }
                case 6: { // SRL/SRA
                    if(instr & 0x400) {
                        setRegister(rd,((int32_t)getRegister(rs1))>>((int32_t)getRegister(rs2)));
                    } else {
                        setRegister(rd,getRegister(rs1)>>getRegister(rs2));
                    }
                    break;
                }
                case 7: { // SLT
                    setRegister(rd,(((int32_t)getRegister(rs1))<((int32_t)getRegister(rs2)))?1:0);
                    break;
                }
                case 8: { // SLTU
                    setRegister(rd,(getRegister(rs1)<getRegister(rs2))?1:0);
                    break;
                }
                case 9: { // MUL/MULU
                    uint32_t lowRd = (instr & 0x7C0) >> 6;
                    if(instr & 0x8) {
                        uint64_t result = ((uint64_t)getRegister(rs1))*((uint64_t)getRegister(rs2));
                        setRegister(lowRd, result & 0xFFFFFFFF);
                        setRegister(rd, result >> 32);
                    } else {
                        uint64_t result = (uint64_t)(((int64_t)getRegister(rs1))*((int64_t)getRegister(rs2)));
                        setRegister(lowRd, result & 0xFFFFFFFF);
                        setRegister(rd, result >> 32);
                    }
                    break;
                }
                case 10: { // DIV/DIVU
                    uint32_t lowRd = (instr & 0x7C0) >> 6;
                    if(instr & 0x8) {
                        setRegister(lowRd, getRegister(rs1)/getRegister(rs2));
                    } else {
                        setRegister(lowRd, getRegister(rs1)/getRegister(rs2));
                        setRegister(rd, getRegister(rs1)%getRegister(rs2));
                    }
                    break;
                }
                default: {
                    // TODO: Trigger Trap
                    break;
                }
            }
            break;
        }
        case 1: {
            int32_t constS = (int32_t)((int16_t)(instr & 0xFFFF));
            uint32_t constU = instr & 0xFFFF;
            uint32_t rd = (instr & 0x1F0000) >> 16;
            uint32_t rs = (instr & 0x3E00000) >> 21;
            switch((opcode & 0b1111)) {
                case 0: { // ADDI
                    setRegister(rd,getRegister(rs)+constS);
                    break;
                }
                case 1: { // ANDI
                    setRegister(rd,getRegister(rs)&constS);
                    break;
                }
                default: {
                    // TODO: Trigger Trap
                    break;
                }
            }
            break;
        }
    }
}