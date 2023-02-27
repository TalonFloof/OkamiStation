#include "okamiboard.h"
#include "htc.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

uint8_t ROM[128*1024];
OkamiPort OkamiPorts[256];

int OkamiBoardRead(uint32_t addr, uint32_t len, void *buf) {
    if(addr < 0x400) { // I/O Ports
        if(OkamiPorts[addr >> 2].isPresent) {
            return OkamiPorts[addr >> 2].read(addr >> 2, len, (uint32_t*)buf);
        }
    } else if(addr >= 0x1F00000) { // Firmware
        memcpy(buf,((uint8_t*)&ROM)+(addr-0x1F00000),len);
        return 1;
    }
    return 0;
}

int OkamiBoardWrite(uint32_t addr, uint32_t len, void *buf) {
    if(addr < 0x400) { // I/O Ports
        if(OkamiPorts[addr >> 2].isPresent) {
            return OkamiPorts[addr >> 2].write(addr >> 2, len, *((uint32_t*)buf));
        }
    } else if(addr >= 0x1F00000) { // Firmware
        return 1;
    }
    return 0;
}

void OkamiBoardInit() {
    KoriBusBanks[15].Used = true;
    KoriBusBanks[15].Read = OkamiBoardRead;
    KoriBusBanks[15].Write = OkamiBoardWrite;
    for(int i=0; i < 256; i++) {
        OkamiPorts[i].isPresent = false;
    }
    HTCInit();
    FILE *firmware = fopen("Firmware.bin", "r");
    if(firmware == NULL) {
        fprintf(stderr, "Couldn't open the Boot ROM\n");
        exit(1);
    }
    fread(&ROM, sizeof(ROM), 1, firmware);
    fclose(firmware);
}