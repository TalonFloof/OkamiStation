#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include "koribus.h"
#include "htc.h"

extern uint64_t cycle_count;
extern uint64_t iHitCount;
extern uint64_t iMissCount;
extern uint64_t dHitCount;
extern uint64_t dMissCount;
int shouldCacheStall = 0;
bool afterInc = false;

uint32_t registers[32];
uint32_t PC = 0xbff00000;

uint32_t extRegisters[0x17];

typedef struct {
    uint64_t isValid : 1;
    uint64_t nonCacheable : 1;
    uint64_t isDirty : 1;
    uint64_t isGlobal : 1;
    uint64_t reserved : 8;
    uint64_t paddr : 20;
    uint64_t addrSpaceID : 12;
    uint64_t vaddr : 20;
} TLBLine;

TLBLine TLB[64];

typedef struct {
    uint64_t cacheWord : 32;
    uint64_t cacheAddr : 30;
    uint64_t unused : 1;
    uint64_t isValid : 1;
} CacheLine;

CacheLine iCacheTags[2048];
CacheLine dCacheTags[2048];

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

int32_t getExtRegister(int index) {
    return extRegisters[index];
}

void setExtRegister(int index, uint32_t val) {
    if(index != 0x13) {
        extRegisters[index] = val;
    }
}

uint32_t readICacheLine(uint32_t addr);

void triggerTrap(uint32_t type, uint32_t addr) {
    switch(type) {
        case 2: { // MCall/KCall
            extRegisters[0] = (((extRegisters[0] & 3) << 2) | 1) | (extRegisters[0] & 0xFFFFFFF0);
            extRegisters[1] = type;
            extRegisters[2] = PC;
            extRegisters[3] = addr & 0x1FFFFFF;
            if(addr & 0x2000000) {
                // This is a MCALL
                PC = extRegisters[7];
            } else {
                // This is a KCALL
                PC = extRegisters[5];
            }
            break;
        }
        default: {
            if(type == 3) {
                extRegisters[0x15] = (extRegisters[0x15] & 0xffe00000) | ((extRegisters[3] >> 10) & 0x1FFFFC);
                extRegisters[0x12] = (addr & 0xFFFFF000) | extRegisters[0x14];
            }
            if(type == 3 && extRegisters[1] != 3) { // Non-nested TLB Miss
                extRegisters[0] = (((extRegisters[0] & 3) << 2) | 1) | (extRegisters[0] & 0xFFFFFFF0);
                extRegisters[1] = type;
                extRegisters[2] = afterInc ? PC-4 : PC;
                extRegisters[3] = addr;
                PC = extRegisters[6];
                break;
            }
            if(extRegisters[5] == 0) {
                fprintf(stderr, "UNCAUGHT TRAP 0x%x TRIGGERED: 0x%08x - ACCESSED: 0x%08x\n", type, afterInc ? PC-4 : PC, addr);
                for(int i=0; i < 32; i++) {
                    fprintf(stderr, "r%i: 0x%08x ", i, getRegister(i));
                }
                exit(1);
            } else {
                extRegisters[0] = (((extRegisters[0] & 3) << 2) | 1) | (extRegisters[0] & 0xFFFFFFF0);
                extRegisters[1] = type;
                extRegisters[2] = afterInc ? PC-4 : PC;
                extRegisters[3] = addr;
                PC = extRegisters[5];
            }
            break;
        }
    }
}

int TLBLookup(uint32_t addr) {
    int i;
    uint32_t vaddr = addr & 0xFFFFF000;
    for(i=0; i < 64; i++) {
        if(TLB[i].isValid && (TLB[i].vaddr == (vaddr >> 12)) && ((TLB[i].addrSpaceID == extRegisters[0x14]) || TLB[i].isGlobal)) {
            return i;
        }
    }
    return -1; /* TLB Miss */
}

