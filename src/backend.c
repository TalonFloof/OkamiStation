#include <stdint.h>
uint32_t HandleControlStore(uint32_t *trap, uint32_t addy, uint32_t val);
uint32_t HandleControlLoad(uint32_t *trap, uint32_t addy);

#include <fifo.h>
#include <float.h>
#include <math.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#define MINI_RV32_RAM_SIZE (4 * 1024 * 1024)
#define MINIRV32_HANDLE_MEM_STORE_CONTROL(t, addy, val) \
  if (HandleControlStore(t, addy, val)) return val;
#define MINIRV32_HANDLE_MEM_LOAD_CONTROL(t, addy, rval) \
  rval = HandleControlLoad(t, addy);
#define MINIRV32_RAM_IMAGE_OFFSET 0x00000000
#define MINIRV32WARN(x...) fprintf(stderr, x);
#define MINIRV32_IMPLEMENTATION
#include <backend.h>
#include <rlgl.h>

extern char execPath[2048];

const int WIDTH = 256;
const int HEIGHT = 192;

const float RATIO = (float)WIDTH / (float)HEIGHT;

const int CYCLES = 33868000; /* 33 MHz Clock Speed */

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
static uint8_t *ROMData = NULL;
static unsigned int ROMSize = 0;

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
      if (GetUsTimestamp() >= (beginTime + 1000000)) break;
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

static FIFOQueue GPUCmdFIFO;

Color EWColorToRLColor(uint32_t color) {
  return (Color){
      .r = (uint8_t)(((double)((color & (0b11111 << 10)) >> 10) / 31) * 255),
      .g = (uint8_t)(((double)((color & (0b11111 << 5)) >> 5) / 31) * 255),
      .b = (uint8_t)(((double)((color & 0b11111)) / 31) * 255),
      .a = (uint8_t)(((double)((color & (0b111 << 15)) >> 15) / 7) * 255),
  };
}

