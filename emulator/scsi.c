#include "scsi.h"
#include "okamiboard.h"
#include "htc.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
/*
Layout:
0: Drive Select
1: Controller Command
2: Transfer Base
3: Transfer Length
*/

typedef enum {
    READY,
    CMD_RECIEVE,
    BUSY,
} SCSIPhase;

typedef struct {
    FILE* image;
    uint32_t busID;
    uint32_t available;
    uint32_t blocks;
    bool isSpinning;
} SCSIDrive;

uint8_t* SCSIBuffer = NULL;

void SCSIDoCommand(SCSIDrive* drive, uint8_t cmd) {
    switch(cmd) {
        case 0: { // Send Data
            
        }
        case 1: { // Send Message

        }
        case 0x80: { // Recieve Data

        }
        case 0x81: { // Recieve Message

        }
    }
}

bool SCSIRead() {

}

bool SCSIWrite() {
    
}

void SCSIInit() {

}