#ifndef _GPU_H
#define _GPU_H 1

#include <stdint.h>

typedef enum {
  GPU_NOP = 0,
  GPU_CLEAR_FIFO,
  GPU_SET_FOG_INFO,
  GPU_SET_AMBIENT_LIGHT,
  GPU_SET_LIGHT_INFO,
  GPU_CLEAR,
  GPU_SET_CAMERA,
  GPU_UPLOAD_GEOMETRY,
  GPU_UPLOAD_GEOMETRY_LIGHT,
  GPU_UPLOAD_GEOMETRY_FOG,
  GPU_UPLOAD_GEOMETRY_LIGHT_FOG,
  GPU_UPLOAD_TEXTURE,
  GPU_FREE_TEXTURE,
  GPU_BIND_TEXTURE,
} GPUCommands;

typedef struct {
  uint32_t x;
  uint32_t y;
  uint32_t z;
  uint32_t color;
  uint32_t texCoordX;
  uint32_t texCoordY;
} Vertex;

static void GPUSendCommand(uint32_t cmd, uint32_t arg1, uint32_t arg2,
                           uint32_t arg3, uint32_t arg4) {
  ((uint32_t*)0x20000008)[1] = arg1;
  ((uint32_t*)0x20000008)[2] = arg2;
  ((uint32_t*)0x20000008)[3] = arg3;
  ((uint32_t*)0x20000008)[4] = arg4;

  ((uint32_t*)0x20000008)[0] = cmd;
}

#endif