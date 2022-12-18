#include <backend.h>
#define RLIGHTS_IMPLEMENTATION
#include <float.h>
#include <raymath.h>
#include <rlgl.h>
#include <stdio.h>

extern char execPath[2048];

const int WIDTH = 256;
const int HEIGHT = 192;

const float RATIO = (float)WIDTH / (float)HEIGHT;

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

void Backend_Run(Backend *backend) {
  SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_TRANSPARENT);
  InitWindow(WIDTH * 3, HEIGHT * 3,
             TextFormat("EmberWolf - Built on %s %s", __TIME__, __DATE__));
  SetTargetFPS(60);

  RenderTexture2D framebuffer = LoadRenderTexture(WIDTH, HEIGHT);

  Camera2D fbCamera = {0};  // Camera of the 2D Framebuffer
  fbCamera.zoom = 1.0;

  Rectangle fbSourceRec = {0.0f, 0.0f, (float)framebuffer.texture.width,
                           -(float)framebuffer.texture.height};
  Rectangle fbDestRec = {-RATIO, -RATIO, (WIDTH * 3) + (RATIO * 2),
                         (HEIGHT * 3) + (RATIO * 2)};

  Vector2 fbOrigin = {0.0f, 0.0f};

  Model cube = LoadModelFromMesh(GenMeshCube(5, 5, 5));
  Texture2D cubeTexture =
      LoadTexture(TextFormat("%s/resources/assets/test.png", execPath));

  backend->shader =
      LoadShader(TextFormat("%s/resources/shaders/base.vs", execPath),
                 TextFormat("%s/resources/shaders/base.fs", execPath));
  backend->shader.locs[SHADER_LOC_VECTOR_VIEW] =
      GetShaderLocation(backend->shader, "u_viewPos");
  backend->shader.locs[SHADER_LOC_MATRIX_MODEL] =
      GetShaderLocation(backend->shader, "matModel");

  int i = 0;

  Backend_SetFog(backend, (float[3]){30, 40, 0}, (Color){32, 32, 48, 255});

  Backend_SetLight(backend, 0, 1, (Color){255, 0, 0, 255}, (float[3]){5, 5, 5},
                   1024);
  Backend_SetLight(backend, 1, 1, (Color){0, 255, 0, 255}, (float[3]){-5, 5, 5},
                   1024);
  Backend_SetLight(backend, 2, 1, (Color){0, 0, 255, 255}, (float[3]){5, 5, -5},
                   1024);
  Backend_SetLight(backend, 3, 1, (Color){255, 255, 255, 255},
                   (float[3]){-5, 5, -5}, 1024);

  int loc = GetShaderLocation(backend->shader, "u_lightAmbient");
  SetShaderValue(backend->shader, loc, (float[4]){0, 0, 0, 1},
                 SHADER_UNIFORM_VEC4);

  for (i = 0; i < cube.materialCount; i++) {
    cube.materials[i].shader = backend->shader;
  }
  i = 0;
  cube.materials[0].maps[MATERIAL_MAP_ALBEDO].texture = cubeTexture;

  SetCameraMode(backend->camera, CAMERA_ORBITAL);

  while (!WindowShouldClose()) {
    UpdateCamera(&(backend->camera));

    SetShaderValue(
        backend->shader, backend->shader.locs[SHADER_LOC_VECTOR_VIEW],
        &((float[3]){backend->camera.position.x, backend->camera.position.y,
                     backend->camera.position.z}),
        SHADER_UNIFORM_VEC3);

    BeginTextureMode(framebuffer);
    ClearBackground((Color){32, 32, 48, 255});
    BeginMode3D(backend->camera);
    DrawModelEx(cube, Vector3Zero(), (Vector3){.x = 0, .y = 0, .z = 0}, 0,
                Vector3One(), WHITE);
    DrawModelEx(cube, (Vector3){.x = 15, .y = 0, .z = 0},
                (Vector3){.x = 0, .y = 0, .z = 0}, 0, Vector3One(), WHITE);
    DrawModelEx(cube, (Vector3){.x = -15, .y = 0, .z = 0},
                (Vector3){.x = 0, .y = 0, .z = 0}, 0, Vector3One(), WHITE);
    DrawModelEx(cube, (Vector3){.x = 0, .y = 0, .z = 15},
                (Vector3){.x = 0, .y = 0, .z = 0}, 0, Vector3One(), WHITE);
    DrawModelEx(cube, (Vector3){.x = 0, .y = 0, .z = -15},
                (Vector3){.x = 0, .y = 0, .z = 0}, 0, Vector3One(), WHITE);
    EndMode3D();
    EndTextureMode();

    BeginDrawing();
    BeginMode2D(fbCamera);
    DrawTexturePro(framebuffer.texture, fbSourceRec, fbDestRec, fbOrigin, 0.0f,
                   WHITE);
    EndMode2D();
    DrawFPS(0, 0);
    EndDrawing();
  }

  UnloadModel(cube);
  UnloadShader(backend->shader);
  UnloadTexture(framebuffer.texture);

  CloseWindow();
}
