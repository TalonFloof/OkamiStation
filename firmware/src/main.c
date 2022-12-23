#include <stdint.h>

#include "gpu.h"
#include "uart.h"
#include "glassCubeTexture.h"

extern const char _copyright_notice;

const Vertex CUBE_VERTICES[] = {
    VERTEX(-256,-256,256,0x1FFFF,0,8192),
    VERTEX(256,-256,256,0x1FFFF,8192,8192),
    VERTEX(256,256,256,0x1FFFF,8192,16384),
    VERTEX(-256,256,256,0x1FFFF,0,16384),

    VERTEX(-256,-256,-256,0x1FFFF,8192,0),
    VERTEX(-256,256,-256,0x1FFFF,16384,0),
    VERTEX(256,256,-256,0x1FFFF,16384,8192),
    VERTEX(256,-256,-256,0x1FFFF,8192,8192),

    VERTEX(-256,256,-256,0x1FFFF,0,8192),
    VERTEX(-256,256,256,0x1FFFF,0,0),
    VERTEX(256,256,256,0x1FFFF,8192,0),
    VERTEX(256,256,-256,0x1FFFF,8192,8192),
    
    VERTEX(-256,-256,-256,0x1FFFF,8192,8192),
    VERTEX(256,-256,-256,0x1FFFF,0,8192),
    VERTEX(256,-256,256,0x1FFFF,0,0),
    VERTEX(-256,-256,256,0x1FFFF,8192,0),

    VERTEX(256,-256,-256,0x1FFFF,8192,8192),
    VERTEX(256,256,-256,0x1FFFF,0,8192),
    VERTEX(256,256,256,0x1FFFF,8192,0),
    VERTEX(256,-256,256,0x1FFFF,0,0),

    VERTEX(-256,-256,-256,0x1FFFF,0,8192),
    VERTEX(-256,-256,256,0x1FFFF,0,0),
    VERTEX(-256,256,256,0x1FFFF,8192,0),
    VERTEX(-256,256,-256,0x1FFFF,8192,8192),
};

const uint16_t CUBE_INDICES[] = {
  // Front face
    0, 1, 2, 0, 2, 3,
    // Top Face
    4, 5, 6, 4, 6, 7,
    // Right face
    8, 9,10, 8,10,11,
    // Left face
    12, 13, 14, 12, 14, 15,
    // Bottom face
    16, 17, 18, 16, 18, 19,
    // Back face
    20, 21, 22, 20, 22, 23,
};

void EWFirmwareStart() {
  GPUSendCommand(GPU_CLEAR_FIFO, 0, 0, 0, 0);  // Initialize the GPU
  GPUSendCommand(GPU_SET_AMBIENT_LIGHT, 0x38421, 0, 0, 0);
  GPUSendCommand(GPU_SET_LIGHT_INFO, 0, 0x3FFFF, XY(-512,-512), XY(-512,2));
  // Setup the Projection Matrix
  GPUSendCommand(GPU_MATRIX_MODE, 0, 0, 0, 0);
  GPUSendCommand(GPU_MATRIX_PERSPECTIVE, 1, 10000, 60, 0);
  // Setup the Geometry Matrix
  GPUSendCommand(GPU_MATRIX_MODE, 1, 0, 0, 0);
  GPUSendCommand(GPU_MATRIX_IDENTITY, 0, 0, 0, 0);
  // Setup the View Matrix
  GPUSendCommand(GPU_MATRIX_MODE, 2, 0, 0, 0);
  GPUSendCommand(GPU_MATRIX_IDENTITY, 0, 0, 0, 0);
  GPUSendCommand(GPU_MATRIX_TRANSLATE, XY(0, 0), (uint32_t)((uint16_t)-512),
                 0, 0);
  // Upload the Geometry
  GPUSendCommand(GPU_UPLOAD_INDEXED_GEOMETRY, XY(1, 0), XY(36,24), ((uint32_t)&CUBE_VERTICES), ((uint32_t)&CUBE_INDICES));
  // Upload the Texture
  GPUSendCommand(GPU_UPLOAD_TEXTURE, XY(1, 1), XY(256,256), ((uint32_t)&GLASS_CUBE_TEXTURE), ((uint32_t)&GLASS_CUBE_PALETTE));
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
    GPUSendCommand(GPU_MATRIX_ROTATE, XY(16384, 0), XY(i,0), 0, 0);
    GPUSendCommand(GPU_RENDER_GEOMETRY, 1, 0x3FFFF, 0, 0);
    GPUSendCommand(GPU_SWAP_BUFFERS, 0, 0, 0, 0);
    i = (i + 125);
    j = (j + (125/2));
  }
}