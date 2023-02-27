#include "okamiboard.h"

uint32_t HTCRegisters[2];

int HTCInterrupt(int irq) {
    if(!( & (1 << (irq&0x1F)))) {
        // Trigger the interrupt
    }
}

int HTCRead(uint32_t port, uint32_t length, uint32_t *value) {
    *value = HTCRegisters[port];
    if(port == 0x2) { // 
    }
}

int HTCWrite(uint32_t port, uint32_t length, uint32_t value) {
    if(port == 0x2) { // Acknowledge
        
    }
}

/* The HTC is the Hardware Trap Controller */
/* Registers: 0-1 Interrupt Mask 2 Interrupt Source */
void HTCInit() {
    HTCRegisters[0] = 0xFFFFFFFF;
    HTCRegisters[1] = 0;
    for(int i=0; i < 3; i++) {
        OkamiPorts[i].isPresent = 1;
        OkamiPorts[i].read = HTCRead;
        OkamiPorts[i].write = HTCWrite;
    }
}