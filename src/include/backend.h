#ifndef _BACKEND_H
#define _BACKEND_H 1

#include <raylib.h>

typedef struct {
  Camera3D camera;
  Shader shader;
} Backend;

Backend NewBackend();
void Backend_Run(Backend *backend);

#endif