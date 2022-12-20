#include <stdint.h>

#include "gpu.h"
#include "uart.h"

void EWFirmwareStart() {
  UART_PutString(
      "EmberWolf Firmware, Copyright (C) 2022-2023 TalonFox.\nLicensed under "
      "the MIT License, see the LICENSE file for more information.\n");
  GPUSendCommand(GPU_CLEAR_FIFO, 0, 0, 0, 0);  // Initialize the GPU
  GPUSendCommand(GPU_CLEAR, 0x38000, 0, 0, 0);
  for (;;) {  // WE CANNOT ESCAPE!
  }
}