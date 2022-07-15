/*
WolfBox Fantasy Workstation
Copyright 2022-2022 Talon396

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "icewolf.h"
#include "icebus.h"
#include "framebuffer.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <SDL2/SDL.h>

static IcewolfHart_t* MAIN_HART;

SDL_Window* WINDOW;
SDL_Renderer* RENDER;
SDL_Texture* FBTEX;

SDL_Rect FBRECT;

void SDLMainLoop();

int time_start, time_end;
int title_update = 0;
uint64_t cycle_count = 0;
bool done;

int main(int argc, char* argv[]) {
    time_t t;
    char title[96];
    srand((unsigned) time(&t));
    MAIN_HART = IceWolf_CreateHart(0);
    FBRECT = (SDL_Rect){
		.w = 1024,
		.h = 768
	};
    if(!IceBusInit()) {
        fprintf(stderr, "\x1b[31mIceBus initalization failed\x1b[0m\n");
        return 1;
    }
    SDL_SetHintWithPriority(SDL_HINT_RENDER_SCALE_QUALITY, "0", SDL_HINT_OVERRIDE);
    SDL_SetHintWithPriority(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "1", SDL_HINT_OVERRIDE);
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "\x1b[31mSDL Initalization Failed due to, %s\x1b[0m\n", SDL_GetError());
        return 1;
    }
    SDL_EnableScreenSaver();
    sprintf((char*)&title, "WolfBox built on %s - %.2f MIPS", __TIMESTAMP__, 0.0);
    WINDOW = SDL_CreateWindow((char*)&title,SDL_WINDOWPOS_UNDEFINED_DISPLAY(0),SDL_WINDOWPOS_UNDEFINED_DISPLAY(0),FBRECT.w,FBRECT.h,SDL_WINDOW_HIDDEN | SDL_WINDOW_ALLOW_HIGHDPI);
    if(!WINDOW) {
		fprintf(stderr, "\x1b[31mSDL Window creation failed\x1b[0m\n");
		return 1;
	}
    RENDER = SDL_CreateRenderer(WINDOW, -1, 0);
	if(RENDER == NULL) {
		fprintf(stderr, "\x1b[31mSDL Render creation failed due to, %s\x1b[0m\n", SDL_GetError());
		return 1;
	}
    FBTEX = SDL_CreateTexture(RENDER,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_STREAMING,FBRECT.w,FBRECT.h);
    if(FBTEX == NULL) {
        fprintf(stderr, "\x1b[31mSDL Texture creation failed due to, %s\x1b[0m\n", SDL_GetError());
		return 1;
    }
    SDL_SetTextureScaleMode(FBTEX, SDL_ScaleModeNearest);
    SDL_ShowWindow(WINDOW);
    SDL_RenderClear(RENDER);
	SDL_RenderCopy(RENDER, FBTEX, &FBRECT, &FBRECT);
	SDL_RenderPresent(RENDER);
    time_start = SDL_GetTicks();
    time_end = SDL_GetTicks();
    title_update = SDL_GetTicks()+1000;
    done = false;
    while (!done) {
		SDLMainLoop();

		time_end = SDL_GetTicks();
		int delay = 1000/60 - (time_end - time_start);
		if (delay > 0) {
			SDL_Delay(delay);
		}
        if(SDL_GetTicks() >= title_update) {
            sprintf((char*)&title, "WolfBox built on %s - %.2f MIPS", __TIMESTAMP__, (double)cycle_count/1000000.0);
            SDL_SetWindowTitle(WINDOW, (char*)&title);
            cycle_count = 0;
            title_update = SDL_GetTicks()+1000;
        }
	}
    return 0;
}

void SDLMainLoop() {
    int displaced_time = SDL_GetTicks() - time_start;
    time_start = SDL_GetTicks();
    if (!displaced_time) // To prevent us from dividing by zero
		displaced_time = 1;
    int cycleseveryframe = HART_FREQ/60/displaced_time;
    int extracycles = HART_FREQ/60 - (cycleseveryframe*displaced_time);
    for (int i = 0; i < displaced_time; i++) {
        int remaining = cycleseveryframe;
        if (i == displaced_time-1)
            remaining += extracycles;
        while (remaining > 0) {
            int amount = IceWolf_RunCycles(MAIN_HART,remaining);
			remaining -= amount;
            cycle_count += amount;
		}
    }
    FBDraw();
	SDL_RenderClear(RENDER);
	SDL_RenderCopy(RENDER, FBTEX, &FBRECT, &FBRECT);
	SDL_RenderPresent(RENDER);
    SDL_Event event;
	while (SDL_PollEvent(&event)) {
        switch (event.type) {
			case SDL_QUIT: {
				done = true;
				break;
			}
            case SDL_KEYDOWN: {
                if(event.key.keysym.scancode == SDL_SCANCODE_F11) {
                    SDL_SetWindowFullscreen(WINDOW,(SDL_GetWindowFlags(WINDOW) & SDL_WINDOW_FULLSCREEN) != SDL_WINDOW_FULLSCREEN);
                }
                break;
            }
        }
    }
}