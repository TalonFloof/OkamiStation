#include <SDL2/SDL.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/time.h>
#include <stdlib.h>
#include "karasuFB.h"
#include "ram.h"
#include "okamiboard.h"
#include "okami1041.h"
#include "oipb.h"
#include "timer.h"

SDL_Window *ScreenWindow;
SDL_Renderer *ScreenRenderer;

uint32_t tick_start = 0;
uint32_t tick_end = 0;
int title_update = 0;
uint64_t cycle_count = 0;
bool done = false;

uint64_t iHitCount = 0;
uint64_t iMissCount = 0;
uint64_t dHitCount = 0;
uint64_t dMissCount = 0;

SDL_Rect winrect = {
    .w = 1024,
    .h = 768,
};

int main() {
    struct timeval timeVal;
    char title[96];
    gettimeofday(&timeVal, 0);
    srandom(timeVal.tv_sec);
    reset();
    RAMInit();
    OkamiBoardInit();
    OIPBInit();

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
        fprintf(stderr, "Failed to create window\n");
        return -1;
    }
    reset();
    ScreenRenderer = SDL_CreateRenderer(ScreenWindow, -1, 0);
    if (ScreenRenderer == 0) {
        fprintf(stderr, "Failed to create renderer\n");
        return -1;
    }
    if(SDL_RenderClear(ScreenRenderer) != 0) {
        fprintf(stderr, "Clear Error: %s\n", SDL_GetError());
        abort();
    }
    SDL_RenderPresent(ScreenRenderer);
    SDL_ShowWindow(ScreenWindow);
    SDL_Event event;
    KarasuInit();
    title_update = SDL_GetTicks()+1000;
    while (!done) {
        int dt = SDL_GetTicks() - tick_start;
        tick_start = SDL_GetTicks();
        if (!dt)
		    dt = 1;
        int cyclespertick = 25000000/60/dt;
	    int extracycles = 25000000/60 - (cyclespertick*dt);
        for (int i = 0; i < dt; i++) {
            int cyclesleft = cyclespertick;
            if (i == dt-1)
                cyclesleft += extracycles;
            TimerTick();
            while (cyclesleft > 0) {
                next();
			    cyclesleft -= 1;
		    }
        }
        KarasuUploadFrame();
        SDL_RenderClear(ScreenRenderer);
        if(SDL_RenderCopy(ScreenRenderer, FBTexture, NULL, NULL) != 0) {
            fprintf(stderr, "Render Copy Error: %s\n", SDL_GetError());
            abort();
        }
        SDL_RenderPresent(ScreenRenderer);
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT: {
                    return 0;
                }
                case SDL_KEYDOWN: {
                    KbdPush(event.key.keysym.scancode,0);
                    break;
                }
                case SDL_KEYUP: {
                    KbdPush(event.key.keysym.scancode,1);
                    break;
                }
            }
        }
        tick_end = SDL_GetTicks();
		int delay = 1000/60 - (tick_end - tick_start);
		if (delay > 0) {
			SDL_Delay(delay);
		}
        if(SDL_GetTicks() >= title_update) {
            sprintf((char*)&title, "OkamiStation - %.2f MIPS", (double)cycle_count/1000000.0);
            SDL_SetWindowTitle(ScreenWindow, (char*)&title);
            //double iPercent = (((double)iHitCount)/(((double)iHitCount)+((double)iMissCount)))*100.0;
            //double dPercent = (((double)dHitCount)+((double)dMissCount)) == 0 ? 100.0 : ((((double)dHitCount)/(((double)dHitCount)+((double)dMissCount)))*100.0);
            //fprintf(stderr, "ICache Hits: %i, ICache Misses: %i (%.2f%% hit/miss)\nDCache Hits: %i, DCache Misses: %i (%.2f%% hit/miss)\n", iHitCount, iMissCount, iPercent, dHitCount, dMissCount, dPercent);
            iHitCount = 0;
            iMissCount = 0;
            dHitCount = 0;
            dMissCount = 0;
            cycle_count = 0;
            title_update = SDL_GetTicks()+1000;
        }
    }
    return 0;
}