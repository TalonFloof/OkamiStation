#include "scsi.h"
#include "okamiboard.h"
#include "koribus.h"
#include "htc.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

/* i may or may not have stolen hyenasky's disk geometry... */
#define LBA_TO_BLOCK(lba)    ((lba)%63)
#define LBA_TO_TRACK(lba)    ((lba)/63)
#define LBA_TO_CYLINDER(lba) (LBA_TO_TRACK(lba)/4)

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
    uint8_t* data;
    uint32_t dataOffset;
    uint32_t dataLen;
    bool hasStatus;
    uint8_t status;
    uint8_t phase;

    uint8_t k;
    uint8_t c;
    uint8_t q;

    bool isSpinning;
    bool isSeeking;
    uint32_t headLocation;
	uint32_t opInterval;
	uint32_t seekDestination;
	uint32_t consecutiveZeroSeekCount;
    uint32_t platterLocation;
    uint8_t opPhase;
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

void SCSISeek(uint8_t id, uint32_t lba, uint8_t phase) {
    SCSIDrive* drive = &SCSIDrives[id];
    drive->isSeeking = true;
    int cylseek = abs(((int)(drive->headLocation) - (int)(LBA_TO_CYLINDER(lba)))) / (1000/200);
    if (drive->headLocation != LBA_TO_CYLINDER(lba))
        cylseek += 3;
    int blockseek = LBA_TO_BLOCK(lba) - drive->platterLocation;
    if(blockseek < 0)
        blockseek += 63;
    drive->opPhase = phase;
    drive->opInterval = cylseek + blockseek/(63/(1000/(3600/60)));
    drive->seekDestination = lba;
    if(drive->opInterval == 0) {
        drive->consecutiveZeroSeekCount += blockseek;
        if(drive->consecutiveZeroSeekCount > (63/(1000/(3600/60)))) {
            drive->opInterval = 1;
            drive->consecutiveZeroSeekCount = 0;
        }
    } else {
        drive->consecutiveZeroSeekCount = 0;
    }
}

void SCSITick() {
    for(int i=0; i < 8; i++) {
        if (SCSIDrives[i].image == NULL)
			continue;
        if(SCSIDrives[i].isSeeking && SCSIDrives[i].opInterval <= 1) {
            SCSIDrives[i].isSeeking = false;
            SCSIDrives[i].opInterval = 0;
            SCSIDrives[i].platterLocation = LBA_TO_BLOCK(SCSIDrives[i].seekDestination);
			SCSIDrives[i].headLocation = LBA_TO_CYLINDER(SCSIDrives[i].seekDestination);
            SCSIDrives[i].phase = SCSIDrives[i].opPhase;
        } else if(SCSIDrives[i].opInterval > 0) {
            SCSIDrives[i].opInterval--;
        } else if (SCSIDrives[i].isSpinning) {
			SCSIDrives[i].platterLocation += (63/(1000/(3600/60)));
			SCSIDrives[i].platterLocation %= 63;
		}
    }
}

