#include "okamiboard.h"
#include "okami1041.h"

uint32_t HTCRegisters[2];

/*
Interrupt Layout:
0 Timer, 1 OIPB #0, 2 OIPB #1, 3 OIPB #2, 4 SCSI
*/

int HTCInterrupt(int irq) {
    if(!(HTCRegisters[0] & (1 << (irq&0x1F)))) {
        if(HTCRegisters[1] == 0) {
            triggerTrap(1,0,false);
        }
        HTCRegisters[1] = HTCRegisters[1] | (1 << (irq&0x1f));
    }
}

int HTCRead(uint32_t port, uint32_t length, uint32_t *value) {
    *value = HTCRegisters[port];
    if(port == 0x2) { // Claim
        for(int i=0; i < 32; i++) {
            if(HTCRegisters[1] & (1 << (i&0x1f))) {
                *value = i+1;
                return 1;
            }
        }
        *value = 0;
        return 1;
    } else {
        *value = HTCRegisters[port];
        return 1;
    }
    return 0;
}

int HTCWrite(uint32_t port, uint32_t length, uint32_t value) {
    if(port == 0x2) { // Acknowledge
        HTCRegisters[1] = HTCRegisters[1] & ~(1 << (value&0x1f));
        return 1;
    } else if(port == 0x0) {
        HTCRegisters[port] = value;
        return 1;
    }
    return 0;
}

/* The HTC is the Hardware Trap Controller */
/* Registers: 0 Interrupt Mask 1 Interrupt Source */
void HTCInit() {
    HTCRegisters[0] = 0xFFFFFFFF;
    HTCRegisters[1] = 0;
    for(int i=0; i < 3; i++) {
        OkamiPorts[i].isPresent = 1;
        OkamiPorts[i].read = HTCRead;
        OkamiPorts[i].write = HTCWrite;
    }
}