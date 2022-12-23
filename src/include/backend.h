#ifndef _BACKEND_H
#define _BACKEND_H 1

#include <mini-rv32ima.h>
#include <pthread.h>
#include <raylib.h>

uint32_t HandleControlStore(uint32_t *trap, uint32_t addy, uint32_t val);
uint32_t HandleControlLoad(uint32_t *trap, uint32_t addy);

typedef struct {
  pthread_t ThreadID;
  Camera3D camera;
  Shader shader;
  struct MiniRV32IMAState state;
} Backend;

typedef struct {
  uint32_t posXY;
  uint32_t posZ;
  uint32_t color;
  uint32_t texCoords;
} EWVertex;

typedef enum {
  GPU_NOP = 0x00,
  GPU_CLEAR_FIFO = 0x01,
  GPU_SET_FOG_INFO = 0x02,
  GPU_SET_AMBIENT_LIGHT = 0x03,
  GPU_SET_LIGHT_INFO = 0x04,
  GPU_CLEAR = 0x05,

  GPU_UPLOAD_GEOMETRY = 0x10,
  GPU_UPLOAD_INDEXED_GEOMETRY = 0x11,
  GPU_FREE_GEOMETRY = 0x12,
  GPU_RENDER_GEOMETRY = 0x13,

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

Backend NewBackend();
void Backend_Run(Backend *backend);

#endif