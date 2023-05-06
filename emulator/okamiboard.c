#include "okamiboard.h"
#include "htc.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>
#elif __linux__
#include <unistd.h>
#elif __APPLE__
#include <mach-o/dyld.h>
#endif

uint8_t ROM[128*1024];
uint8_t NVRAM[0x10000];
OkamiPort OkamiPorts[256];
char executablePath[2048];

/*
Ports: 
  0x00-0x02: HTC
  0x03-0x04: Programmable Timer (clocked @ 1000 Hz)
  0x05-0x06: RTC
  0x07-0x0f: Audio Controller (maybe?)
  0x10-0x12: OIPB
  0x20-0x25: OkamiC7429 (SCSI Controller)
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
    #if _WIN32
        int size = GetModuleFileNameA(NULL, executablePath, 2047);
        executablePath[size] = '\0';
    #elif __linux__
        int size = readlink("/proc/self/exe", executablePath, 2047);
        executablePath[size] = '\0';
    #elif __APPLE__
        int size = 2048;
        _NSGetExecutablePath(executablePath, &size);
    #else
        strcpy(executablePath, "./okamistation")
    #endif
    int i = strlen(executablePath);
    while(executablePath[i] != '/') {i--;}
    executablePath[i+13] = '\0';
    memcpy((char*)&executablePath[i+1],"Firmware.bin",12);
    KoriBusBanks[15].Used = true;
    KoriBusBanks[15].Read = OkamiBoardRead;
    KoriBusBanks[15].Write = OkamiBoardWrite;
    for(int i=0; i < 256; i++) {
        OkamiPorts[i].isPresent = false;
    }
    HTCInit();
    FILE *firmware = fopen((char*)&executablePath, "rb");
    if(firmware == NULL) {
        fprintf(stderr, "Couldn't open the Boot ROM\n");
        exit(1);
    }
    fread(&ROM, sizeof(ROM), 1, firmware);
    fclose(firmware);
    executablePath[i+9] = '\0';
    memcpy((char*)&executablePath[i+1],"nvram.nv",8);
    FILE *nvramfile = fopen((char*)&executablePath, "rb");
    if(nvramfile != NULL) {
        fread(&NVRAM,sizeof(NVRAM),1,nvramfile);
        fclose(nvramfile);
    }
}

void OkamiBoardSaveNVRAM() {
    fprintf(stderr, "Flushing NVRAM...\n");
    FILE* nvramfile = fopen((char*)&executablePath, "wb");
    fwrite(&NVRAM,sizeof(NVRAM),1,nvramfile);
    fclose(nvramfile);
}