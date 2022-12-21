#include <stdint.h>

#include "gpu.h"
#include "uart.h"

extern const char _copyright_notice;

void EWFirmwareStart() {
  UART_PutString(&_copyright_notice);
  GPUSendCommand(GPU_CLEAR_FIFO, 0, 0, 0, 0);  // Initialize the GPU
  GPUSendCommand(GPU_CLEAR, 0x38000, 0, 0, 0);
  GPUSendCommand(GPU_SWAP_BUFFERS, 0, 0, 0, 0);
  for (;;) {  // WE CANNOT ESCAPE!
  }
}