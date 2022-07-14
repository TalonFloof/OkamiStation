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
#include <stdio.h>

typedef int (*IceBusDevWrite)(uint64_t addr, uint64_t len, void *buf);
typedef int (*IceBusDevRead)(uint64_t addr, uint64_t len, void *buf);

typedef struct IceBusBank {
    bool Used;
    IceBusDevRead Read;
    IceBusDevRead Write;
} IceBusBank_t;

extern IceBusBank_t IceBusBanks[16];

static inline int IceBusRead(uint64_t addr, uint64_t len, uint8_t* buf) {
    int bank = addr >> 31;
    if(IceBusBanks[bank].Used) {
        return IceBusBanks[bank].Read(addr & 0x7FFFFFFF, len, (void*)buf);
    }
    return 1;
}


static inline int IceBusWrite(uint64_t addr, uint64_t len, uint8_t* buf) {
    int bank = addr >> 31;
    if(IceBusBanks[bank].Used) {
        return IceBusBanks[bank].Write(addr & 0x7FFFFFFF, len, (void*)buf);
    }
    return 1;
}

bool IceBusInit();