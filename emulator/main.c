#include <SDL2/SDL.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/time.h>
#include <stdlib.h>
#include "karasuFB.h"
#include "ram.h"
#include "okami1041.h"

SDL_Window *ScreenWindow;
SDL_Renderer *ScreenRenderer;

uint32_t tick_start;
uint32_t tick_end;
bool done = false;

int main() {
    struct timeval timeVal;
    gettimeofday(&timeVal, 0);
    srandom(timeVal.tv_sec);
    RAMInit();
    KarasuInit();

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "unable to initialize SDL: %s", SDL_GetError());
        return -1;
    }
    SDL_ShowCursor(SDL_DISABLE);
    ScreenWindow = SDL_CreateWindow(
        "OkamiStation",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        1024,
        768,
        SDL_WINDOW_HIDDEN
    );
    if (!ScreenWindow) {
        fprintf(stderr, "failed to create window\n");
        return -1;
    }
    ScreenRenderer = SDL_CreateRenderer(ScreenWindow, -1, 0);
    if (!ScreenRenderer) {
        fprintf(stderr, "failed to create renderer\n");
        return -1;
    }
    reset();
    SDL_ShowWindow(ScreenWindow);
    SDL_Event event;
    SDL_Rect screenrect = {
        .w = 1024,
        .h = 768,
    };

    SDL_Rect winrect = {
        .w = 1024,
        .h = 768,
        .x = 0,
        .y = 0
    };

    while (!done) {
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT: {
                    return 0;
                }
            }
        }
        KarasuUploadFrame();
        SDL_RenderClear(ScreenRenderer);
        SDL_RenderCopy(ScreenRenderer, FBTexture, &screenrect, &winrect);
        SDL_RenderPresent(ScreenRenderer);
        tick_end = SDL_GetTicks();
		int delay = 1000/60 - (tick_end - tick_start);
		if (delay > 0) {
			SDL_Delay(delay);
		}
    }
    return 0;
}