uint32_t readICacheLine(uint32_t addr) {
    int index = (addr >> 2) & 0x7FF;
    if(iCacheTags[index].isValid && (iCacheTags[index].cacheAddr << 2) == (addr & 0x1FFFFFFC)) {
        iHitCount += 1;
        return iCacheTags[index].cacheWord;
    } else {
        iMissCount += 1;
        stallTicks = shouldCacheStall*4; // Cache Miss Stall
        if(!KoriBusRead(addr & 0x1FFFFFFC,4,((uint8_t*)&iCacheTags)+(index*8))) {
            triggerTrap(8,addr); // Fetch Exception
            return 0;
        }
        iCacheTags[index].cacheAddr = ((addr & 0x1FFFFFFC) >> 2);
        iCacheTags[index].isValid = 1;
        return iCacheTags[index].cacheWord;
    }
}

bool readDCacheLine(uint32_t addr, uint8_t* buf, uint32_t size) {
    int index = (addr >> 2) & 0x7FF;
    if(!(dCacheTags[index].isValid && (dCacheTags[index].cacheAddr << 2) == (addr & 0x1FFFFFFC))) {
        dMissCount += 1;
        stallTicks = shouldCacheStall*4; // Cache Miss Stall
        if(!KoriBusRead(addr & 0x1FFFFFFC,4,((uint8_t*)&dCacheTags)+(index*8))) {
            triggerTrap(9,addr); // Data Exception
            return false;
        }
        dCacheTags[index].cacheAddr = ((addr & 0x1FFFFFFC) >> 2);
        dCacheTags[index].isValid = 1;
    } else {
        dHitCount += 1;
    }
    memcpy(buf,((uint8_t*)&dCacheTags)+(index*8)+(addr & 0x3),size);
    return true;
}

bool writeDCacheLine(uint32_t addr, uint8_t* value, uint32_t size) {
    int index = (addr >> 2) & 0x7FF;
    if(!KoriBusWrite(addr & 0x1FFFFFFF,size,value)) {
        triggerTrap(9,addr); // Data Exception
        return false;
    }
    if(!(dCacheTags[index].isValid && (dCacheTags[index].cacheAddr << 2) == (addr & 0x1FFFFFFC))) {
        dMissCount += 1;
        stallTicks = shouldCacheStall*4; // Cache Miss Stall
        if(!KoriBusRead(addr & 0x1FFFFFFC,4,((uint8_t*)&dCacheTags)+(index*8))) {
            triggerTrap(9,addr); // Data Exception
            return false;
        }
        dCacheTags[index].cacheAddr = ((addr & 0x1FFFFFFC) >> 2);
        dCacheTags[index].isValid = 1;
    } else {
        dHitCount += 1;
    }
    memcpy(((uint8_t*)&dCacheTags)+(index*8)+(addr & 3),value,size);
    return true;
}

