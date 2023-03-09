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
2: SCSI FIFO Buffer
3: Transfer Base
4: Transfer Length
*/

typedef enum {
    READY,
    CMD_RECIEVE,
    
} SCSIPhase;

typedef struct {
    FILE* image;
    uint32_t busID;
    uint32_t available;
    uint32_t blocks;
    bool isSpinning;
} SCSIDrive;

void SCSIDoCommand(SCSIDrive* drive, uint8_t cmd) {
    int do_dma = cmd & 0x80;
    switch(cmd & 0x7f) {
        case 0: { // Send Data
            
        }
        case 1: { // Send Message

        }
        case 0x40: { // Recieve Data

        }
        case 0x41: { // Recieve Message

        }
    }
}