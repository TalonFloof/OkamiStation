/*
WolfBox Fantasy Workstation
Copyright 2022-2022 Talon396

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "icewolf.h"
#include "icebus.h"
#include "motherboard.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

void IceWolf_Trap(IcewolfHart_t* hart, int typ) {
    if((hart->status & STATUS_MODE) == 0) { // User
        hart->UTrap.cause = typ;
        hart->status |= STATUS_UTRAP_GATE;
    } else if((hart->status & STATUS_MODE) == (1<<5)) { // Supervisor
        hart->STrap.cause = typ;
        hart->status |= STATUS_STRAP_GATE;
    } else if((hart->status & STATUS_MODE) == (2<<5)) { // Machine
        hart->MTrap.cause = typ;
        hart->status |= STATUS_MTRAP_GATE;
    }
    if(typ == TRAP_DATA || typ == TRAP_FETCH || typ == TRAP_DIVBYZERO || typ == TRAP_UNDEFINED) {
        fprintf(stderr, "Trap @ 0x%016lx, Cause: %i, Mode: %li\n", hart->Registers[0], typ, (hart->status & STATUS_MODE) >> 5);
        for(int i=0; i < 0x20; i++) {
            fprintf(stderr, "%02i: %016lx\n", i, hart->Registers[i]);
        }
    }
}

static inline bool IceWolf_MemAccess(IcewolfHart_t* hart, uint64_t addr, uint8_t* buf, uint64_t len, bool write, bool fetch) {
    if(addr & (len-1)) {
        IceWolf_Trap(hart,((int)fetch)+3);
        return false;
    }
    if(addr >= 0x80000000+FIRMWARE_SIZE) { // No Cache
        hart->StallTicks += BUS_STALL;
        int result;
        if(write)
			result = IceBusWrite(addr, len, buf);
		else
			result = IceBusRead(addr, len, buf);
        if(result != 0) {
            IceWolf_Trap(hart,((int)fetch)+3);
            return false;
        }
        return true;
    } else {
        uint64_t* tags = fetch ? (uint64_t*)((uintptr_t)hart+offsetof(IcewolfHart_t,ICacheTag)) : (uint64_t*)((uintptr_t)hart+offsetof(IcewolfHart_t,DCacheTag));
        uint8_t* cache = fetch ? (uint8_t*)((uintptr_t)hart+offsetof(IcewolfHart_t,ICache)) : (uint8_t*)((uintptr_t)hart+offsetof(IcewolfHart_t,DCache));
        uint64_t lineaddr = addr & ~(CACHELINESIZE-1);
		uint64_t lineoff = addr & (CACHELINESIZE-1);
		uint64_t lineno = addr>>4;
		uint64_t set = lineno&(CACHESETCOUNT-1);
		uint64_t insertat = -1;
        uint64_t line = -1;
        uint8_t *cacheline;
        for(int i = 0; i < CACHEWAYCOUNT; i++) {
			if((tags[set*CACHEWAYCOUNT+i] & ~0xF) == lineaddr && (tags[set*CACHEWAYCOUNT+i] & 1)) {
				line = set*CACHEWAYCOUNT+i;
				cacheline = &cache[line*CACHELINESIZE];
				break;
			}
			if(!(tags[set*CACHEWAYCOUNT+i] & 1))
				insertat = i;
		}
        if(line == -1) {
            hart->StallTicks += MISS_STALL;
            if(insertat == -1)
				line = set*CACHEWAYCOUNT+((fetch ? hart->ICache_Fill : hart->DCache_Fill)&(CACHEWAYCOUNT-1));
			else
				line = set*CACHEWAYCOUNT+insertat;
            cacheline = &cache[line*CACHELINESIZE];
            if(!fetch && ((tags[line] & 3) == 3)) { // Flush Data Cache Line
                hart->StallTicks += BUS_STALL;
                if(IceBusWrite(lineaddr, CACHELINESIZE, cacheline) != 0) {
                    IceWolf_Trap(hart,((int)fetch)+3);
                    return false;
                }
            }
            tags[line] = lineaddr | 1;
            if(IceBusRead(lineaddr, CACHELINESIZE, cacheline) != 0) {
                IceWolf_Trap(hart,((int)fetch)+3);
                return false;
            }
			if(fetch)
				hart->ICache_Fill++;
			else
				hart->DCache_Fill++;
        }
        if(write) {
            if((tags[line] & 2) == 0)
                hart->DCache_DirtyCount++;
            tags[line] = tags[line] | 2;
            memcpy(&cacheline[lineoff], buf, len);
            hart->DCache_DirtyCount += 1;
            if (hart->TicksUntilFlush == 0)
				hart->TicksUntilFlush = BUS_STALL;
        } else {
            memcpy(buf, &cacheline[lineoff], len);
        }
    }
    return true;
}

static inline uint64_t IceWolf_ReadExReg(IcewolfHart_t* hart, int index) {
    switch(index) {
        case 0x000: // utrap.entry
            return hart->UTrap.entry;
        case 0x001: // utrap.pc
            return hart->UTrap.pc;
        case 0x002: // utrap.scratch
            return hart->UTrap.scratch;
        case 0x003: // utrap.cause
            return hart->UTrap.cause;
        case 0x004: // utrap.value
            return hart->UTrap.value;
        case 0x010: // strap.entry
            return hart->STrap.entry;
        case 0x011: // strap.pc
            return hart->STrap.pc;
        case 0x012: // strap.scratch
            return hart->STrap.scratch;
        case 0x013: // strap.cause
            return hart->STrap.cause;
        case 0x014: // strap.value
            return hart->STrap.value;
        case 0x020: // mtrap.entry
            return hart->MTrap.entry;
        case 0x021: // mtrap.pc
            return hart->MTrap.pc;
        case 0x022: // mtrap.scratch
            return hart->MTrap.scratch;
        case 0x023: // mtrap.cause
            return hart->MTrap.cause;
        case 0x024: // mtrap.value
            return hart->MTrap.value;
        case 0x100: // status
            return hart->status;
        case 0x101: // mhartid
            if((hart->status & (STATUS_MODE >> 5)) == 1 && (hart->status & STATUS_SUPHARTID) != 0) { // Supervisor
                return hart->hartid;
            } else if((hart->status & (STATUS_MODE >> 5)) == 2) { // Machine
                return hart->hartid;
            }
            return 0;
        case 0x102: // mtimer.freq
            if((hart->status & (STATUS_MODE >> 5)) == 1 && (hart->status & STATUS_SUPTIMER) != 0) { // Supervisor
                return 1000; // 1000 Hz
            } else if((hart->status & (STATUS_MODE >> 5)) == 2) { // Machine
                return 1000; // 1000 Hz
            }
            return 0;
        case 0x103: // mtimer.current
            if((hart->status & (STATUS_MODE >> 5)) == 1 && (hart->status & STATUS_SUPTIMER) != 0) { // Supervisor
                return 0;
            } else if((hart->status & (STATUS_MODE >> 5)) == 2) { // Machine
                return 0;
            }
            return 0;
        case 0x104: // mtimer.trigger
            if((hart->status & (STATUS_MODE >> 5)) == 1 && (hart->status & STATUS_SUPTIMER) != 0) { // Supervisor
                return 0;
            } else if((hart->status & (STATUS_MODE >> 5)) == 2) { // Machine
                return 0;
            }
            return 0;
        case 0x105: // mtimer.flags
            if((hart->status & (STATUS_MODE >> 5)) == 1 && (hart->status & STATUS_SUPTIMER) != 0) { // Supervisor
                return 0;
            } else if((hart->status & (STATUS_MODE >> 5)) == 2) { // Machine
                return 0;
            }
            return 0;
        default:
            if(index >= 0xF00 && index <= 0xF3F) { // TLB
                if((hart->status & (STATUS_MODE >> 5)) == 1 && (hart->status & STATUS_SUPTLB) != 0) { // Supervisor
                    return hart->TLB[index-0xF00];
                } else if((hart->status & (STATUS_MODE >> 5)) == 2) { // Machine
                    return hart->TLB[index-0xF00];
                }
            }
            return 0;
    }
}

static inline void IceWolf_WriteExReg(IcewolfHart_t* hart, int index, uint64_t value) {
    switch(index) {
        case 0x000: // utrap.entry
            hart->UTrap.entry = value;
            break;
        case 0x001: // utrap.pc
            hart->UTrap.pc = value;
            break;
        case 0x002: // utrap.acc
            hart->UTrap.acc = value;
            break;
        case 0x003: // utrap.scratch
            hart->UTrap.scratch = value;
            break;
        case 0x004: // utrap.cause
            hart->UTrap.cause = value;
            break;
        case 0x005: // utrap.value
            hart->UTrap.value = value;
            break;
        case 0x010: // strap.entry
            hart->STrap.entry = value;
            break;
        case 0x011: // strap.pc
            hart->STrap.pc = value;
            break;
        case 0x012: // strap.acc
            hart->STrap.acc = value;
            break;
        case 0x013: // strap.scratch
            hart->STrap.scratch = value;
            break;
        case 0x014: // strap.cause
            hart->STrap.cause = value;
            break;
        case 0x015: // strap.value
            hart->STrap.value = value;
            break;
        case 0x020: // mtrap.entry
            hart->MTrap.entry = value;
            break;
        case 0x021: // mtrap.pc
            hart->MTrap.pc = value;
            break;
        case 0x022: // mtrap.acc
            hart->MTrap.acc = value;
            break;
        case 0x023: // mtrap.scratch
            hart->MTrap.scratch = value;
            break;
        case 0x024: // mtrap.cause
            hart->MTrap.cause = value;
            break;
        case 0x025: // mtrap.value
            hart->MTrap.value = value;
            break;
        case 0x100: // status
            if((hart->status & STATUS_MODE) == 1<<5) { // Supervisor
                const uint64_t mask = STATUS_MTRAP_GATE | STATUS_STRAP_GATE | STATUS_UTRAP_GATE | STATUS_MODE | STATUS_SUPDEBUG | STATUS_SUPHARTID | STATUS_SUPTIMER | STATUS_SUPTLB | STATUS_MTRAP_MODE;
                hart->status = (value & ~mask) | (hart->status & mask);
            } else if((hart->status & STATUS_MODE) == 2<<5) { // Machine
                const uint64_t mask = STATUS_MTRAP_GATE | STATUS_STRAP_GATE | STATUS_UTRAP_GATE | STATUS_MODE;
                hart->status = (value & ~mask) | (hart->status & mask);
            }
            break;
        case 0x101: // mhartid
            break;
        case 0x102: // mtimer.freq
            break;
        case 0x103: // mtimer.current
            if((hart->status & (STATUS_MODE >> 5)) == 1 && (hart->status & STATUS_SUPTIMER) != 0) { // Supervisor
                // Nothing
            } else if((hart->status & (STATUS_MODE >> 5)) == 2) { // Machine
                // Nothing
            }
            break;
        case 0x104: // mtimer.trigger
            if((hart->status & (STATUS_MODE >> 5)) == 1 && (hart->status & STATUS_SUPTIMER) != 0) { // Supervisor
                // Nothing
            } else if((hart->status & (STATUS_MODE >> 5)) == 2) { // Machine
                // Nothing
            }
            break;
        case 0x105: // mtimer.flags
            if((hart->status & (STATUS_MODE >> 5)) == 1 && (hart->status & STATUS_SUPTIMER) != 0) { // Supervisor
                // Nothing
            } else if((hart->status & (STATUS_MODE >> 5)) == 2) { // Machine
                // Nothing
            }
            break;
        default:
            if(index >= 0xF00 && index <= 0xF3F) { // TLB
                if((hart->status & (STATUS_MODE >> 5)) == 1 && (hart->status & STATUS_SUPTLB) != 0) { // Supervisor
                    hart->TLB[index-0xF00] = value;
                } else if((hart->status & (STATUS_MODE >> 5)) == 2) { // Machine
                    hart->TLB[index-0xF00] = value;
                }
            }
            break;
    }
}

static inline uint64_t IceWolf_ReadReg(IcewolfHart_t* hart, int index) {
    switch(index) {
        case 0x00: case 0x01: case 0x02: case 0x03: case 0x04: case 0x05: case 0x06: case 0x07: case 0x08: case 0x09: case 0x0a: case 0x0b: case 0x0c: case 0x0d: case 0x0e: case 0x0f:
        case 0x10: case 0x11: case 0x12: case 0x13: case 0x14: case 0x15: case 0x16: case 0x17: case 0x18: case 0x19: case 0x1a: case 0x1b: case 0x1c: case 0x1d: case 0x1e: case 0x1f:
        case 0x20: case 0x5c:
            return hart->Registers[index];
        case 0x3f: // zero
            return 0;
        case 0x40: { // mem.lb
            uint64_t output = 0;
            IceWolf_MemAccess(hart,IceWolf_ReadReg(hart,hart->Registers[0x20]),(uint8_t*)&output,1,false,false);
            return output;
        }
        case 0x41: { // mem.lh
            uint64_t output = 0;
            IceWolf_MemAccess(hart,IceWolf_ReadReg(hart,hart->Registers[0x20]),(uint8_t*)&output,2,false,false);
            return output;
        }
        case 0x42: { // mem.lw
            uint64_t output = 0;
            IceWolf_MemAccess(hart,IceWolf_ReadReg(hart,hart->Registers[0x20]),(uint8_t*)&output,4,false,false);
            return output;
        }
        case 0x43: { // mem.ld
            uint64_t output = 0;
            IceWolf_MemAccess(hart,IceWolf_ReadReg(hart,hart->Registers[0x20]),(uint8_t*)&output,8,false,false);
            return output;
        }
        case 0x5d: // xreg.val
            return IceWolf_ReadExReg(hart,hart->Registers[0x5c]);
        default:
            IceWolf_Trap(hart,TRAP_UNDEFINED);
            return 0;
    }
}

static inline void IceWolf_WriteReg(IcewolfHart_t* hart, int index, uint64_t value) {
    switch(index) {
        case 0x00:
            hart->Registers[0] = value;
            hart->Branched = true;
            break;
        case 0x01: case 0x02: case 0x03: case 0x04: case 0x05: case 0x06: case 0x07: case 0x08: case 0x09: case 0x0a: case 0x0b: case 0x0c: case 0x0d: case 0x0e: case 0x0f:
        case 0x10: case 0x11: case 0x12: case 0x13: case 0x14: case 0x15: case 0x16: case 0x17: case 0x18: case 0x19: case 0x1a: case 0x1b: case 0x1c: case 0x1d: case 0x1e: case 0x1f:
        case 0x20: case 0x5c:
            hart->Registers[index] = value;
            break;
        case 0x21: // alu.add
            IceWolf_WriteReg(hart, hart->Registers[0x20], IceWolf_ReadReg(hart,hart->Registers[0x20]) + value);
            break;
        case 0x22: // alu.sub
            IceWolf_WriteReg(hart, hart->Registers[0x20], IceWolf_ReadReg(hart,hart->Registers[0x20]) - value);
            break;
        case 0x23: // alu.xor
            IceWolf_WriteReg(hart, hart->Registers[0x20], IceWolf_ReadReg(hart,hart->Registers[0x20]) ^ value);
            break;
        case 0x24: // alu.or
            IceWolf_WriteReg(hart, hart->Registers[0x20], IceWolf_ReadReg(hart,hart->Registers[0x20]) | value);
            break;
        case 0x25: // alu.and
            IceWolf_WriteReg(hart, hart->Registers[0x20], IceWolf_ReadReg(hart,hart->Registers[0x20]) & value);
            break;
        case 0x26: // alu.sll
            IceWolf_WriteReg(hart, hart->Registers[0x20], IceWolf_ReadReg(hart,hart->Registers[0x20]) << value);
            break;
        case 0x27: // alu.srl
            IceWolf_WriteReg(hart, hart->Registers[0x20], IceWolf_ReadReg(hart,hart->Registers[0x20]) >> value);
            break;
        case 0x28: // alu.sra
            IceWolf_WriteReg(hart, hart->Registers[0x20], (int64_t)IceWolf_ReadReg(hart,hart->Registers[0x20]) >> value);
            break;
        case 0x29: // alu.slt
            IceWolf_WriteReg(hart, hart->Registers[0x20], (int64_t)IceWolf_ReadReg(hart,hart->Registers[0x20]) < (int64_t)value);
            break;
        case 0x2a: // alu.sltu
            IceWolf_WriteReg(hart, hart->Registers[0x20], IceWolf_ReadReg(hart,hart->Registers[0x20]) < value);
            break;
        case 0x2b: // alu.mul
            IceWolf_WriteReg(hart, hart->Registers[0x20], (IceWolf_ReadReg(hart,hart->Registers[0x20]) * value) & 0xFFFFFFFF);
            break;
        case 0x2c: // alu.mulh
            IceWolf_WriteReg(hart, hart->Registers[0x20], ((int64_t)IceWolf_ReadReg(hart,hart->Registers[0x20]) * (int64_t)value) >> 32);
            break;
        case 0x2d: // alu.mulsu
            IceWolf_WriteReg(hart, hart->Registers[0x20], ((int64_t)IceWolf_ReadReg(hart,hart->Registers[0x20]) * (uint64_t)value) >> 32);
            break;
        case 0x2e: // alu.mulu
            IceWolf_WriteReg(hart, hart->Registers[0x20], (IceWolf_ReadReg(hart,hart->Registers[0x20]) * value) >> 32);
            break;
        case 0x2f: // alu.div
            if(value == 0) {
                IceWolf_Trap(hart,TRAP_DIVBYZERO);
                return;
            }
            IceWolf_WriteReg(hart, hart->Registers[0x20], (int64_t)IceWolf_ReadReg(hart,hart->Registers[0x20]) / (int64_t)value);
            break;
        case 0x30: // alu.divu
            if(value == 0) {
                IceWolf_Trap(hart,TRAP_DIVBYZERO);
                return;
            }
            IceWolf_WriteReg(hart, hart->Registers[0x20], IceWolf_ReadReg(hart,hart->Registers[0x20]) / value);
            break;
        case 0x31: // alu.rem
            if(value == 0) {
                IceWolf_Trap(hart,TRAP_DIVBYZERO);
                return;
            }
            IceWolf_WriteReg(hart, hart->Registers[0x20], (int64_t)IceWolf_ReadReg(hart,hart->Registers[0x20]) % (int64_t)value);
            break;
        case 0x32: // alu.remu
            if(value == 0) {
                IceWolf_Trap(hart,TRAP_DIVBYZERO);
                return;
            }
            IceWolf_WriteReg(hart, hart->Registers[0x20], IceWolf_ReadReg(hart,hart->Registers[0x20]) % value);
            break;
        case 0x3f:
            break;
        case 0x44: // mem.sb
            IceWolf_MemAccess(hart,IceWolf_ReadReg(hart,hart->Registers[0x20]),(uint8_t*)&value,1,true,false);
            break;
        case 0x45: // mem.sh
            IceWolf_MemAccess(hart,IceWolf_ReadReg(hart,hart->Registers[0x20]),(uint8_t*)&value,2,true,false);
            break;
        case 0x46: // mem.sw
            IceWolf_MemAccess(hart,IceWolf_ReadReg(hart,hart->Registers[0x20]),(uint8_t*)&value,4,true,false);
            break;
        case 0x47: // mem.sd
            IceWolf_MemAccess(hart,IceWolf_ReadReg(hart,hart->Registers[0x20]),(uint8_t*)&value,8,true,false);
            break;
        case 0x4f: // mem.fence
            switch(value) {
                case 0x00: // sfence.dl (Invalidate Dirty Lines in Data Cache Locally)
                case 0x01: // sfence.dg (Invalidate Dirty Lines in Data Cache Globally)
                    for(int i=0; i < CACHELINECOUNT; i++) {
                        if((hart->DCacheTag[i] & 3) == 3) {
                            uint64_t addr = hart->DCacheTag[i] & ~(CACHELINESIZE-1);
                            IceBusWrite(addr,16,(uint8_t*)((uintptr_t)hart+offsetof(IcewolfHart_t,DCache)+(i*CACHELINESIZE)));
                            hart->DCacheTag[i] &= ~2;
                        }
                    }
                    hart->DCache_WriteIndex = 0;
                    hart->TicksUntilFlush = 0;
                    break;
                case 0x02: // sfence.dal (Invalidate Entire Data Cache Locally)
                case 0x03: // sfence.dag (Invalidate Entire Data Cache Globally)
                    for(int i=0; i < CACHELINECOUNT; i++) {
                        if((hart->DCacheTag[i] & 3) == 3) {
                            uint64_t addr = hart->DCacheTag[i] & ~(CACHELINESIZE-1);
                            IceBusWrite(addr,16,(uint8_t*)((uintptr_t)hart+offsetof(IcewolfHart_t,DCache)+addr));
                        }
                        hart->DCacheTag[i] = 0;
                    }
                    break;
                case 0x04: // fence.i (Invalidate Instruction Cache Globally)
                    for(int i=0; i < CACHELINECOUNT; i++) {
                        hart->ICacheTag[i] = 0;
                    }
                    break;
                case 0x05: // fence
                    break;
                default:
                    IceWolf_Trap(hart,TRAP_UNDEFINED);
                    break;
            }
            break;
        case 0x50: // bran.eq
            if(IceWolf_ReadReg(hart,value & 0xFF) == IceWolf_ReadReg(hart,(value & 0xFF00) >> 8)) {
                hart->Branched = true;
                hart->Registers[0] += (((int64_t)(hart->LUI << (64-(hart->LUILength*8)) )) >> (64-(hart->LUILength*8)) );
            }
            break;
        case 0x51: // bran.ne
            if((IceWolf_ReadReg(hart,value & 0xFF)) != (IceWolf_ReadReg(hart,(value & 0xFF00) >> 8))) {
                hart->Branched = true;
                hart->Registers[0] += (((int64_t)(hart->LUI << (64-(hart->LUILength*8)) )) >> (64-(hart->LUILength*8)) );
            }
            break;
        case 0x52: // bran.lt
            if((int64_t)IceWolf_ReadReg(hart,value & 0xFF) < (int64_t)IceWolf_ReadReg(hart,(value & 0xFF00) >> 8)) {
                hart->Branched = true;
                hart->Registers[0] += (((int64_t)(hart->LUI << (64-(hart->LUILength*8)) )) >> (64-(hart->LUILength*8)) );
            }
            break;
        case 0x53: // bran.ge
            if((int64_t)IceWolf_ReadReg(hart,value & 0xFF) >= (int64_t)IceWolf_ReadReg(hart,(value & 0xFF00) >> 8)) {
                hart->Branched = true;
                hart->Registers[0] += (((int64_t)(hart->LUI << (64-(hart->LUILength*8)) )) >> (64-(hart->LUILength*8)) );
            }
            break;
        case 0x54: // bran.ltu
            if(IceWolf_ReadReg(hart,value & 0xFF) < IceWolf_ReadReg(hart,(value & 0xFF00) >> 8)) {
                hart->Branched = true;
                hart->Registers[0] += (((int64_t)(hart->LUI << (64-(hart->LUILength*8)) )) >> (64-(hart->LUILength*8)) );
            }
            break;
        case 0x55: // bran.geu
            if(IceWolf_ReadReg(hart,value & 0xFF) >= IceWolf_ReadReg(hart,(value & 0xFF00) >> 8)) {
                hart->Branched = true;
                hart->Registers[0] += (((int64_t)(hart->LUI << (64-(hart->LUILength*8)) )) >> (64-(hart->LUILength*8)) );
            }
            break;
        case 0x56: // bran.lnk
            hart->Branched = true;
            IceWolf_WriteReg(hart,value & 0xFF,hart->Registers[0]+4);
            hart->Registers[0] += (((int64_t)(hart->LUI << (64-(hart->LUILength*8)) )) >> (64-(hart->LUILength*8)) );
            break;
        case 0x57: // bran.rel
            hart->Branched = true;
            hart->Registers[0] += (((int64_t)(value << (64-(hart->LUILength*8)) )) >> (64-(hart->LUILength*8)) );
            break;
        case 0x58: // bran.rln
            hart->Branched = true;
            IceWolf_WriteReg(hart,value & 0xFF,hart->Registers[0]+4);
            hart->Registers[0] = IceWolf_ReadReg(hart,(value & 0xFF00) >> 8);
            break;
        case 0x5a: // trapret
            if((hart->status & STATUS_MODE) == 0) { // User
                if((hart->UTrap.entry & 3) == 3) {
                    hart->UTrap.entry &= ~2UL;
                    hart->UTrap.cause = 0;
                    hart->Registers[0x00] = hart->UTrap.pc;
                    hart->Registers[0x20] = hart->UTrap.acc;
                }
            } else if((hart->status & STATUS_MODE) == 1<<5) { // Supervisor
                if((hart->STrap.entry & 3) == 3) {
                    hart->STrap.entry &= ~2UL;
                    hart->STrap.cause = 0;
                    hart->Registers[0x00] = hart->STrap.pc;
                    hart->Registers[0x20] = hart->STrap.acc;
                }
            } else if((hart->status & STATUS_MODE) == 2<<5) { // Machine
                if((hart->MTrap.entry & 3) == 3) {
                    hart->MTrap.entry &= ~2UL;
                    hart->MTrap.cause = 0;
                    hart->Registers[0x00] = hart->MTrap.pc;
                    hart->Registers[0x20] = hart->MTrap.acc;
                }
            }
            break;
        case 0x5b: // ecall
            IceWolf_Trap(hart,TRAP_ENVCALL);
            break;
        case 0x5d: // xreg.val
            IceWolf_WriteExReg(hart,hart->Registers[0x5c],value);
            break;
        case 0x5e: { // xreg.swp
            uint64_t old = IceWolf_ReadReg(hart,value & 0xFF);
            IceWolf_WriteReg(hart,value & 0xFF,IceWolf_ReadExReg(hart,hart->Registers[0x5c]));
            IceWolf_WriteExReg(hart,hart->Registers[0x5c],old);
            break; 
        }
        case 0x5f: // lui
            fprintf(stderr, "\x1b[31mwat\x1b[0m\n");
            for(int i=0; i < 0x20; i++) {
                fprintf(stderr, "%02i: %016lx\n", i, hart->Registers[i]);
            }
            abort();
            break;
        default:
            IceWolf_Trap(hart,TRAP_UNDEFINED);
            break;
    }
}

int IceWolf_RunCycles(IcewolfHart_t* hart, int cycles) {
    int i = 0;
    int real_count = 0;
    for(int i = 0; i < cycles; i++) {
        if(hart->StallTicks) {
			hart->StallTicks--;
			continue;
		}
        real_count += 1;
        if(hart->TicksUntilFlush)
            if(!--hart->TicksUntilFlush) {
                bool found = false;
                int i;
                for (i = hart->DCache_WriteIndex; i<CACHELINECOUNT; i++) {
					if (hart->DCacheTag[i]) {
						found = true;
						break;
					}
				}
                if (!found)
					for (i = 0; i<(hart->DCache_WriteIndex); i++) {
						if (hart->DCacheTag[i]) {
							found = true;
							break;
						}
					}
                if (found) {
                    IceBusWrite(hart->DCacheTag[i] & ~0xF, CACHELINESIZE, (uint8_t*)((uintptr_t)hart+offsetof(IcewolfHart_t,DCache)+(i*CACHELINESIZE)));
                    hart->DCacheTag[i] = 0;
                    hart->DCache_WriteIndex = (i+1)&(CACHELINECOUNT-1);
                    if (hart->DCache_DirtyCount--)
						hart->TicksUntilFlush = BUS_STALL;
                }
            }
        if((hart->status & 7) != 0) { // Trap!
            if((hart->status & STATUS_UTRAP_GATE) != 0) { // UTrap
                hart->status &= ~STATUS_MODE;
                hart->status |= ((hart->status & STATUS_UTRAP_MODE)>>12)<<5;
                hart->UTrap.pc = hart->Registers[0];
                hart->UTrap.acc = hart->Registers[0x20];
                hart->Registers[0] = hart->UTrap.entry & ~3;
                hart->UTrap.entry |= 2;
                hart->status &= ~STATUS_UTRAP_GATE;
            } else if((hart->status & STATUS_STRAP_GATE) != 0) { // STrap
                hart->status &= ~STATUS_MODE;
                hart->status |= ((hart->status & STATUS_STRAP_MODE)>>14)<<5;
                hart->STrap.pc = hart->Registers[0];
                hart->STrap.acc = hart->Registers[0x20];
                hart->Registers[0] = hart->STrap.entry & ~3;
                hart->STrap.entry |= 2;
                hart->status &= ~STATUS_STRAP_GATE;
            } else if((hart->status & STATUS_MTRAP_GATE) != 0) { // MTrap
                hart->status &= ~STATUS_MODE;
                hart->status |= ((hart->status & STATUS_MTRAP_MODE)>>16)<<5;
                hart->MTrap.pc = hart->Registers[0];
                hart->MTrap.acc = hart->Registers[0x20];
                hart->Registers[0] = hart->MTrap.entry & ~3;
                hart->MTrap.entry |= 2;
                hart->status &= ~STATUS_MTRAP_GATE;
            }
        }
        uint32_t opcode;
        if((hart->status & 7) == 0) {
            hart->Branched = false;
            if(IceWolf_MemAccess(hart,hart->Registers[0],(uint8_t*)&opcode,4,false,true)) {
                if(opcode & 0x80) {
                    if((opcode & 0x7F) == 0x5f) {
                        if(hart->LUILength >= 6) {
                            IceWolf_Trap(hart,TRAP_UNDEFINED);
                            break;
                        }
                        hart->LUI = (hart->LUI << 16) | ((opcode & 0xFFFF0000) >> 16);
                        hart->LUILength += 2;
                    } else {
                        IceWolf_WriteReg(hart,opcode & 0x7F,((opcode & 0xFFFF0000)>>16) | (hart->LUI << 16));
                        hart->LUI = 0UL;
                        hart->LUILength = 0;
                    }
                    hart->Registers[0] = hart->Branched?hart->Registers[0]:hart->Registers[0]+4;
                } else {
                    IceWolf_WriteReg(hart,opcode & 0x7F,IceWolf_ReadReg(hart,(opcode & 0x7F00) >> 8));
                    hart->Registers[0] = hart->Branched?hart->Registers[0]:hart->Registers[0]+4;
                }
            }
        }
    }
    return real_count;
}

IcewolfHart_t* IceWolf_CreateHart(uint64_t id) {
    IcewolfHart_t* hart = malloc(sizeof(IcewolfHart_t));
    memset((uint8_t*)hart, 0, sizeof(IcewolfHart_t));
    hart->hartid = id;
    hart->status = (2<<5);
    hart->Registers[0] = 0x80000000;
    return hart;
}