void Backend_Run(Backend *backend) {
  /* INITIALIZE GPU COMMAND FIFO */
  GPUCmdFIFO.mutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
  GPUCmdFIFO.size = 256;
  GPUCmdFIFO.data = malloc(256 * sizeof(uintptr_t));
  /* LOAD FIRMWARE */
  ROMData = LoadFileData(TextFormat("%s/resources/EmberWolfFirmware", execPath),
                         &ROMSize);
  if (ROMData == NULL) {
    fprintf(stderr, "Couldn't find firmware file!\n");
    return;
  }
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
  uint32_t *cmdData;
  while (!WindowShouldClose()) {
    // Check FIFO Buffer
    BeginDrawing();
    while ((cmdData = FIFORead(&GPUCmdFIFO)) != NULL) {
      switch (cmdData[0]) {
        case 0x00:
          break;
        case 0x05:
          BeginTextureMode(framebuffer);
          ClearBackground(EWColorToRLColor(cmdData[1]));
          BeginMode3D(backend->camera);
          EndMode3D();
          EndTextureMode();
          break;
        case 0x07:
        case 0x08:
        case 0x09:
        case 0x0a: {
          int enableLight = ((cmdData[0] == 0x08) || (cmdData[0] == 0x0a));
          int enableFog = ((cmdData[0] == 0x09) || (cmdData[0] == 0x0a));
          // Construct Mesh
          EWVertex *verptr =
              ((cmdData[3] > 0xf0000000) ? ROMData[cmdData[3] - 0xf0000000]
                                         : RAM[cmdData[3]]);
          Mesh mesh = {0};
          mesh.triangleCount = cmdData[2] / 3;
          mesh.vertexCount = mesh.triangleCount * 3;
          mesh.vertices =
              (float *)MemAlloc(mesh.vertexCount * 3 * sizeof(float));
          mesh.texcoords =
              (float *)MemAlloc(mesh.vertexCount * 2 * sizeof(float));
          mesh.normals =
              (float *)MemAlloc(mesh.vertexCount * 3 * sizeof(float));
          for (int i = 0; i < cmdData[2]; i++) {
            EWVertex *ver = verptr + i;
            mesh.vertices[(i * 3) + 0] = (((float)ver->x) / 1000000);
            mesh.vertices[(i * 3) + 1] = (((float)ver->y) / 1000000);
            mesh.vertices[(i * 3) + 2] = (((float)ver->z) / 1000000);
          }
          break;
        }
        default:
          fprintf(stderr,
                  "\x1b[1;31mRECEIVED UNKNOWN GPU COMMAND: 0x%08x (0x%08x, "
                  "0x%08x, 0x%08x, 0x%08x)!\x1b[0m\n",
                  cmdData[0], cmdData[1], cmdData[2], cmdData[3], cmdData[4]);
          break;
      }
      free(cmdData);
    }
    SetShaderValue(
        backend->shader, backend->shader.locs[SHADER_LOC_VECTOR_VIEW],
        &((float[3]){backend->camera.position.x, backend->camera.position.y,
                     backend->camera.position.z}),
        SHADER_UNIFORM_VEC3);
    BeginMode2D(fbCamera);
    DrawTexturePro(framebuffer.texture, fbSourceRec, fbDestRec, fbOrigin, 0.0f,
                   WHITE);
    EndMode2D();
    EndDrawing();
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

static uint32_t GPUMsg[5];

uint32_t HandleControlStore(uint32_t *trap, uint32_t addy, uint32_t val) {
  if (addy == 0x11000000) {  // Debug UART
    fprintf(stderr, "%c", val);
  } else if (addy >= 0x20000008 && addy <= 0x2000001b) {  // GPU Command FIFO
    GPUMsg[(addy - 0x20000008) / 4] = val;
    if (addy == 0x20000008) {
      if (val == 1) {
        // Clear the FIFO
        pthread_mutex_lock(&(GPUCmdFIFO.mutex));
        for (int i = GPUCmdFIFO.head; i < GPUCmdFIFO.tail; i++) {
          free(GPUCmdFIFO.data[i]);
        }
        GPUCmdFIFO.head = 0;
        GPUCmdFIFO.tail = 0;
        pthread_mutex_unlock(&(GPUCmdFIFO.mutex));
      } else {
        uint32_t *buf = calloc(5, sizeof(uint32_t));
        memcpy(buf, (void *)&GPUMsg, sizeof(uint32_t) * 5);
        for (int i = 0; i < 5; i++) {
          GPUMsg[i] = 0;
        }
        if (FIFOWrite(&GPUCmdFIFO, buf) == -1) {
          fprintf(stderr, "Failed to push command to buffer!\n");
        }
      }
    }
  } else {
    *trap = (7 + 1);
  }
  return 0;
}

const char *GPUMagicStr = "EMBRGPU \0\0\0";

uint32_t HandleControlLoad(uint32_t *trap, uint32_t addy) {
  if (addy >= 0xf0000000 && addy <= (0xf0000000 + ROMSize)) {  // ROM
    return *((uint32_t *)(ROMData + (addy - 0xf0000000)));
  } else if (addy >= 0x20000000 && addy <= 0x2000008) {  // GPU Magic Number
    return *((uint32_t *)(GPUMagicStr + (addy - 0x20000000)));
  } else {
    *trap = (5 + 1);
  }
  return 0;
}

/*
        if (m_normalsModeState == ENormalsMode::Flat)
        {
            for (size_t i = 0; i < m_vertexState.count; i+=3)
            {
                Vec3f v1 = m_vertexState[i+1].pos - m_vertexState[i].pos;
                Vec3f v2 = m_vertexState[i+2].pos - m_vertexState[i].pos;
                Vec3f faceNormal = Vec3f::Cross(v1, v2).GetNormalized();

                m_vertexState[i].norm = faceNormal;
                m_vertexState[i+1].norm = faceNormal;
                m_vertexState[i+2].norm = faceNormal;
            }

            // create vertex buffer
            uint32_t numVertices = (uint32_t)m_vertexState.count;
            if (numVertices != bgfx::getAvailTransientVertexBuffer(numVertices,
   m_layout) ) return; bgfx::allocTransientVertexBuffer(&vertexBuffer,
   numVertices, m_layout); VertexData* verts = (VertexData*)vertexBuffer.data;
            bx::memCopy(verts, m_vertexState.pData, numVertices *
   sizeof(VertexData) );
*/