void SCSIDoCommand(uint8_t id) {
    switch(SCSIDrives[id].cmd[0]) {
        case 0x08: { // READ(6)
            if(!SCSIDrives[id].isSpinning) {
                SCSICheckStatusError(id,2,4,2); /* Not Ready - need initialise command (start unit) */
                break;
            }
            uint32_t lba = (SCSIDrives[id].cmd[1]<<16)|(SCSIDrives[id].cmd[2]<<8)|SCSIDrives[id].cmd[3];
            uint32_t blocks = SCSIDrives[id].cmd[4];
            SCSIDrives[id].dataOffset = 0;
            SCSIDrives[SCSIController.id].dataLen = blocks*512;
            fseek(SCSIDrives[id].image,lba*512,SEEK_SET);
            SCSISeek(id,lba,PHASE_DATA_IN);
            SCSISuccess(id);
            break;
        }
        case 0x0a: { // WRITE(6)
            if(!SCSIDrives[id].isSpinning) {
                SCSICheckStatusError(id,2,4,2); /* Not Ready - need initialise command (start unit) */
                break;
            }
            uint32_t lba = (SCSIDrives[id].cmd[1]<<16)|(SCSIDrives[id].cmd[2]<<8)|SCSIDrives[id].cmd[3];
            uint32_t blocks = SCSIDrives[id].cmd[4];
            SCSIDrives[id].dataOffset = 0;
            SCSIDrives[id].dataLen = blocks*512;
            fseek(SCSIDrives[id].image,lba*512,SEEK_SET);
            SCSISeek(id,lba,PHASE_DATA_OUT);
            SCSISuccess(id);
            break;
        }
        case 0x1b: { // START STOP UNIT
            SCSIDrives[id].isSpinning = (SCSIDrives[id].cmd[4] & 1);
            SCSISuccess(id);
            break;
        }
        case 0x25: { // READ CAPACITY(10)
            SCSIDrives[id].data = malloc(8);
            SCSIDrives[id].dataOffset = 0;
            SCSIDrives[id].dataLen = 8;
            ((uint32_t*)SCSIDrives[id].data)[0] = SCSIDrives[id].blocks-1;
            ((uint32_t*)SCSIDrives[id].data)[1] = 512;
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
                case PHASE_DATA_IN: {
                    if(SCSIDrives[SCSIController.id].cmd[0] == 0x08) {
                        fread(value,1,1,SCSIDrives[SCSIController.id].image);
                    } else {
                        *value = SCSIDrives[SCSIController.id].data[SCSIDrives[SCSIController.id].dataOffset];
                    }
                    SCSIDrives[SCSIController.id].dataOffset += 1;
                    if(SCSIDrives[SCSIController.id].dataOffset >= SCSIDrives[SCSIController.id].dataLen) {
                        if(SCSIDrives[SCSIController.id].data != NULL)
                            free(SCSIDrives[SCSIController.id].data);
                        if(SCSIDrives[SCSIController.id].hasStatus) {
                            SCSIDrives[SCSIController.id].phase = PHASE_STATUS;
                        } else {
                            SCSIDrives[SCSIController.id].phase = 0;
                        }
                    }
                    return 1;
                }
                case PHASE_STATUS: {
                    *value = SCSIDrives[SCSIController.id].status;
                    SCSIDrives[SCSIController.id].phase = 0;
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
                case PHASE_DATA_OUT: {
                    if(SCSIDrives[SCSIController.id].cmd[0] == 0x0a) {
                        fwrite(&value,1,1,SCSIDrives[SCSIController.id].image);
                        SCSIDrives[SCSIController.id].dataOffset++;
                        if(SCSIDrives[SCSIController.id].dataOffset >= SCSIDrives[SCSIController.id].dataLen) {
                            SCSIDrives[SCSIController.id].dataOffset = 0;
                            SCSIDrives[SCSIController.id].dataLen = 0;
                            SCSIDrives[SCSIController.id].phase = PHASE_STATUS;
                        }
                        return 1;
                    } else {
                        SCSIDrives[SCSIController.id].data[SCSIDrives[SCSIController.id].dataOffset++] = value&0xFF;
                    }
                }
                case PHASE_COMMAND: {
                    SCSIDrives[SCSIController.id].cmd[SCSIDrives[SCSIController.id].cmdOffset++] = value&0xFF;
                    if(SCSIDrives[SCSIController.id].cmdOffset >= SCSIGetCommandLength(SCSIController.id)) {
                        SCSIDoCommand(SCSIController.id);
                        if(SCSIDrives[SCSIController.id].hasStatus && SCSIDrives[SCSIController.id].phase == PHASE_COMMAND && SCSIDrives[SCSIController.id].opInterval == 0) {
                            SCSIDrives[SCSIController.id].phase = PHASE_STATUS;
                        } else if(SCSIDrives[SCSIController.id].phase == PHASE_COMMAND && SCSIDrives[SCSIController.id].opInterval == 0) {
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
            SCSIController.id = (value&0xF);
            SCSIController.initiator_id = ((value>>4)&0xF);
            if(SCSIDrives[SCSIController.id].image != NULL && SCSIDrives[SCSIController.id].phase == 0) {
                SCSIDrives[SCSIController.id].phase = PHASE_COMMAND;
            }
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
    FILE* hdfile = fopen(path,"r+b");
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
    SCSIDrives[nextID].isSeeking = false;
    SCSIDrives[nextID].dataOffset = 0;
    SCSIDrives[nextID].dataLen = 0;
    SCSIDrives[nextID].data = NULL;
    nextID++;
}

void SCSICloseDrives() {
    for(int i=0; i < 8; i++) {
        if(SCSIDrives[i].image != NULL)
            fclose(SCSIDrives[i].image);
    }
}