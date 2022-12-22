#include <stdint.h>
uint32_t HandleControlStore(uint32_t *trap, uint32_t addy, uint32_t val);
uint32_t HandleControlLoad(uint32_t *trap, uint32_t addy);

#include <fifo.h>
#include <float.h>
#include <map.h>
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
#include <raymath.h>
#include <rlgl.h>

extern char execPath[2048];

const int WIDTH = 256;
const int HEIGHT = 192;

const float RATIO = (float)WIDTH / (float)HEIGHT;

const int CYCLES = 33868000; /* 33 MHz Clock Speed */

uint8_t RAM[4 * 1024 * 1024];

Backend NewBackend() {
  Camera3D camera = {0};
  camera.position = (Vector3){5.0f, 5.0f, 5.0f};
  camera.target = (Vector3){0.0f, 0.0f, 0.0f};
  camera.up = (Vector3){0.0f, 1.0f, 0.0f};
  camera.fovy = 60.0f;
  camera.projection = CAMERA_PERSPECTIVE;
  return (Backend){.camera = camera};
}

void Backend_SetFog(Backend *backend, float depth[3], Color color) {
  Vector4 col = ColorNormalize(color);
  int fogDepthLoc = GetShaderLocation(backend->shader, "u_fogDepth");
  int fogColorLoc = GetShaderLocation(backend->shader, "u_fogColor");
  SetShaderValue(backend->shader, fogDepthLoc, depth, SHADER_UNIFORM_VEC3);
  SetShaderValue(backend->shader, fogColorLoc, &col, SHADER_UNIFORM_VEC4);
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
static uint32_t frameCount = 0;

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
    for (int i = 0; i < CYCLES / 60; i += 10000) {
      if (GetUsTimestamp() >= (beginTime + (1000000 / 60))) break;
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
    if (GetUsTimestamp() < (beginTime + (1000000 / 60)))
      usleep((beginTime + (1000000 / 60)) - GetUsTimestamp());
    frameCount++;
  }
  pthread_exit(NULL);
  return NULL;
}

static FIFOQueue GPUCmdFIFO;

static uint32_t GPUFlags;

Color EWColorToRLColor(uint32_t color) {
  return (Color){
      .r = (uint8_t)(((double)((color & (0b11111 << 10)) >> 10) / 31) * 255),
      .g = (uint8_t)(((double)((color & (0b11111 << 5)) >> 5) / 31) * 255),
      .b = (uint8_t)(((double)((color & 0b11111)) / 31) * 255),
      .a = (uint8_t)(((double)((color & (0b111 << 15)) >> 15) / 7) * 255),
  };
}

float EWVertex16ToFloat(uint16_t num) { return ((float)((int16_t)num)) / 256; }

float EWTexCoord16ToFloat(uint16_t num) {
  return ((float)((int16_t)num)) / 16384;
}

float EWMatrixFloat16ToFloat(uint16_t num) {
  return ((float)((int16_t)num)) / 128;
}

float EWMatrixRot16ToFloat(uint16_t num) {
  return ((float)((int16_t)num)) / 32768;
}

void PrintMatrix(Matrix matrix) {
  for (int y = 0; y < 4; y++) {
    fprintf(stderr, "| ");
    for (int x = 0; x < 4; x++) {
      fprintf(stderr, "%.2f ", (((float *)&matrix))[y * 4 + x]);
    }
    fprintf(stderr, "|\n");
  }
  fprintf(stderr, "\n");
}

typedef map_t(Model) ModelMap;

void EWBeginMode3D(Matrix proj, Matrix view) {
  rlDrawRenderBatchActive();  // Update and draw internal render batch

  rlMatrixMode(RL_PROJECTION);  // Switch to projection matrix
  rlPushMatrix();  // Save previous matrix, which contains the settings for the
                   // 2d ortho projection
  rlLoadIdentity();  // Reset current matrix (projection)
  rlMultMatrixf(MatrixToFloat(proj));

  rlMatrixMode(RL_MODELVIEW);  // Switch back to modelview matrix
  rlLoadIdentity();            // Reset current matrix (modelview)

  rlMultMatrixf(MatrixToFloat(
      view));  // Multiply modelview matrix by view matrix (camera)

  rlEnableDepthTest();  // Enable DEPTH_TEST for 3D
}

