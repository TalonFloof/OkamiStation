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

#pragma once
#include <stdint.h>
#include <stdbool.h>

#define HART_FREQ 30000000 // 30 MHz

#define MEMORY (8*1024*1024) // 8 MiB

#define CACHESIZESHIFT 15
#define CACHELINESHIFT 4
#define CACHEWAYSHIFT 0
#define CACHESIZE (1<<CACHESIZESHIFT)
#define CACHELINECOUNT (CACHESIZE>>CACHELINESHIFT)
#define CACHELINESIZE (1<<CACHELINESHIFT)
#define CACHEWAYCOUNT (1<<CACHEWAYSHIFT)
#define CACHESETCOUNT (1<<(CACHESIZESHIFT-CACHELINESHIFT-CACHEWAYSHIFT))

#define BUS_STALL (HART_FREQ/8333333)
#define MISS_STALL (BUS_STALL+1)

#define TRAP_TIMER 0
#define TRAP_IRQ 1
#define TRAP_ENVCALL 2
#define TRAP_DATA 3
#define TRAP_FETCH 4
#define TRAP_TLBMISS 5
#define TRAP_UNDEFINED 6
#define TRAP_DIVBYZERO 7

#define STATUS_MTRAP_GATE (1<<0)
#define STATUS_STRAP_GATE (1<<1)
#define STATUS_UTRAP_GATE (1<<2)
#define STATUS_USETLB (1<<3)
#define STATUS_RESERVED (1<<4)
#define STATUS_MODE (3<<5)
#define STATUS_SUPDEBUG (1<<7)
#define STATUS_SUPHARTID (1<<8)
#define STATUS_SUPTIMER (1<<9)
#define STATUS_SUPTLB (1<<10)
#define STATUS_RESERVED2 (1<<11)
#define STATUS_UTRAP_MODE (3<<12)
#define STATUS_STRAP_MODE (3<<14)
#define STATUS_MTRAP_MODE (3<<16)

typedef struct TrapRegs {
    uint64_t entry; // Bit 0: Enabled, Bit 1: In Service
    uint64_t pc;
    uint64_t scratch;
    uint64_t cause;
    uint64_t value;
} TrapRegs_t;

typedef struct IcewolfHart {
    uint64_t hartid;
    uint64_t status;
    int StallTicks;
    int TicksUntilFlush;
    uint64_t LUI;
    int LUILength;
    bool Branched;
    uint64_t Registers[96];
    //////////// Cache Stuff ////////////
    uint8_t ICache[CACHESIZE];
    uint64_t ICacheTag[CACHELINECOUNT];
    int ICache_Fill;
    uint8_t DCache[CACHESIZE];
    uint64_t DCacheTag[CACHELINECOUNT];
    int DCache_Fill;
    int DCache_WriteIndex;
    int DCache_DirtyCount;
    uint64_t TLB[64];
    /////////////////////////////////////
    TrapRegs_t UTrap;
    TrapRegs_t STrap;
    TrapRegs_t MTrap;
} IcewolfHart_t;

int IceWolf_RunCycles(IcewolfHart_t* hart, int cycles);
static inline bool IceWolf_MemAccess(IcewolfHart_t* hart, uint64_t addr, uint8_t* buf, uint64_t len, bool write, bool fetch);
void IceWolf_Trap(IcewolfHart_t* hart, int type);
IcewolfHart_t* IceWolf_CreateHart(uint64_t id);