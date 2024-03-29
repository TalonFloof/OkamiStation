#include "timer.h"
#include "okamiboard.h"
#include "htc.h"

/*
0x3: Timer Counter
0x4: Timer Hit
*/

uint32_t timerCounter = 0;
uint32_t timerHit = 0;

void TimerTick() {
    if(timerCounter >= timerHit) {
        HTCInterrupt(0);
        timerCounter -= timerHit;
    } else {
        timerCounter += 1;
    }
}

int TimerRead(uint32_t port, uint32_t length, uint32_t *value) {
    return 0;
}

int TimerWrite(uint32_t port, uint32_t length, uint32_t value) {
    if(port == 0x3) {
        timerCounter = value;
        return 1;
    } else if(port == 0x4) {
        timerHit = value;
        return 1;
    }
    return 0;
}

void TimerInit() {
    for(int i=3; i <= 4; i++) {
        OkamiPorts[i].isPresent = true;
        OkamiPorts[i].read = TimerRead;
        OkamiPorts[i].write = TimerWrite;
    }
}