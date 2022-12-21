#ifndef _GPU_H
#define _GPU_H 1

#include <stdint.h>

typedef enum {
  GPU_NOP = 0x00,
  GPU_CLEAR_FIFO = 0x01,
  GPU_SET_FOG_INFO = 0x02,
  GPU_SET_AMBIENT_LIGHT = 0x03,
  GPU_SET_LIGHT_INFO = 0x04,
  GPU_CLEAR = 0x05,

  GPU_UPLOAD_GEOMETRY = 0x10,
  GPU_FREE_GEOMETRY = 0x11,
  GPU_RENDER_GEOMETRY = 0x12,

  GPU_UPLOAD_TEXTURE = 0x20,
  GPU_FREE_TEXTURE = 0x21,
  GPU_BIND_TEXTURE = 0x22,

  GPU_MATRIX_MODE = 0x30,
  GPU_MATRIX_IDENTITY = 0x31,
  GPU_MATRIX_PERSPECTIVE = 0x32,
  GPU_MATRIX_TRANSLATE = 0x33,
  GPU_MATRIX_ROTATE = 0x34,
  GPU_MATRIX_SCALE = 0x35,

  GPU_SWAP_BUFFERS = 0xff,
} GPUCommands;

#define XY(x, y) (((uint32_t)((uint16_t)y) << 16) | (uint32_t)((uint16_t)x))

typedef struct {
  uint32_t posXY;  // (Signed, 15-bit fractional, divides by 256)
  uint32_t posZ;   // (Signed, 15-bit fractional, divides by 256)
  uint32_t color;
  uint32_t texCoords;  // (Signed, 15-bit fractional, divides by 16384)
} Vertex;

static void GPUSendCommand(uint32_t cmd, uint32_t arg1, uint32_t arg2,
                           uint32_t arg3, uint32_t arg4) {
  while (((*((uint32_t*)0x20000001c)) & 0x4) != 0) {
  }
  ((uint32_t*)0x20000008)[1] = arg1;
  ((uint32_t*)0x20000008)[2] = arg2;
  ((uint32_t*)0x20000008)[3] = arg3;
  ((uint32_t*)0x20000008)[4] = arg4;

  ((uint32_t*)0x20000008)[0] = cmd;
}

#endif