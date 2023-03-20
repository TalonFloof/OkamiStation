#include "scsi.h"
#include "okamiboard.h"
#include "koribus.h"
#include "htc.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
/*
Registers:
0x20: Current SCSI Input/Output Data (R/W)
0x21: Phase & SCSI Bus Status (R/W)
0x22: Target and Initiator ID (R/W)
0x23: Status (R/W)
0x24: DMA Start Address (Initiates immediately after writing) (W)
0x25: DMA Length (W)
*/

typedef struct {
    FILE* image;
    uint32_t busID;
    uint32_t blocks;
    uint8_t cmd[10];
    uint8_t cmdOffset;
    bool hasStatus;
    uint8_t status;
    uint8_t phase;

    uint8_t k;
    uint8_t c;
    uint8_t q;

    bool isSpinning;
    uint32_t headTrackLocation;
	uint32_t operationInterval;
	uint32_t seekDestination;
	uint32_t consecutiveZeroSeekCount;
} SCSIDrive;

typedef enum {
    PHASE_DATA_OUT = 0b000,
    PHASE_COMMAND = 0b010,
    PHASE_MESSAGE_OUT = 0b110,
    PHASE_DATA_IN = 0b001,
    PHASE_STATUS = 0b011,
    PHASE_MESSAGE_IN = 0b111,
} SCSITransferPhases;

typedef struct {
    uint8_t initiator_id;
    uint8_t id;

    uint8_t data;
    uint32_t dma_start;
    uint32_t dma_length;
} OkamiC7429;

OkamiC7429 SCSIController;
SCSIDrive SCSIDrives[8];
int nextID = 0;

int SCSIGetID(uint8_t id) {
    int i = 0;
    for(;id > 0;id >>= 1) {
        if((id & 0x1) != 0) {
            return i;
        }
        i+=1;
    }
    return -1;
}

int SCSIGetCommandLength(uint8_t id) {
    int cmd = SCSIDrives[id].cmd[0];
    if(cmd <= 0x1f) {
        return 6;
    } else {
        return 10;
    }
}

void SCSICheckStatusError(uint8_t id, uint8_t k, uint8_t c, uint8_t q) {
    SCSIDrives[id].hasStatus = true;
    SCSIDrives[id].status = 0x2; // Check Condition
    SCSIDrives[id].k = k;
    SCSIDrives[id].c = c;
    SCSIDrives[id].q = q;
}

void SCSISuccess(uint8_t id) {
    SCSIDrives[id].hasStatus = true;
    SCSIDrives[id].status = 0;
}

void SCSIDoCommand(uint8_t id) {
    switch(SCSIDrives[id].cmd[0]) {
        case 0x08: { // READ(6)
            break;
        }
        case 0x1b: { // START STOP UNIT
            SCSIDrives[id].isSpinning = (SCSIDrives[id].cmd[4] & 1);
            SCSISuccess(id);
            break;
        }
        default: {
            SCSICheckStatusError(id,0x5,0x20,0x00);
            break;
        }
    }
}

int SCSIPortRead(uint32_t port, uint32_t length, uint32_t *value) {
    switch(port) {
        case 0x20: {
            switch(SCSIDrives[SCSIController.id].phase) {
                case PHASE_STATUS: {
                    SCSIDrives[SCSIController.id].phase = PHASE_COMMAND;
                    return 1;
                }
                default: {
                    return 0;
                }
            }
            return 1;
        }
        case 0x21: {
            *value = SCSIDrives[SCSIController.id].phase;
            return 1;
        }
        case 0x22: {
            *value = (((uint32_t)SCSIController.initiator_id)<<8)|(SCSIController.id);
            return 1;
        }
    }
    return 0;
}

int SCSIPortWrite(uint32_t port, uint32_t length, uint32_t value) {
    switch(port) {
        case 0x20: {
            switch(SCSIDrives[SCSIController.id].phase) {
                case PHASE_COMMAND: {
                    SCSIDrives[SCSIController.id].cmd[SCSIDrives[SCSIController.id].cmdOffset++] = value&0xFF;
                    if(SCSIDrives[SCSIController.id].cmdOffset >= SCSIGetCommandLength(SCSIController.id)) {
                        fprintf(stderr, "SCSI Run Command 0x%02x\n", SCSIDrives[SCSIController.id].cmd[0]);
                        SCSIDoCommand(SCSIController.id);
                        if(SCSIDrives[SCSIController.id].hasStatus) {
                            SCSIDrives[SCSIController.id].phase = PHASE_STATUS;
                        } else if(SCSIDrives[SCSIController.id].phase == PHASE_COMMAND) {
                            SCSIDrives[SCSIController.id].phase = PHASE_DATA_OUT;
                        }
                        SCSIDrives[SCSIController.id].cmdOffset = 0;
                    }
                    return 1;
                }
                default: {
                    return 0;
                }
            }
            break;
        }
        case 0x21: {
            /* We can't write to this because we will never be connected as the target device */
            return 1;
        }
        case 0x22: {
            SCSIController.id = (value&0xFF);
            SCSIController.initiator_id = ((value>>8)&0xFF);
            return 1;
        }
        case 0x24: { // DMA isn't implemented yet...
            return 0;
        }
        case 0x25: {
            SCSIController.dma_length = value;
            return 1;
        }
    }
    return 0;
}

void SCSIInit() {
    for(int i=0x20; i<=0x25; i++) {
        OkamiPorts[i].isPresent = 1;
        OkamiPorts[i].read = SCSIPortRead;
        OkamiPorts[i].write = SCSIPortWrite;
    }
}

void SCSIAttachDrive(const char* path) {
    FILE* hdfile = fopen(path,"r+");
    if(!hdfile) {
        fprintf(stderr, "WARN: Unable to attach SCSI Drive at path \"%s\"\n", path);
        return;
    }
    fseek(hdfile,0,SEEK_END);
    long size = ftell(hdfile);
    fseek(hdfile,0,SEEK_SET);
    SCSIDrives[nextID].image = hdfile;
    SCSIDrives[nextID].blocks = size/512;
    SCSIDrives[nextID].busID = nextID;
    SCSIDrives[nextID].cmdOffset = 0;
    SCSIDrives[nextID].hasStatus = false;
    SCSIDrives[nextID].phase = PHASE_COMMAND;
    SCSIDrives[nextID].isSpinning = false;
    nextID++;
}