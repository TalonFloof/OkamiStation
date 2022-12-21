#include <stdint.h>

#include "gpu.h"
#include "uart.h"

extern const char _copyright_notice;

const Vertex TRIANGLE_VERTICES[] = {
    {
        .posXY = XY(256, 256),
        .posZ = 0,
        .color = 0x3FC00,
        .texCoords = 0,
    },
    {
        .posXY = XY(256, 512),
        .posZ = 0,
        .color = 0x383E0,
        .texCoords = 0,
    },
    {
        .posXY = XY(512, 512),
        .posZ = 0,
        .color = 0x3801F,
        .texCoords = 0,
    },
};

void EWFirmwareStart() {
  UART_PutString(&_copyright_notice);
  GPUSendCommand(GPU_CLEAR_FIFO, 0, 0, 0, 0);  // Initialize the GPU
  GPUSendCommand(GPU_CLEAR, 0x38000, 0, 0, 0);
  // Setup the Projection Matrix
  GPUSendCommand(GPU_MATRIX_MODE, 0, 0, 0, 0);
  GPUSendCommand(GPU_MATRIX_PERSPECTIVE, 1, 20, 60, 0);
  GPUSendCommand(GPU_MATRIX_ROTATE, XY(0, 64), 0, 0, 0);
  // Setup the Geometry Matrix
  GPUSendCommand(GPU_MATRIX_MODE, 1, 0, 0, 0);
  GPUSendCommand(GPU_MATRIX_IDENTITY, 0, 0, 0, 0);
  // Setup the View Matrix
  GPUSendCommand(GPU_MATRIX_MODE, 2, 0, 0, 0);
  GPUSendCommand(GPU_MATRIX_IDENTITY, 0, 0, 0, 0);
  GPUSendCommand(GPU_MATRIX_TRANSLATE, XY(-192, -192),
                 (uint32_t)((uint16_t)-256), 0, 0);  // (-1.5,-1.5,-2)
  // Upload and Render the Geometry
  GPUSendCommand(GPU_UPLOAD_GEOMETRY, 1, 0, 3, ((uint32_t)&TRIANGLE_VERTICES));
  GPUSendCommand(GPU_RENDER_GEOMETRY, 1, 0, 0, 0);
  // Render onto the Screen!
  GPUSendCommand(GPU_SWAP_BUFFERS, 0, 0, 0, 0);
  for (;;) {  // WE CANNOT ESCAPE!
  }
}