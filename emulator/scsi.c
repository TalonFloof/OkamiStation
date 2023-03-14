#include "scsi.h"
#include "okamiboard.h"
#include "koribus.h"
#include "htc.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
/*
This file implements the NCR 5380 controller and the SCSI Hard Drives attached to it.

Registers:
0x20: Current SCSI Input/Output Data (R/W)
0x21: Initiator Command (R/W)
0x22: Mode (R/W)
0x23: Target Command (R/W)
0x24: Current SCSI Bus Status (R)
0x24: Select Enable (W)
0x25: Bus and Status (R)
0x25: Start DMA Send (W)
0x26: Input Data (R)
0x26: Start DMA Target Recieve (W)
0x27: Reset Parity/Interrupts (R)
0x27: Start DMA Initiator Recieve (W)
*/

typedef struct {

} SCSISignal;

typedef struct {
    FILE* image;
    uint32_t busID;
    uint32_t blocks;
    uint32_t heads;
    uint32_t cylinders;
    uint32_t sectors;
    bool isSpinning;
} SCSIDrive;

typedef enum {
    PHASE_DATA_OUT = 0b000,
    PHASE_COMMAND = 0b100,
    PHASE_MESSAGE_OUT = 0b110,
    PHASE_DATA_IN = 0b001,
    PHASE_STATUS = 0b011,
    PHASE_MESSAGE_IN = 0b111,
} NCRTransferPhases;

typedef struct {
    uint8_t data_in;
    uint8_t data_out;
    union { /* Initiator Command Register */
        uint8_t raw;
        struct {
            uint8_t assert_databus : 1;
            uint8_t assert_atn : 1;
            uint8_t assert_sel : 1;
            uint8_t assert_bsy : 1;
            uint8_t assert_ack : 1;
            uint8_t la : 1;
            uint8_t aip : 1;
            uint8_t assert_rst : 1;
        } bits;
    } icr;
    union { /* Mode Register */
        uint8_t raw;
        struct {
            uint8_t arbitrate : 1;
            uint8_t dma_mode : 1;
            uint8_t monitor_busy : 1;
            uint8_t enable_eop_interrupt : 1;
            uint8_t enable_parity_interrupt : 1; /* Parity checking is not emulated */
            uint8_t enable_parity_checking : 1;
            uint8_t target_mode : 1;
            uint8_t block_mode_dma : 1;
        } bits;
    } mode;
    union { /* Target Command Register */
        uint8_t raw;
        struct {
            uint8_t assert_io : 1;
            uint8_t assert_cd : 1;
            uint8_t assert_msg : 1;
            uint8_t assert_req : 1;
            uint8_t unused : 4;
        } bits;
    } tcr;
    uint8_t ser; /* Select Enable Register */
} NCR5380;

NCR5380 SCSIController;
SCSIDrive SCSIDrives[8];

int SCSIPortRead(uint32_t port, uint32_t length, uint32_t *value) {
    switch(port) {
        case 0x21: {
            *value = SCSIController.icr.raw;
        }
    }
    return 1;
}

int SCSIPortWrite(uint32_t port, uint32_t length, uint32_t value) {
    switch(port) {
        case 0x21: {
            SCSIController.icr = 
        }
        case 0x22: {
            if(value & 1) {
                SCSIController.icr.bits.aip = 1;
                SCSIController.icr.bits.la = 0;
            } else if(value & 1 == 0) {
                SCSIController.icr.bits.aip = 0;
            }
            break;
        }
    }
    return 1;
}

void SCSIInit() {
    for(int i=0x20; i<=0x27; i++) {
        OkamiPorts[i].isPresent = 1;
        OkamiPorts[i].read = SCSIPortRead;
        OkamiPorts[i].write = SCSIPortWrite;
    }
}