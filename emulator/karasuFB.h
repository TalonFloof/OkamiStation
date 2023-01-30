#pragma once
#include <SDL2/SDL.h>
#include <stdint.h>

#ifndef _KARASU_IMPL
extern SDL_Texture *FBTexture;
#endif

void KarasuInit();
void KarasuUploadFrame();