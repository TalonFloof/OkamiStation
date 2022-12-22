#include <stdint.h>

#include "gpu.h"
#include "uart.h"

extern const char _copyright_notice;

const Vertex CUBE_VERTICES[] = {
    {XY(-256, -256),XY(-256, 0),0x3FFFF,0},
    {XY(-256, -256),XY(256, 0),0x3FFFF,0},
    {XY(-256, 256),XY(256, 0),0x3FFFF,0},

    {XY(256, 256),XY(-256, 0),0x3FFFF,0},
    {XY(-256, -256),XY(-256, 0),0x3FFFF,0},
    {XY(-256, 256),XY(-256, 0),0x3FFFF,0},

    {XY(256, -256),XY(256, 0),0x3FFFF,0},
    {XY(-256, -256),XY(-256, 0),0x3FFFF,0},
    {XY(256, -256),XY(-256, 0),0x3FFFF,0},

    {XY(256, 256),XY(-256, 0),0x3FFFF,0},
    {XY(256, -256),XY(-256, 0),0x3FFFF,0},
    {XY(-256, -256),XY(-256, 0),0x3FFFF,0},

    {XY(-256, -256),XY(-256, 0),0x3FFFF,0},
    {XY(-256, 256),XY(256, 0),0x3FFFF,0},
    {XY(-256, 256),XY(-256, 0),0x3FFFF,0},

    {XY(256, -256),XY(256, 0),0x3FFFF,0},
    {XY(-256, -256),XY(256, 0),0x3FFFF,0},
    {XY(-256, -256),XY(-256, 0),0x3FFFF,0},

    {XY(-256, 256),XY(256, 0),0x3FFFF,0},
    {XY(-256, -256),XY(256, 0),0x3FFFF,0},
    {XY(256, -256),XY(256, 0),0x3FFFF,0},

    {XY(256, 256),XY(256, 0),0x3FFFF,0},
    {XY(256, -256),XY(-256, 0),0x3FFFF,0},
    {XY(256, 256),XY(-256, 0),0x3FFFF,0},

    {XY(256, -256),XY(-256, 0),0x3FFFF,0},
    {XY(256, 256),XY(256, 0),0x3FFFF,0},
    {XY(256, -256),XY(256, 0),0x3FFFF,0},

    {XY(256, 256),XY(256, 0),0x3FFFF,0},
    {XY(256, 256),XY(-256, 0),0x3FFFF,0},
    {XY(-256, 256),XY(-256, 0),0x3FFFF,0},

    {XY(256, 256),XY(256, 0),0x3FFFF,0},
    {XY(-256, 256),XY(-256, 0),0x3FFFF,0},
    {XY(-256, 256),XY(256, 0),0x3FFFF,0},

    {XY(256, 256),XY(256, 0),0x3FFFF,0},
    {XY(-256, 256),XY(256, 0),0x3FFFF,0},
    {XY(256, -256),XY(256, 0),0x3FFFF,0},
};

void EWFirmwareStart() {
  GPUSendCommand(GPU_CLEAR_FIFO, 0, 0, 0, 0);  // Initialize the GPU
  GPUSendCommand(GPU_SET_AMBIENT_LIGHT, 0x398C6, 0, 0, 0);
  GPUSendCommand(GPU_SET_LIGHT_INFO, 1, 0x3FFFF, XY(0,0), XY(512,65535));
  // Setup the Projection Matrix
  GPUSendCommand(GPU_MATRIX_MODE, 0, 0, 0, 0);
  GPUSendCommand(GPU_MATRIX_PERSPECTIVE, 1, 20, 60, 0);
  // Setup the Geometry Matrix
  GPUSendCommand(GPU_MATRIX_MODE, 1, 0, 0, 0);
  GPUSendCommand(GPU_MATRIX_IDENTITY, 0, 0, 0, 0);
  // Setup the View Matrix
  GPUSendCommand(GPU_MATRIX_MODE, 2, 0, 0, 0);
  GPUSendCommand(GPU_MATRIX_IDENTITY, 0, 0, 0, 0);
  GPUSendCommand(GPU_MATRIX_TRANSLATE, XY(0, 0), (uint32_t)((uint16_t)-512),
                 0, 0);
  // Upload the Geometry
  GPUSendCommand(GPU_UPLOAD_GEOMETRY, 1, 0, 36, ((uint32_t)&CUBE_VERTICES));
  UART_PutString(&_copyright_notice);
  int i=0;
  uint32_t prevFrame = GPUCurrentFrame();
  for (;;) {
    while(prevFrame == GPUCurrentFrame()) {}
    prevFrame = GPUCurrentFrame();
    GPUSendCommand(GPU_CLEAR, 0x30000, 0, 0, 0);
    GPUSendCommand(GPU_MATRIX_MODE, 1, 0, 0, 0);
    GPUSendCommand(GPU_MATRIX_IDENTITY, 0, 0, 0, 0);
    GPUSendCommand(GPU_MATRIX_ROTATE, XY(i, 0), XY(i,0), 0, 0);
    GPUSendCommand(GPU_RENDER_GEOMETRY, 1, 0, 0, 0);
    GPUSendCommand(GPU_SWAP_BUFFERS, 0, 0, 0, 0);
    i = (i + 125);
  }
}