void Backend_Run(Backend *backend) {
  /* INITIALIZE GPU COMMAND FIFO */
  GPUCmdFIFO.isClearing = 0;
  GPUCmdFIFO.size = 256;
  GPUCmdFIFO.data = malloc(256 * sizeof(uintptr_t));
  /* INITIALIZE GPU GEOMETRY CACHE */
  ModelMap GPUGeometryCache;
  int GPUVertexCount = 0;
  map_init(&GPUGeometryCache);
  /* INITIALIZE GPU MATRICES */
  Matrix GPUMatrices[3];
  int GPUActiveMatrix = 0;
  Vector3 GPUViewPos = Vector3Zero();
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
  SetConfigFlags(FLAG_WINDOW_TRANSPARENT);
  InitWindow(WIDTH * 3, HEIGHT * 3, TextFormat("EmberWolf"));

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

  double prevSecond = GetTime();
  atomic_store(&backendReady, 1);
  uint32_t *cmdData;

  SetCameraMode(backend->camera, CAMERA_FREE);

  while (!WindowShouldClose()) {
    UpdateCamera(&backend->camera);
    // Check FIFO Buffer
    BeginDrawing();
    while ((cmdData = FIFORead(&GPUCmdFIFO)) != NULL) {
      switch (cmdData[0]) {
        case GPU_NOP:
          break;
        case GPU_SET_FOG_INFO:
          break;
        case GPU_SET_AMBIENT_LIGHT:
          Color col = EWColorToRLColor(cmdData[1]);
          int loc = GetShaderLocation(backend->shader, "u_lightAmbient");
          float finalCol[4] = {((float)col.r) / 255, ((float)col.g) / 255, ((float)col.b) / 255, ((float)col.a) / 255};
          SetShaderValue(backend->shader, loc, &finalCol, SHADER_UNIFORM_VEC4);
          break;
        case GPU_SET_LIGHT_INFO:
          float pos[3] = {0,0,0};
          pos[0] = EWMatrixFloat16ToFloat((uint16_t)(cmdData[3] & 0xFFFF));
          pos[1] = EWMatrixFloat16ToFloat((uint16_t)((cmdData[3] & 0xFFFF0000) >> 16));
          pos[2] = EWMatrixFloat16ToFloat((uint16_t)(cmdData[4] & 0xFFFF));
          Backend_SetLight(backend,cmdData[1],(cmdData[2] & 0x38000 != 0) ? 1 : 0,EWColorToRLColor(cmdData[2]),pos,(uint16_t)((cmdData[4] & 0xFFFF0000) >> 16));
          break;
        case GPU_CLEAR:
          BeginTextureMode(framebuffer);
          ClearBackground(EWColorToRLColor(cmdData[1]));
          EndTextureMode();
          break;

        case GPU_UPLOAD_GEOMETRY: {
          if (GPUVertexCount + cmdData[3] > 6144) {
            GPUFlags |= 0x1;
            break;
          }
          GPUVertexCount += cmdData[3];
          // Construct Mesh
          EWVertex *verptr =
              (EWVertex *)((cmdData[4] > 0xf0000000)
                               ? (ROMData + (cmdData[4] - 0xf0000000))
                               : (((uint8_t *)&RAM) + cmdData[3]));
          Mesh mesh = {0};
          mesh.triangleCount = cmdData[3] / 3;
          mesh.vertexCount = mesh.triangleCount * 3;
          mesh.vertices =
              (float *)MemAlloc(mesh.vertexCount * 3 * sizeof(float));
          mesh.texcoords =
              (float *)MemAlloc(mesh.vertexCount * 2 * sizeof(float));
          mesh.normals =
              (float *)MemAlloc(mesh.vertexCount * 3 * sizeof(float));
          mesh.colors = (unsigned char *)MemAlloc(mesh.vertexCount * 4 *
                                                  sizeof(unsigned char));
          for (int i = 0; i < cmdData[3]; i += 3) {
            EWVertex *ver1 = &verptr[i];
            EWVertex *ver2 = &verptr[i+1];
            EWVertex *ver3 = &verptr[i+2];
            mesh.vertices[(i * 3) + 0] =
                EWVertex16ToFloat((uint16_t)(ver1->posXY & 0xFFFF));
            mesh.vertices[(i * 3) + 1] =
                EWVertex16ToFloat((uint16_t)((ver1->posXY & 0xFFFF0000) >> 16));
            mesh.vertices[(i * 3) + 2] =
                EWVertex16ToFloat((uint16_t)(ver1->posZ & 0xFFFF));
            mesh.vertices[(i * 3) + 3] =
                EWVertex16ToFloat((uint16_t)(ver2->posXY & 0xFFFF));
            mesh.vertices[(i * 3) + 4] =
                EWVertex16ToFloat((uint16_t)((ver2->posXY & 0xFFFF0000) >> 16));
            mesh.vertices[(i * 3) + 5] =
                EWVertex16ToFloat((uint16_t)(ver2->posZ & 0xFFFF));
            mesh.vertices[(i * 3) + 6] =
                EWVertex16ToFloat((uint16_t)(ver3->posXY & 0xFFFF));
            mesh.vertices[(i * 3) + 7] =
                EWVertex16ToFloat((uint16_t)((ver3->posXY & 0xFFFF0000) >> 16));
            mesh.vertices[(i * 3) + 8] =
                EWVertex16ToFloat((uint16_t)(ver3->posZ & 0xFFFF));
            mesh.texcoords[(i * 2) + 0] =
                EWTexCoord16ToFloat((uint16_t)(ver1->texCoords & 0xFFFF));
            mesh.texcoords[(i * 2) + 1] = EWTexCoord16ToFloat(
                (uint16_t)((ver1->texCoords & 0xFFFF0000) >> 16));
            mesh.texcoords[(i * 2) + 2] =
                EWTexCoord16ToFloat((uint16_t)(ver2->texCoords & 0xFFFF));
            mesh.texcoords[(i * 2) + 3] = EWTexCoord16ToFloat(
                (uint16_t)((ver2->texCoords & 0xFFFF0000) >> 16));
            mesh.texcoords[(i * 2) + 4] =
                EWTexCoord16ToFloat((uint16_t)(ver3->texCoords & 0xFFFF));
            mesh.texcoords[(i * 2) + 5] = EWTexCoord16ToFloat(
                (uint16_t)((ver3->texCoords & 0xFFFF0000) >> 16));
            Vector3 v1 = Vector3Subtract(
                (Vector3){
                    .x = mesh.vertices[(i * 3) + 3],
                    .y = mesh.vertices[(i * 3) + 4],
                    .z = mesh.vertices[(i * 3) + 5],
                },
                (Vector3){
                    .x = mesh.vertices[(i * 3) + 0],
                    .y = mesh.vertices[(i * 3) + 1],
                    .z = mesh.vertices[(i * 3) + 2],
                });
            Vector3 v2 = Vector3Subtract(
                (Vector3){
                    .x = mesh.vertices[(i * 3) + 6],
                    .y = mesh.vertices[(i * 3) + 7],
                    .z = mesh.vertices[(i * 3) + 8],
                },
                (Vector3){
                    .x = mesh.vertices[(i * 3) + 0],
                    .y = mesh.vertices[(i * 3) + 1],
                    .z = mesh.vertices[(i * 3) + 2],
                });
            Vector3 faceNormal = Vector3Normalize(Vector3CrossProduct(v1, v2));
            mesh.normals[(i * 3) + 0] = faceNormal.x;
            mesh.normals[(i * 3) + 1] = faceNormal.y;
            mesh.normals[(i * 3) + 2] = faceNormal.z;
            mesh.normals[(i * 3) + 3] = faceNormal.x;
            mesh.normals[(i * 3) + 4] = faceNormal.y;
            mesh.normals[(i * 3) + 5] = faceNormal.z;
            mesh.normals[(i * 3) + 6] = faceNormal.x;
            mesh.normals[(i * 3) + 7] = faceNormal.y;
            mesh.normals[(i * 3) + 8] = faceNormal.z;
            Color *colPtr = (Color *)(mesh.colors + ((i * 4)));
            colPtr[0] = EWColorToRLColor(ver1->color);
            colPtr[1] = EWColorToRLColor(ver2->color);
            colPtr[2] = EWColorToRLColor(ver3->color);
          }
          // Upload Mesh
          UploadMesh(&mesh, false);
          // Construct Model
          Model model = LoadModelFromMesh(mesh);
          model.materials[model.meshMaterial[0]]
              .maps[MATERIAL_MAP_DIFFUSE]
              .color = WHITE;
          model.materials[0].shader = backend->shader;
          map_set(&GPUGeometryCache, TextFormat("%08x", cmdData[1]), model);
          break;
        }
        case GPU_FREE_GEOMETRY: {
          Model *model =
              map_get(&GPUGeometryCache, TextFormat("%08x", cmdData[1]));
          if (model == NULL) {
            break;
          }
          GPUVertexCount -= model->meshes[0].vertexCount;
          UnloadModel(*model);
          map_remove(&GPUGeometryCache, TextFormat("%08x", cmdData[1]));
          break;
        }
        case GPU_RENDER_GEOMETRY:
          Model *model =
              map_get(&GPUGeometryCache, TextFormat("%08x", cmdData[1]));
          if (model == NULL) {
            break;
          }
          BeginTextureMode(framebuffer);
          //BeginMode3D(backend->camera);
          EWBeginMode3D(GPUMatrices[0], GPUMatrices[2]);
          SetShaderValue(
          backend->shader, backend->shader.locs[SHADER_LOC_VECTOR_VIEW],
          &((float[3]){GPUViewPos.x, GPUViewPos.y, GPUViewPos.z}),
          SHADER_UNIFORM_VEC3);
          DrawMesh(model->meshes[0], model->materials[0], GPUMatrices[1]);
          EndMode3D();
          EndTextureMode();
          break;

        case GPU_MATRIX_MODE:
          GPUActiveMatrix = cmdData[1];
          break;
        case GPU_MATRIX_IDENTITY:
          GPUMatrices[GPUActiveMatrix] = MatrixIdentity();
          if(GPUActiveMatrix == 2)
            GPUViewPos = Vector3Zero();
          break;
        case GPU_MATRIX_PERSPECTIVE:
          GPUMatrices[GPUActiveMatrix] = MatrixPerspective(
              ((double)cmdData[3]) * DEG2RAD, 4.0 / 3.0, 1, 10000);
          break;
        case GPU_MATRIX_TRANSLATE:
          Vector3 vec = (Vector3){EWMatrixFloat16ToFloat((uint16_t)(cmdData[1] & 0xFFFF)),
                  EWMatrixFloat16ToFloat(
                      (uint16_t)((cmdData[1] & 0xFFFF0000) >> 16)),
                  EWMatrixFloat16ToFloat((uint16_t)(cmdData[2] & 0xFFFF))};
          GPUMatrices[GPUActiveMatrix] = MatrixMultiply(MatrixTranslate(vec.x,vec.y,vec.z),GPUMatrices[GPUActiveMatrix]);
          if(GPUActiveMatrix == 2)
            GPUViewPos = Vector3Add(GPUViewPos,vec);
          break;
        case GPU_MATRIX_ROTATE:
          GPUMatrices[GPUActiveMatrix] = MatrixMultiply(
              MatrixRotateXYZ((Vector3){
                  .x = EWMatrixRot16ToFloat((uint16_t)(cmdData[1] & 0xFFFF)) *
                       PI,
                  .y = EWMatrixRot16ToFloat(
                           (uint16_t)((cmdData[1] & 0xFFFF0000) >> 16)) *
                       PI,
                  .z = EWMatrixRot16ToFloat((uint16_t)(cmdData[2] & 0xFFFF)) *
                       PI}),
              GPUMatrices[GPUActiveMatrix]);
          break;
        case GPU_MATRIX_SCALE:
          GPUMatrices[GPUActiveMatrix] = MatrixMultiply(
              MatrixScale(
                  EWMatrixFloat16ToFloat((uint16_t)(cmdData[1] & 0xFFFF)),
                  EWMatrixFloat16ToFloat(
                      (uint16_t)((cmdData[1] & 0xFFFF0000) >> 16)),
                  EWMatrixFloat16ToFloat((uint16_t)(cmdData[2] & 0xFFFF))),
              GPUMatrices[GPUActiveMatrix]);
          break;

        case GPU_SWAP_BUFFERS:
          BeginMode2D(fbCamera);
          DrawTexturePro(framebuffer.texture, fbSourceRec, fbDestRec, fbOrigin,
                         0.0f, WHITE);
          EndMode2D();
          break;
        default:
          fprintf(stderr,
                  "\x1b[1;31mRECEIVED UNKNOWN GPU COMMAND: 0x%08x (0x%08x, "
                  "0x%08x, 0x%08x, 0x%08x)!\x1b[0m\n",
                  cmdData[0], cmdData[1], cmdData[2], cmdData[3], cmdData[4]);
          break;
      }
      free(cmdData);
    }
    /*SetShaderValue(
        backend->shader, backend->shader.locs[SHADER_LOC_VECTOR_VIEW],
        &((float[3]){backend->camera.position.x, backend->camera.position.y,
                     backend->camera.position.z}),
        SHADER_UNIFORM_VEC3);*/
    EndDrawing();
    if (floor(prevSecond) != floor(GetTime())) {
      SetWindowTitle(TextFormat(
          "EmberWolf - Built on %s (%.2f MIPS%s)", __TIMESTAMP__, ips / 1000000,
          ((ips / 1000000) < 30) ? ", CPU TOO SLOW!" : ""));
      ips = 0;
      prevSecond = GetTime();
    }
  }

  const char *key;
  map_iter_t iter = map_iter(&GPUGeometryCache);

  while ((key = map_next(&GPUGeometryCache, &iter))) {
    UnloadModel(*(map_get(&GPUGeometryCache, key)));
  }

  UnloadShader(backend->shader);
  UnloadTexture(framebuffer.texture);
  map_deinit(&GPUGeometryCache);

  CloseWindow();
}

