#include "okamiboard.h"
#include "htc.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

uint8_t ROM[128*1024];
uint8_t NVRAM[0x10000];
OkamiPort OkamiPorts[256];

/*
Ports: 
  0x00-0x02: HTC
  0x03-0x04: Programmable Timer (clocked @ 1000 Hz)
  0x05-0x06: RTC
  0x07-0x0f: Audio Controller (maybe?)
  0x10-0x12: OIPB
  0x20-0x24: NCR 5380 (SCSI Controller)
  0x30-0x3f: Maybe a floppy disk controller?
  0xf0-0xf1: KoriBus Controller
*/


int OkamiBoardRead(uint32_t addr, uint32_t len, void *buf) {
    if(addr < 0x400) { // I/O Ports
        if(OkamiPorts[addr >> 2].isPresent) {
            return OkamiPorts[addr >> 2].read(addr >> 2, len, (uint32_t*)buf);
        }
    } else if(addr >= 0x1000 && addr <= 0x11000) { // NVRAM
        memcpy(buf,((uint8_t*)&NVRAM)+(addr-0x1000),len);
        return 1;
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
    } else if(addr >= 0x1000 && addr <= 0x11000) { // NVRAM
        memcpy(((uint8_t*)&NVRAM)+(addr-0x1000),buf,len);
        return 1;
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
    FILE *nvramfile = fopen("nvram.nv", "r");
    if(nvramfile != NULL) {
        fread(&NVRAM,sizeof(NVRAM),1,nvramfile);
        fclose(nvramfile);
    }
}

void OkamiBoardSaveNVRAM() {
    fprintf(stderr, "Flushing NVRAM...\n");
    FILE* nvramfile = fopen("nvram.nv", "w");
    fwrite(&NVRAM,sizeof(NVRAM),1,nvramfile);
    fclose(nvramfile);
}