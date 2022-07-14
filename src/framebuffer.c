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

#include "framebuffer.h"
#include "icebus.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
extern SDL_Rect FBRECT;

uint8_t *Framebuffer = NULL;
uint32_t Pal[256];

bool Dirty = true;

uint64_t DirtyX1, DirtyX2, DirtyY1, DirtyY2 = 0;

bool FBDraw() {
    if (!Dirty)
		return false;
    uint64_t dirty_index = (DirtyY1*FBRECT.w)+DirtyX1;
    int w = DirtyX2-DirtyX1+1;
	int h = DirtyY2-DirtyY1+1;
    for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
            Framebuffer[dirty_index++] = Pal[((uint8_t*)Framebuffer)[dirty_index+x]&0xFF];
        }
    }
}

int FBInit() {
    IceBusBanks[15].Used = true;
    Framebuffer = malloc(FBRECT.w*FBRECT.h);
}