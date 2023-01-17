#include <stdint.h>

uint32_t registers[32];

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

void next() {
    
}