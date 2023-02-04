#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include "koribus.h"

uint32_t registers[32];
uint32_t PC = 0xbff00000;

uint32_t extRegisters[0x15];

typedef struct {
    uint64_t isValid : 1;
    uint64_t nonCacheable : 1;
    uint64_t isDirty : 1;
    uint64_t size : 5;
    uint64_t vaddr : 24;
    uint64_t addrSpaceID : 8;
    uint64_t paddr : 24;
} TLBLine;

TLBLine TLB[64];

typedef struct {
    uint64_t cacheWord : 32;
    uint64_t cacheAddr : 28;
    uint64_t isValid : 1;
    uint64_t cacheParity : 3;
} CacheLine;

CacheLine iCacheTags[4096];
CacheLine dCacheTags[4096];

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

uint32_t getExtRegister(int index) {
    switch(index) {
        case 0x00:
        case 0x01:
        case 0x02:
        case 0x03:
        case 0x04:
        case 0x05:
        case 0x10:
        case 0x14: {
            return extRegisters[index];
        }
        case 0x11: {
            return ((uint64_t*)&TLB)[index] & 0xFFFFFFFF;
        }
        case 0x12: {
            return ((uint64_t*)&TLB)[index] >> 32;
        }
        case 0x13: {
            return (random()%56)+8; // Doesn't comply with my documentation but whatever, it's for debugging purposes anyway.
        }
    }
    return 0;
}

void setExtRegister(int index, uint32_t val) {
    switch(index) {
        case 0x00:
        case 0x01:
        case 0x02:
        case 0x03:
        case 0x04:
        case 0x05:
        case 0x10:
        case 0x14: {
            extRegisters[index] = val;
            break;
        }
        case 0x11: {
            ((uint32_t*)&TLB)[index*2] = val;
            break;
        }
        case 0x12: {
            ((uint32_t*)&TLB)[(index*2)+1] = val;
            break;
        }
    }
}

void triggerTrap(uint32_t type, uint32_t addr) {
    fprintf(stderr, "TRAP 0x%x TRIGGERED: 0x%08x\n", type, PC-4);
}

int TLBLookup(uint32_t addr) {
    int i;
    uint32_t vaddr = addr & 0xFFFFFF00;
    for(i=0; i < 64; i++) {
        if(TLB[i].isValid && (TLB[i].addrSpaceID == extRegisters[0x14] || (TLB[i].addrSpaceID & 0x80))) {
            /* Check if we're within the size boundary */
            uint32_t size = 1 << TLB[i].size;
            if(size < 256)
                continue;
            if(vaddr >= (TLB[i].vaddr << 8) && vaddr <= (TLB[i].vaddr << 8)+size) {
                return i;
            }
            continue;
        }
    }
    return -1; /* TLB Miss */
}

uint64_t calculateParity(uint64_t line) {
    uint64_t result = 0;
    int i;
    for(i=0; i < 20; i++) {
        uint64_t digit = (line & (0x7 << (i*3))) >> (i*3);
        result ^= digit;
    }
    return result;
}

uint32_t readICacheLine(uint32_t addr) {
    int index = (addr >> 2) & 0xFFF;
    uint64_t parity = calculateParity(((uint64_t*)&iCacheTags)[index]);
    if(iCacheTags[index].isValid && (iCacheTags[index].cacheAddr << 2) == (addr & 0x3FFFFFFC) && iCacheTags[index].cacheParity == parity) {
        return iCacheTags[index].cacheWord;
    } else {
        stallTicks = 4; // Cache Miss Stall
        if(!KoriBusRead(addr & 0x3FFFFFFC,4,((uint8_t*)&iCacheTags)+(index*8))) {
            triggerTrap(8,addr); // Fetch Exception
            return 0;
        }
        iCacheTags[index].cacheAddr = ((addr & 0x3FFFFFFC) >> 2);
        iCacheTags[index].isValid = 1;
        iCacheTags[index].cacheParity = calculateParity(((uint64_t*)&iCacheTags)[index]);
        return iCacheTags[index].cacheWord;
    }
}

