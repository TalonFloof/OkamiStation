#include <stdint.h>

#include "gpu.h"
#include "uart.h"
#include "glassCubeTexture.h"

extern const char _copyright_notice;

const Vertex CUBE_VERTICES[] = {
    VERTEX(-256,-256,-256,0x3FFFF,0,0),
    VERTEX(256,-256,-256,0x3FFFF,16384,0),
    VERTEX(256,256,-256,0x3FFFF,16384,16384),
    VERTEX(256,256,-256,0x3FFFF,16384,16384),
    VERTEX(-256,256,-256,0x3FFFF,0,16384),
    VERTEX(-256,-256,-256,0x3FFFF,0,0),

    VERTEX(-256,-256,256,0x3FFFF,0,0),
    VERTEX(256,-256,256,0x3FFFF,16384,0),
    VERTEX(256,256,256,0x3FFFF,16384,16384),
    VERTEX(256,256,256,0x3FFFF,16384,16384),
    VERTEX(-256,256,256,0x3FFFF,0,16384),
    VERTEX(-256,-256,256,0x3FFFF,0,0),

    VERTEX(-256,256,256,0x3FFFF,16384,0),
    VERTEX(-256,256,-256,0x3FFFF,16384,16384),
    VERTEX(-256,-256,-256,0x3FFFF,0,16384),
    VERTEX(-256,-256,-256,0x3FFFF,0,16384),
    VERTEX(-256,-256,256,0x3FFFF,0,0),
    VERTEX(-256,256,256,0x3FFFF,16384,0),

    VERTEX(256,256,256,0x3FFFF,16384,0),
    VERTEX(256,256,-256,0x3FFFF,16384,16384),
    VERTEX(256,-256,-256,0x3FFFF,0,16384),
    VERTEX(256,-256,-256,0x3FFFF,0,16384),
    VERTEX(256,-256,256,0x3FFFF,0,0),
    VERTEX(256,256,256,0x3FFFF,16384,0),

    VERTEX(-256,-256,-256,0x3FFFF,0,16384),
    VERTEX(256,-256,-256,0x3FFFF,16384,16384),
    VERTEX(256,-256,256,0x3FFFF,16384,0),
    VERTEX(256,-256,256,0x3FFFF,16384,0),
    VERTEX(-256,-256,256,0x3FFFF,0,0),
    VERTEX(-256,-256,-256,0x3FFFF,0,16384),

    VERTEX(-256,256,-256,0x3FFFF,0,16384),
    VERTEX(256,256,-256,0x3FFFF,16384,16384),
    VERTEX(256,256,256,0x3FFFF,16384,0),
    VERTEX(256,256,256,0x3FFFF,16384,0),
    VERTEX(-256,256,256,0x3FFFF,0,0),
    VERTEX(-256,256,-256,0x3FFFF,0,16384),
};

void EWFirmwareStart() {
  GPUSendCommand(GPU_CLEAR_FIFO, 0, 0, 0, 0);  // Initialize the GPU
  //GPUSendCommand(GPU_SET_FOG_INFO, 1, XY(5*32,10*32), 0x38421, 0);
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
  GPUSendCommand(GPU_UPLOAD_GEOMETRY, XY(1, 0), 36, ((uint32_t)&CUBE_VERTICES), 0);
  // Upload the Texture
  GPUSendCommand(GPU_UPLOAD_TEXTURE, XY(2, 1), XY(128,128), ((uint32_t)&glassCubeTexture_bin), 0);
  GPUSendCommand(GPU_BIND_TEXTURE, 1, 1, 0, 0);
  UART_PutString(&_copyright_notice);
  int i=0;
  int j=0;
  uint32_t prevFrame = GPUCurrentFrame();
  for (;;) {
    while(prevFrame == GPUCurrentFrame()) {}
    prevFrame = GPUCurrentFrame();
    GPUSendCommand(GPU_CLEAR, 0x38000, 0, 0, 0);
    GPUSendCommand(GPU_MATRIX_MODE, 2, 0, 0, 0);
    GPUSendCommand(GPU_MATRIX_IDENTITY, 0, 0, 0, 0);
    GPUSendCommand(GPU_MATRIX_ROTATE, XY(0, 0), XY(0,0), 0, 0);
    GPUSendCommand(GPU_MATRIX_TRANSLATE, XY(0, 0), (uint32_t)((uint16_t)-512),
                 0, 0);
    GPUSendCommand(GPU_MATRIX_MODE, 1, 0, 0, 0);
    GPUSendCommand(GPU_MATRIX_IDENTITY, 0, 0, 0, 0);
    GPUSendCommand(GPU_MATRIX_ROTATE, XY(i, 0), XY(i,0), 0, 0);
    GPUSendCommand(GPU_RENDER_GEOMETRY, 1, 0x1FFFF, 0, 0);
    GPUSendCommand(GPU_MATRIX_IDENTITY, 0, 0, 0, 0);
    GPUSendCommand(GPU_MATRIX_TRANSLATE, XY(0, 0), (uint32_t)((uint16_t)1024),
                 0, 0);
    GPUSendCommand(GPU_MATRIX_ROTATE, XY(i, 0), XY(i,0), 0, 0);
    GPUSendCommand(GPU_RENDER_GEOMETRY, 1, 0x1FFFF, 0, 0);
    GPUSendCommand(GPU_MATRIX_IDENTITY, 0, 0, 0, 0);
    GPUSendCommand(GPU_MATRIX_TRANSLATE, XY(512, 0), (uint32_t)((uint16_t)512),
                 0, 0);
    GPUSendCommand(GPU_MATRIX_ROTATE, XY(i, 0), XY(i,0), 0, 0);
    GPUSendCommand(GPU_RENDER_GEOMETRY, 1, 0x1FFFF, 0, 0);
    GPUSendCommand(GPU_MATRIX_IDENTITY, 0, 0, 0, 0);
    GPUSendCommand(GPU_MATRIX_TRANSLATE, XY(-512, 0), (uint32_t)((uint16_t)512),
                 0, 0);
    GPUSendCommand(GPU_MATRIX_ROTATE, XY(i, 0), XY(i,0), 0, 0);
    GPUSendCommand(GPU_RENDER_GEOMETRY, 1, 0x1FFFF, 0, 0);
    GPUSendCommand(GPU_SWAP_BUFFERS, 0, 0, 0, 0);
    i = (i + 125);
    j = (j + (125/2));
  }
}