bool memAccess(uint32_t addr, uint8_t* buf, uint32_t len, bool write, bool fetch) {
    if((addr & (len - 1)) != 0) {
        if(write) {
            triggerTrap(7,addr); // Unaligned Write
            return false;
        } else {
            triggerTrap(6,addr); // Unaligned Read
            return false;
        }
    }
    if(((extRegisters[0] & 1) == 0) && addr >= 0x80000000) {
        triggerTrap(11,addr); // Permission Exception
        return false;
    }
    if(addr < 0x80000000 || (addr >= 0xc0000000 && addr <= 0xffffffff)) { // user segment / kernel3 segment
        int tlbEntry = TLBLookup(addr);
        if(tlbEntry == -1) {
            triggerTrap(3,addr); // TLB Miss
            return false;
        } else if(!TLB[tlbEntry].isDirty && write) {
            triggerTrap(4,addr); // TLB Modify
            return false;
        }
        if(write) {
            return writeDCacheLine((TLB[tlbEntry].paddr << 12) | (addr & 0xFFF),buf,len);
        } else if(fetch) {
            uint32_t val = readICacheLine(addr-0x80000000);
            memcpy(buf,(uint8_t*)&val,len);
            return true;
        } else {
            return readDCacheLine((TLB[tlbEntry].paddr << 12) | (addr & 0xFFF),buf,len);
        }
    } else if(addr >= 0x80000000 && addr <= 0x9fffffff) { // kernel1 segment
        if(fetch) {
            if(extRegisters[0] & 0x40) {
                triggerTrap(8,addr); // Fetch Exception
                return false;
            }
            uint32_t val = readICacheLine(addr-0x80000000);
            memcpy(buf,(uint8_t*)&val,len);
            return true;
        } else {
            if((extRegisters[0] & 0x40) && addr <= 0x90000000) {
                if(extRegisters[0] & 0x80) {
                    if(write) {
                        memcpy(((uint8_t*)&iCacheTags)+(addr-0x80000000),buf,len);
                        return true;
                    } else {
                        memcpy(buf,((uint8_t*)&iCacheTags)+(addr-0x80000000),len);
                        return true;
                    }
                } else {
                    if(write) {
                        memcpy(((uint8_t*)&dCacheTags)+(addr-0x80000000),buf,len);
                        return true;
                    } else {
                        memcpy(buf,((uint8_t*)&dCacheTags)+(addr-0x80000000),len);
                        return true;
                    }
                }
            } else {
                if(write) {
                    return writeDCacheLine(addr,buf,len);
                } else {
                    return readDCacheLine(addr,buf,len);
                }
            }
        }
    } else if(addr >= 0xa0000000 && addr <= 0xbfffffff) { // kernel2 segment
        if(fetch) {
            iMissCount += 1;
        } else {
            dMissCount += 1;
        }
        stallTicks = shouldCacheStall?3:3; // Uncached Stall
        if(write) {
            bool result = KoriBusWrite(addr-0xa0000000,len,buf);
            if(!result) {
                triggerTrap(9,addr); // Data Exception
            }
            return result;
        } else {
            bool result = KoriBusRead(addr-0xa0000000,len,buf);
            if(!result) {
                if(fetch) {
                    triggerTrap(8,addr); // Fetch Exception
                } else {
                    triggerTrap(9,addr); // Data Exception
                }
            }
            return result;
        }
    }
    return 0;
}

void reset() {
    PC = 0xbff00000;
    memset((void*)registers,0,sizeof(registers));
    memset((void*)extRegisters,0,sizeof(extRegisters));
    extRegisters[0] = 1;
    extRegisters[0x13] = 8;
    for(int i=0; i < 2048; i++) {
        ((uint64_t*)&iCacheTags)[i] = ((uint64_t)random()) | ((uint64_t)random() << 32);
        ((uint64_t*)&dCacheTags)[i] = ((uint64_t)random()) | ((uint64_t)random() << 32);
        if(i < 64) {
            ((uint64_t*)&TLB)[i] = ((uint64_t)random()) | ((uint64_t)random() << 32);
        }
    }
}