uint32_t readDCacheLine(uint32_t addr, uint32_t size) {
    int index = (addr >> 2) & 0xFFF;
    uint64_t parity = calculateParity(((uint64_t*)&dCacheTags)[index]);
    uint32_t val;
    if(dCacheTags[index].isValid && (dCacheTags[index].cacheAddr << 2) == (addr & 0x3FFFFFFC) && dCacheTags[index].cacheParity == parity) {
        val = dCacheTags[index].cacheWord;
    } else {
        stallTicks = 4; // Cache Miss Stall
        if(!KoriBusRead(addr & 0x3FFFFFFC,4,((uint8_t*)&iCacheTags)+(index*8))) {
            triggerTrap(9,addr); // Data Exception
            return 0;
        }
        dCacheTags[index].cacheAddr = ((addr & 0x3FFFFFFC) >> 2);
        dCacheTags[index].isValid = 1;
        dCacheTags[index].cacheParity = calculateParity(((uint64_t*)&dCacheTags)[index]);
        val = dCacheTags[index].cacheWord;
    }
    if(size == 1) {
        val >>= (addr & 3)*8;
    } else if(size == 2) {
        val >>= (addr & 2)*16;
    }
    return val;
}

void writeDCacheLine(uint32_t addr, uint32_t value, uint32_t size) {
    int index = (addr >> 2) & 0xFFF;
    uint64_t parity = calculateParity(((uint64_t*)&dCacheTags)[index]);
    if(!KoriBusWrite(addr & 0x3FFFFFFC,4,(uint8_t*)&value)) {
        triggerTrap(9,addr); // Data Exception
        return;
    }
    if(dCacheTags[index].isValid && (dCacheTags[index].cacheAddr << 2) == (addr & 0x3FFFFFFC) && dCacheTags[index].cacheParity == parity) {
        if(size == 1) {
            uint32_t shift = (addr & 3)*8;
            dCacheTags[index].cacheWord = (dCacheTags[index].cacheWord & ~(0xFF << shift)) | (value << shift);
        } else if(size == 2) {
            uint32_t shift = (addr & 2)*16;
            dCacheTags[index].cacheWord = (dCacheTags[index].cacheWord & ~(0xFFFF << shift)) | (value << shift);
        } else {
            dCacheTags[index].cacheWord = value;
        }
        dCacheTags[index].cacheParity = calculateParity(((uint64_t*)&dCacheTags)[index]);
    } else {
        stallTicks = 4; // Cache Miss Stall
        if(!KoriBusRead(addr & 0x3FFFFFFC,4,((uint8_t*)&iCacheTags)+(index*8))) {
            triggerTrap(9,addr); // Data Exception
            return;
        }
        if(size == 1) {
            uint32_t shift = (addr & 3)*8;
            dCacheTags[index].cacheWord = (dCacheTags[index].cacheWord & ~(0xFF << shift)) | (value << shift);
        } else if(size == 2) {
            uint32_t shift = (addr & 2)*16;
            dCacheTags[index].cacheWord = (dCacheTags[index].cacheWord & ~(0xFFFF << shift)) | (value << shift);
        } else {
            dCacheTags[index].cacheWord = value;
        }
        dCacheTags[index].cacheAddr = ((addr & 0x3FFFFFFC) >> 2);
        dCacheTags[index].isValid = 1;
        dCacheTags[index].cacheParity = calculateParity(((uint64_t*)&dCacheTags)[index]);
    }
}

