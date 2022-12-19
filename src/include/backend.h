#ifndef _BACKEND_H
#define _BACKEND_H 1

#include <mini-rv32ima.h>
#include <raylib.h>

typedef struct {
  Camera3D camera;
  Shader shader;
  struct MiniRV32IMAState state;
} Backend;

Backend NewBackend();
void Backend_Run(Backend *backend);

#endif