void next() {
    if(stallTicks > 0) {
        stallTicks--;
        return;
    }
    if(HTCPending && (extRegisters[0] & 2)) {
        triggerTrap(1,0);
        HTCPending = false;
    }
    cycle_count += 1;
    extRegisters[0x13] = (((extRegisters[0x13] - 8) + 1) % 56) + 8;
    uint32_t instr = 0;
    if(!memAccess(PC,(uint8_t*)&instr,4,false,true)) {
        return;
    }
    //fprintf(stderr, "%08x: %08x\n", PC, instr);
    PC += 4;
    afterInc = true;
    uint32_t opcode = (instr & 0xFC000000) >> 26;
    switch((opcode & 0b110000) >> 4) {
        case 0: { // Arithmetic
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
                        setRegister(rd,((int32_t)getRegister(rs1))>>(getRegister(rs2)));
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
                    if(getRegister(rs2) == 0) {
                        triggerTrap(10,0);
                        break;
                    }
                    if(instr & 0x8) {
                        setRegister(lowRd, getRegister(rs1)/getRegister(rs2));
                        setRegister(rd, getRegister(rs1)%getRegister(rs2));
                    } else {
                        setRegister(lowRd, (uint32_t)((int32_t)getRegister(rs1)/(int32_t)getRegister(rs2)));
                        setRegister(rd, (uint32_t)((int32_t)getRegister(rs1)%(int32_t)getRegister(rs2)));
                    }
                    break;
                }
                default: {
                    triggerTrap(5,0);
                    break;
                }
            }
            break;
        }
        case 1: { // Arithmetic w/ Immediate
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
                    setRegister(rd,getRegister(rs)&constU);
                    break;
                }
                case 2: { // ORI
                    setRegister(rd,getRegister(rs)|constU);
                    break;
                }
                case 3: { // XORI
                    setRegister(rd,getRegister(rs)^constU);
                    break;
                }
                case 4: { // SLLI
                    setRegister(rd,getRegister(rs)<<constU);
                    break;
                }
                case 5: { // SRLI/SRAI
                    if(instr & 0x400) {
                        setRegister(rd,(uint32_t)((int32_t)getRegister(rs)>>constU));
                    } else {
                        setRegister(rd,getRegister(rs)>>constU);
                    }
                    break;
                }
                case 6: { // SLTI
                    setRegister(rd,(((int32_t)getRegister(rs))<constS)?1:0);
                    break;
                }
                case 7: { // SLTIU
                    setRegister(rd,(getRegister(rs)<constU)?1:0);
                    break;
                }
                case 8: { // LUI
                    setRegister(rd,constU<<16);
                    break;
                }
                default: {
                    triggerTrap(5,0);
                    break;
                }
            }
            break;
        }
        case 2: { // Branching
            uint32_t rs1 = (instr & 0x1F0000) >> 16;
            uint32_t rs2 = (instr & 0x3E00000) >> 21;
            uint32_t rs = rs1;
            uint32_t rd = (instr & 0xf800) >> 11;
            int32_t jumpOffset = ((int32_t)((int16_t)(instr & 0xFFFF)))*4;
            int32_t jumpAddr = (((int32_t)((instr & 0x3FFFFFF) << 6)) >> 6) * 4;
            switch((opcode & 0b1111)) {
                case 0: { // B
                    PC += jumpAddr;
                    break;
                }
                case 1: { // BL
                    setRegister(31,PC);
                    PC += jumpAddr;
                    break;
                }
                case 2: { // BLR
                    setRegister(rs,PC);
                    PC = getRegister(rd);
                    break;
                }
                case 3: { // BEQ
                    if(getRegister(rs1)==getRegister(rs2)) {
                        PC += jumpOffset;
                    }
                    break;
                }
                case 4: { // BNE
                    if(getRegister(rs1)!=getRegister(rs2)) {
                        PC += jumpOffset;
                    }
                    break;
                }
                case 5: { // BGE
                    if(((int32_t)getRegister(rs1))>=((int32_t)getRegister(rs2))) {
                        PC += jumpOffset;
                    }
                    break;
                }
                case 6: { // BLT
                    if(((int32_t)getRegister(rs1))<((int32_t)getRegister(rs2))) {
                        PC += jumpOffset;
                    }
                    break;
                }
                case 7: { // BGEU
                    if(getRegister(rs1)>=getRegister(rs2)) {
                        PC += jumpOffset;
                    }
                    break;
                }
                case 8: { // BLTU
                    if(getRegister(rs1)<getRegister(rs2)) {
                        PC += jumpOffset;
                    }
                    break;
                }
                default: {
                    triggerTrap(5,0);
                    break;
                }
            }
            break;
        }
        case 3: { // Memory/TLB/Extended Registers/Traps
            if(opcode & 0b1000) {
                uint32_t operation = (instr & 0x7);
                uint32_t rs = (instr & 0x1F0000) >> 16;
                uint32_t rd = (instr & 0x3E00000) >> 21;
                uint32_t offset = (instr & 0xFFFF);
                switch(opcode & 0b111) {
                    case 0b011: { // TLBRI/TLBWI/TLBWR/TLBP
                        if(!(extRegisters[0] & 1)) {
                            triggerTrap(11,0);
                        }
                        switch(operation) {
                            case 0b000: { // TLBRI
                                extRegisters[0x11] = (((uint32_t*)&TLB)[extRegisters[0x10]*2]);
                                extRegisters[0x12] = (((uint32_t*)&TLB)[(extRegisters[0x10]*2)+1]);
                                break;
                            }
                            case 0b010: { // TLBWI
                                (((uint32_t*)&TLB)[extRegisters[0x10]*2]) = extRegisters[0x11];
                                (((uint32_t*)&TLB)[(extRegisters[0x10]*2)+1]) = extRegisters[0x12];
                                break;
                            }
                            case 0b011: { // TLBWR
                                uint32_t rand = extRegisters[0x13];
                                (((uint32_t*)&TLB)[rand*2]) = extRegisters[0x11];
                                (((uint32_t*)&TLB)[(rand*2)+1]) = extRegisters[0x12];
                                break;
                            }
                            case 0b100: { // TLBP
                                for(int i=0; i < 64; i++) {
                                    if((((uint32_t*)&TLB)[(i*2)+1]) == extRegisters[0x12]) {
                                        extRegisters[0x10] = i;
                                        goto after;
                                    }
                                }
                                extRegisters[0x10] = 0x80000000;
after:
                                break;
                            }
                        }
                        break;
                    }
                    case 0b100: { // MFEX
                        if(!(extRegisters[0] & 1)) {
                            triggerTrap(11,0);
                        }
                        setRegister(rd,getExtRegister(offset));
                        break;
                    }
                    case 0b101: { // MTEX
                    if(!(extRegisters[0] & 1)) {
                            triggerTrap(11,0);
                        }
                        setExtRegister(offset,getRegister(rs));
                        break;
                    }
                    case 0b110: { // KCALL/MCALL
                        triggerTrap(2,instr & 0x3FFFFFF);
                        break;
                    }
                    case 0b111: { // RFT
                        if(!(extRegisters[0] & 1)) {
                            triggerTrap(11,0);
                        }
                        extRegisters[0] = ((extRegisters[0] & 0x3f) >> 2) | (extRegisters[0] & ~(0x3f));
                        PC = extRegisters[2];
                        break;
                    }
                }
            } else {
                int32_t offset = (int32_t)((int16_t)(instr & 0xFFFF));
                uint32_t rs = (instr & 0x1F0000) >> 16;
                uint32_t rd = (instr & 0x3E00000) >> 21;
                uint32_t addr = getRegister(rs)+offset;
                uint32_t val = 0;
                switch(opcode & 0b111) {
                    case 0: { // LB
                        if(memAccess(addr,(uint8_t*)&val,1,false,false)) {
                            setRegister(rd,(int32_t)((int8_t)val));
                        }
                        break;
                    }
                    case 1: { // LBU
                        if(memAccess(addr,(uint8_t*)&val,1,false,false)) {
                            setRegister(rd,val);
                        }
                        break;
                    }
                    case 2: { // LH
                        if(memAccess(addr,(uint8_t*)&val,2,false,false)) {
                            setRegister(rd,(int32_t)((int16_t)val));
                        }
                        break;
                    }
                    case 3: { // LHU
                        if(memAccess(addr,(uint8_t*)&val,2,false,false)) {
                            setRegister(rd,val);
                        }
                        break;
                    }
                    case 4: { // LW
                        if(memAccess(addr,(uint8_t*)&val,4,false,false)) {
                            setRegister(rd,val);
                        }
                        break;
                    }
                    case 5: { // SB
                        val = getRegister(rd);
                        memAccess(addr,(uint8_t*)&val,1,true,false);
                        break;
                    }
                    case 6: { // SH
                        val = getRegister(rd);
                        memAccess(addr,(uint8_t*)&val,2,true,false);
                        break;
                    }
                    case 7: { // SW
                        val = getRegister(rd);
                        memAccess(addr,(uint8_t*)&val,4,true,false);
                        break;
                    }
                }
            }
            break;
        }
    }
    afterInc = false;
}