bool memAccess(uint32_t addr, uint8_t* buf, uint32_t len, bool write, bool fetch) {
    uint32_t kaddr = addr & 0x3FFFFFFF;
    if(addr < 0x80000000) { // user segment
        int tlbEntry = TLBLookup(addr);
        if(tlbEntry == -1) {
            triggerTrap(3,addr); // TLB Miss
            return false;
        }
    } else if(addr >= 0x80000000 && addr <= 0x9fffffff) { // kernel1 segment
        if(fetch) {
            if(extRegisters[0] & 0x8) {
                triggerTrap(8,addr); // Fetch Exception
                return false;
            }
            uint32_t val = readICacheLine(addr-0x80000000);
            memcpy(buf,(uint8_t*)&val,len);
            return true;
        } else {
            if(extRegisters[0] & 0x8) {
                if(extRegisters[0] & 0x10) {
                    if(write) {
                        ((uint32_t*)&iCacheTags)[(addr-0x80000000) >> 2] = *((uint32_t*)buf);
                    } else {
                        uint32_t val = ((uint32_t*)&iCacheTags)[(addr-0x80000000) >> 2];
                        memcpy(buf,(uint8_t*)&val,len);
                    }
                } else {
                    if(write) {
                        ((uint32_t*)&dCacheTags)[(addr-0x80000000) >> 2] = *((uint32_t*)buf);
                    } else {
                        uint32_t val = ((uint32_t*)&dCacheTags)[(addr-0x80000000) >> 2];
                        memcpy(buf,(uint8_t*)&val,len);
                    }
                }
            } else {
                if(write) {
                    writeDCacheLine(addr-0x80000000,*((uint32_t*)buf),len);
                    return true;
                } else {
                    uint32_t val = readDCacheLine(addr-0x80000000,len);
                    memcpy(buf,(uint8_t*)&val,len);
                    return true;
                }
            }
        }
    } else if(addr >= 0xa0000000 && addr <= 0xbfffffff) { // kernel2 segment
        stallTicks = 3; // Uncached Stall
        if(write) {
            bool result = KoriBusWrite(addr-0xa0000000,len,buf);
            if(!result) {
                triggerTrap(7,addr); // Data Exception
            }
            return result;
        } else {
            bool result = KoriBusRead(addr-0xa0000000,len,buf);
            if(!result) {
                if(fetch) {
                    triggerTrap(8,addr); // Fetch Exception
                } else {
                    triggerTrap(7,addr); // Data Exception
                }
            }
            return result;
        }
    } else if(addr >= 0xc0000000 && addr <= 0xffffffff) { // kernel3 segment
        int tlbEntry = TLBLookup(addr);
        if(tlbEntry == -1) {
            triggerTrap(3,addr); // TLB Miss
            return false;
        }
    }
}

void reset() {
    PC = 0xbff00000;
    memset((void*)registers,0,sizeof(registers));
    memset((void*)extRegisters,0,sizeof(extRegisters));
    extRegisters[0] = 1;
    for(int i=0; i < 4096; i++) {
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
    uint32_t instr = 0;
    if(!memAccess(PC,(uint8_t*)&instr,4,false,true)) {
        return;
    }
    fprintf(stderr, "%08x: %08x\n", PC, instr);
    PC += 4;
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
                    if(instr & 0x8) {
                        setRegister(lowRd, getRegister(rs1)/getRegister(rs2));
                    } else {
                        setRegister(lowRd, getRegister(rs1)/getRegister(rs2));
                        setRegister(rd, getRegister(rs1)%getRegister(rs2));
                    }
                    break;
                }
                default: {
                    triggerTrap(4,0);
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
                    setRegister(rd,getRegister(rs)&constS);
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
                    triggerTrap(4,0);
                    break;
                }
            }
            break;
        }
        case 2: { // Branching
            uint32_t rs1 = (instr & 0x1F0000) >> 16;
            uint32_t rs2 = (instr & 0x3E00000) >> 21;
            uint32_t rs = rs1;
            uint32_t rd = rs2;
            int32_t jumpOffset = ((int32_t)((int16_t)(instr & 0xFFFF)))*4;
            uint32_t jumpAddr = (jumpAddr & 0x3FFFFFF) << 2;
            switch((opcode & 0b1111)) {
                case 0: { // B
                    PC = (PC & 0xF0000000) | jumpAddr;
                    break;
                }
                case 1: { // BL
                    setRegister(31,PC);
                    PC = (PC & 0xF0000000) | jumpAddr;
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
                    triggerTrap(4,0);
                    break;
                }
            }
            break;
        }
        case 3: { // Memory/Extended Registers/Traps
            if(opcode & 0b1000) {
                uint32_t rs = (instr & 0x1F0000) >> 16;
                uint32_t rd = (instr & 0x3E00000) >> 21;
                uint32_t offset = (instr & 0xFFFF);
                switch(opcode & 0b111) {
                    case 0b1100: { // MFEX
                        setRegister(rd,getExtRegister(offset));
                        break;
                    }
                    case 0b1101: { // MTEX
                        setExtRegister(offset,getRegister(rs));
                        break;
                    }
                    case 0b1110: { // KCALL
                        triggerTrap(2,0);
                        break;
                    }
                    case 0b1111: { // RFT
                        extRegisters[0] = ((extRegisters[0] & 3) >> 1) | (extRegisters[0] & ~3);
                        PC = extRegisters[2];
                        break;
                    }
                }
            } else {
                int32_t offset = (int32_t)((int16_t)(instr & 0xFFFF));
                uint32_t rs = (instr & 0x1F0000) >> 16;
                uint32_t rd = (instr & 0x3E00000) >> 21;
                uint32_t addr = getRegister(rs)+offset;
                uint32_t val;
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
}