static uint32_t GPUMsg[5];

uint32_t HandleControlStore(uint32_t *trap, uint32_t addy, uint32_t val) {
  if (addy == 0x11000000) {  // Debug UART
    putc((char)val, stderr);
  } else if (addy >= 0x20000008 && addy <= 0x2000001b) {  // GPU Command FIFO
    GPUMsg[(addy - 0x20000008) / 4] = val;
    if (addy == 0x20000008) {
      if (val == 1) {
        // Clear the FIFO
        atomic_store(&GPUCmdFIFO.isClearing, 1);
        for (int i = 0; i < GPUCmdFIFO.size; i++) {
          if (GPUCmdFIFO.data[i] != NULL) {
            free(GPUCmdFIFO.data[i]);
            GPUCmdFIFO.data[i] = NULL;
          }
        }
        GPUCmdFIFO.head = 0;
        GPUCmdFIFO.tail = 0;
        atomic_store(&GPUCmdFIFO.isClearing, 0);
      } else {
        uint32_t *buf = calloc(5, sizeof(uint32_t));
        memcpy(buf, (void *)&GPUMsg, sizeof(uint32_t) * 5);
        for (int i = 0; i < 5; i++) {
          GPUMsg[i] = 0;
        }
        if (FIFOWrite(&GPUCmdFIFO, buf) == -1) {
          free(buf);  // To prevent the gpu command memory from leaking.
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
  } else if (addy >= 0x20000000 && addy <= 0x2000007) {  // GPU Magic Number
    return *((uint32_t *)(GPUMagicStr + (addy - 0x20000000)));
  } else if (addy >= 0x2000001c && addy <= 0x2000001f) {  // GPU Flags
    uint32_t num = GPUFlags;
    GPUFlags &= 0xfffffff0;
    num |=
        ((((GPUCmdFIFO.head + 1) % GPUCmdFIFO.size) == GPUCmdFIFO.tail) ? 0x4
                                                                        : 0x0);
    num |= ((GPUCmdFIFO.tail == GPUCmdFIFO.head) ? 0x8 : 0x0);
    return num;
  } else if (addy >= 0x20000020 && addy <= 0x20000023) {
    return frameCount;
  } else {
    *trap = (5 + 1);
  }
  return 0;
}