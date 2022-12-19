#include <float.h>
#include <math.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#define MINI_RV32_RAM_SIZE (4 * 1024 * 1024)
#define MINIRV32_RAM_IMAGE_OFFSET 0x00000000
#define MINIRV32WARN(x...) fprintf(stderr, x);
#define MINIRV32_IMPLEMENTATION
#include <backend.h>
#include <rlgl.h>

extern char execPath[2048];

const int WIDTH = 256;
const int HEIGHT = 192;

const float RATIO = (float)WIDTH / (float)HEIGHT;

const int CYCLES = (33 * 1000 * 1000); /* 33 MHz Clock Speed */

uint8_t RAM[4 * 1024 * 1024];

Backend NewBackend() {
  Camera3D camera = {0};
  camera.position = (Vector3){10.0f, 10.0f, 10.0f};
  camera.target = (Vector3){0.0f, 0.0f, 0.0f};
  camera.up = (Vector3){0.0f, 0.5f, 0.0f};
  camera.fovy = 45.0f;
  camera.projection = CAMERA_PERSPECTIVE;
  return (Backend){.camera = camera};
}

void Backend_SetFog(Backend *backend, float depth[3], Color color) {
  float col[4] = {0.0, 0.0, 0.0, 0.0};
  col[0] = ((float)color.r) / 255;
  col[1] = ((float)color.g) / 255;
  col[2] = ((float)color.b) / 255;
  col[3] = ((float)color.a) / 255;
  int fogDepthLoc = GetShaderLocation(backend->shader, "u_fogDepth");
  int fogColorLoc = GetShaderLocation(backend->shader, "u_fogColor");
  SetShaderValue(backend->shader, fogDepthLoc, depth, SHADER_UNIFORM_VEC3);
  SetShaderValue(backend->shader, fogColorLoc, col, SHADER_UNIFORM_VEC4);
}

void Backend_SetLight(Backend *backend, int lightID, int enabled, Color color,
                      float pos[3], float steps) {
  Vector4 col = ColorNormalize(color);
  col.w = enabled;
  int loc = GetShaderLocation(backend->shader,
                              TextFormat("u_lightColor[%i]", lightID));
  SetShaderValue(backend->shader, loc, &col, SHADER_UNIFORM_VEC4);
  loc = GetShaderLocation(backend->shader,
                          TextFormat("u_lightPosition[%i]", lightID));
  SetShaderValue(backend->shader, loc, pos, SHADER_UNIFORM_VEC3);
  loc = GetShaderLocation(backend->shader,
                          TextFormat("u_lightSteps[%i]", lightID));
  SetShaderValue(backend->shader, loc, (float[2]){steps, 0},
                 SHADER_UNIFORM_VEC2);
}

static float ips = 0;
static atomic_bool backendReady = 0;

uint64_t GetUsTimestamp() {
  struct timeval tv;
  gettimeofday(&tv, 0);
  return tv.tv_usec + ((uint64_t)(tv.tv_sec)) * 1000000LL;
}

void *RISCVProcThread(Backend *backend) {
  while (!atomic_load(&backendReady)) {
    // Spinloop
  }
  for (int i = 0; i < 32; i++) {
    backend->state.regs[i] = 0;
  }
  backend->state.pc = 0xf0000000;
  backend->state.extraflags |= 3;
  while (1) {
    uint64_t beginTime = GetUsTimestamp();
    uint64_t prevTime = GetUsTimestamp();
    for (int i = 0; i < CYCLES; i += 10000) {
      uint64_t elapsedUs = GetUsTimestamp() / 1 - prevTime;
      prevTime += elapsedUs;
      int ret = MiniRV32IMAStep(&(backend->state), RAM, 0, (uint32_t)elapsedUs,
                                10000);
      switch (ret) {
        case 0:
          break;
        case 1:
          break;
        case 3:
          break;
        default:
          fprintf(stderr, "Unknown failure\n");
          abort();
          break;
      }
      ips += 10000;
    }
    if (GetUsTimestamp() < (beginTime + 1000000))
      usleep((beginTime + 1000000) - GetUsTimestamp());
  }
  pthread_exit(NULL);
  return NULL;
}

void Backend_Run(Backend *backend) {
  /* LOAD FIRMWARE */
  // LoadFileData();
  pthread_t processorThreadID;
  pthread_create(&processorThreadID, NULL, (void *(*)(void *))RISCVProcThread,
                 backend);
  SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_TRANSPARENT);
  InitWindow(WIDTH * 3, HEIGHT * 3, TextFormat("EmberWolf"));
  SetTargetFPS(60);

  RenderTexture2D framebuffer = LoadRenderTexture(WIDTH, HEIGHT);

  Camera2D fbCamera = {0};  // Camera of the 2D Framebuffer
  fbCamera.zoom = 1.0;

  Rectangle fbSourceRec = {0.0f, 0.0f, (float)framebuffer.texture.width,
                           -(float)framebuffer.texture.height};
  Rectangle fbDestRec = {-RATIO, -RATIO, (WIDTH * 3) + (RATIO * 2),
                         (HEIGHT * 3) + (RATIO * 2)};

  Vector2 fbOrigin = {0.0f, 0.0f};

  backend->shader =
      LoadShader(TextFormat("%s/resources/shaders/base.vs", execPath),
                 TextFormat("%s/resources/shaders/base.fs", execPath));
  backend->shader.locs[SHADER_LOC_VECTOR_VIEW] =
      GetShaderLocation(backend->shader, "u_viewPos");
  backend->shader.locs[SHADER_LOC_MATRIX_MODEL] =
      GetShaderLocation(backend->shader, "matModel");

  SetCameraMode(backend->camera, CAMERA_PERSPECTIVE);

  double prevSecond = GetTime();
  atomic_store(&backendReady, 1);
  while (!WindowShouldClose()) {
    UpdateCamera(&(backend->camera));

    SetShaderValue(
        backend->shader, backend->shader.locs[SHADER_LOC_VECTOR_VIEW],
        &((float[3]){backend->camera.position.x, backend->camera.position.y,
                     backend->camera.position.z}),
        SHADER_UNIFORM_VEC3);

    BeginTextureMode(framebuffer);
    BeginMode3D(backend->camera);
    // GPU Rendering goes here.
    EndMode3D();
    EndTextureMode();

    BeginDrawing();
    BeginMode2D(fbCamera);
    DrawTexturePro(framebuffer.texture, fbSourceRec, fbDestRec, fbOrigin, 0.0f,
                   WHITE);
    EndMode2D();
    EndDrawing();
    // BEGIN CPU CYCLE EXECUTION
    if (floor(prevSecond) != floor(GetTime())) {
      SetWindowTitle(TextFormat("EmberWolf - Built on %s (%.2f MIPS, %i FPS)",
                                __TIMESTAMP__, ips / 1000000, GetFPS()));
      ips = 0;
      prevSecond = GetTime();
    }
  }

  UnloadShader(backend->shader);
  UnloadTexture(framebuffer.texture);

  CloseWindow();
}
