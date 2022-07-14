#include "icewolf.h"
#include "icebus.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <SDL2/SDL.h>

static IcewolfHart_t* MAIN_HART;

SDL_Window* WINDOW;
SDL_Renderer* RENDER;
SDL_Texture* FBTEX;

SDL_Rect FBRECT;

void SDLMainLoop();

int time_start, time_end;
bool done;

int main(int argc, char* argv[]) {
    fprintf(stderr, "WolfBox Fantasy Computer, version 0.1\n");
    fprintf(stderr, "Copyright (C) 2022-2022 Talon396\n");
    fprintf(stderr, "This software is licensed under the Apache License, Version 2.0\n");
    fprintf(stderr, "License conditions can be found in the COPYING file or at:\n");
    fprintf(stderr, "\thttp://www.apache.org/licenses/LICENSE-2.0\n");
    fprintf(stderr, "Unless required by applicable law or agreed to in writing, software\ndistributed under the License is distributed on an \"AS IS\" BASIS,\nWITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.\nSee the License for the specific language governing permissions and\nlimitations under the License.\n");
    MAIN_HART = IceWolf_CreateHart(0);
    FBRECT = (SDL_Rect){
		.w = 1024,
		.h = 768
	};
    SDL_SetHintWithPriority(SDL_HINT_RENDER_SCALE_QUALITY, "0", SDL_HINT_OVERRIDE);
    SDL_SetHintWithPriority(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "1", SDL_HINT_OVERRIDE);
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "\x1b[31mSDL Initalization Failed due to, %s\x1b[0m\n", SDL_GetError());
        return 1;
    }
    if(!IceBusInit()) {
        fprintf(stderr, "\x1b[31mIceBus initalization failed\x1b[0m\n");
        return 1;
    }
    SDL_EnableScreenSaver();
    WINDOW = SDL_CreateWindow("WolfBox",SDL_WINDOWPOS_UNDEFINED_DISPLAY(0),SDL_WINDOWPOS_UNDEFINED_DISPLAY(0),FBRECT.w,FBRECT.h,SDL_WINDOW_HIDDEN | SDL_WINDOW_ALLOW_HIGHDPI);
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
    done = false;
    while (!done) {
		SDLMainLoop();

		time_end = SDL_GetTicks();
		int delay = 1000/60 - (time_end - time_start);
		if (delay > 0) {
			SDL_Delay(delay);
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
			remaining -= IceWolf_RunCycles(MAIN_HART,remaining);
		}
    }
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
        }
    }
}