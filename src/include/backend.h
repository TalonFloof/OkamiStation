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
  uint32_t texCoordX;
  uint32_t texCoordY;
} EWVertex;

Backend NewBackend();
void Backend_Run(Backend *backend);

#endif