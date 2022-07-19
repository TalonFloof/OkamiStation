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
extern SDL_Texture* FBTEX;

uint8_t *Framebuffer = NULL;
uint32_t* Pixelbuffer = NULL;
uint32_t Pal[256] = {0x000000,0x000040,0x000080,0x0000c0,0x0000ff,0x002400,0x002440,0x002480,0x0024c0,0x0024ff,0x004900,0x004940,0x004980,0x0049c0,0x0049ff,0x006d00,0x006d40,0x006d80,0x006dc0,0x006dff,0x009200,0x009240,0x009280,0x0092c0,0x0092ff,0x00b600,0x00b640,0x00b680,0x00b6c0,0x00b6ff,0x00db00,0x00db40,0x00db80,0x00dbc0,0x00dbff,0x00ff00,0x00ff40,0x00ff80,0x00ffc0,0x00ffff,0x330000,0x330040,0x330080,0x3300c0,0x3300ff,0x332400,0x332440,0x332480,0x3324c0,0x3324ff,0x334900,0x334940,0x334980,0x3349c0,0x3349ff,0x336d00,0x336d40,0x336d80,0x336dc0,0x336dff,0x339200,0x339240,0x339280,0x3392c0,0x3392ff,0x33b600,0x33b640,0x33b680,0x33b6c0,0x33b6ff,0x33db00,0x33db40,0x33db80,0x33dbc0,0x33dbff,0x33ff00,0x33ff40,0x33ff80,0x33ffc0,0x33ffff,0x660000,0x660040,0x660080,0x6600c0,0x6600ff,0x662400,0x662440,0x662480,0x6624c0,0x6624ff,0x664900,0x664940,0x664980,0x6649c0,0x6649ff,0x666d00,0x666d40,0x666d80,0x666dc0,0x666dff,0x669200,0x669240,0x669280,0x6692c0,0x6692ff,0x66b600,0x66b640,0x66b680,0x66b6c0,0x66b6ff,0x66db00,0x66db40,0x66db80,0x66dbc0,0x66dbff,0x66ff00,0x66ff40,0x66ff80,0x66ffc0,0x66ffff,0x990000,0x990040,0x990080,0x9900c0,0x9900ff,0x992400,0x992440,0x992480,0x9924c0,0x9924ff,0x994900,0x994940,0x994980,0x9949c0,0x9949ff,0x996d00,0x996d40,0x996d80,0x996dc0,0x996dff,0x999200,0x999240,0x999280,0x9992c0,0x9992ff,0x99b600,0x99b640,0x99b680,0x99b6c0,0x99b6ff,0x99db00,0x99db40,0x99db80,0x99dbc0,0x99dbff,0x99ff00,0x99ff40,0x99ff80,0x99ffc0,0x99ffff,0xcc0000,0xcc0040,0xcc0080,0xcc00c0,0xcc00ff,0xcc2400,0xcc2440,0xcc2480,0xcc24c0,0xcc24ff,0xcc4900,0xcc4940,0xcc4980,0xcc49c0,0xcc49ff,0xcc6d00,0xcc6d40,0xcc6d80,0xcc6dc0,0xcc6dff,0xcc9200,0xcc9240,0xcc9280,0xcc92c0,0xcc92ff,0xccb600,0xccb640,0xccb680,0xccb6c0,0xccb6ff,0xccdb00,0xccdb40,0xccdb80,0xccdbc0,0xccdbff,0xccff00,0xccff40,0xccff80,0xccffc0,0xccffff,0xff0000,0xff0040,0xff0080,0xff00c0,0xff00ff,0xff2400,0xff2440,0xff2480,0xff24c0,0xff24ff,0xff4900,0xff4940,0xff4980,0xff49c0,0xff49ff,0xff6d00,0xff6d40,0xff6d80,0xff6dc0,0xff6dff,0xff9200,0xff9240,0xff9280,0xff92c0,0xff92ff,0xffb600,0xffb640,0xffb680,0xffb6c0,0xffb6ff,0xffdb00,0xffdb40,0xffdb80,0xffdbc0,0xffdbff,0xffff00,0xffff40,0xffff80,0xffffc0,0xffffff,0x000000,0x111111,0x222222,0x333333,0x444444,0x555555,0x666666,0x777777,0x888888,0x999999,0xaaaaaa,0xbbbbbb,0xcccccc,0xdddddd,0xeeeeee,0xffffff};

bool Dirty = false;

uint64_t DirtyX1 = 0;
uint64_t DirtyX2 = 0;
uint64_t DirtyY1 = 0;
uint64_t DirtyY2 = 0;

void MarkDirty(uint64_t x1, uint64_t y1, uint64_t x2, uint64_t y2) {
    if(!Dirty) {
		DirtyX1 = x1;
		DirtyY1 = y1;
		DirtyX2 = x2;
		DirtyY2 = y2;
		Dirty = true;
		return;
	}
    if(x1 < DirtyX1)
		DirtyX1 = x1;
    if(y1 < DirtyY1)
        DirtyY1 = y1;
    if (x2 > DirtyX2) {
		if (x2 < (FBRECT.w)) {
			DirtyX2 = x2;
		} else {
			DirtyX2 = (FBRECT.w)-1;
		}
	}
    if (y2 > DirtyY2) {
		if (y2 < (FBRECT.h)) {
			DirtyY2 = y2;
		} else {
			DirtyY2 = (FBRECT.h)-1;
		}
	}
}

int FBWrite(uint64_t addr, uint64_t len, void *buf) {
    if (addr < 0x1000) { // Registers
        if(addr >= 0xC00) { // Palette
            Pal[addr-0xC00] = *(uint32_t*)buf;
        }
        return 0;
    } else {
        addr -= 0x1000;
        if (addr+len > FBRECT.w*FBRECT.h)
			return 1;
		uint64_t x = addr%FBRECT.w;
		uint64_t y = addr/FBRECT.w;
        uint64_t x1 = (addr+len-1)%FBRECT.w;
		uint64_t y1 = (addr+len-1)/FBRECT.w;
        memcpy(&Framebuffer[addr], (uint8_t*)buf, len);
        MarkDirty(x,y,x1,y1);
        return 0;
    }
}

int FBRead(uint64_t addr, uint64_t len, void *buf) {
    if (addr < 0x1000) { // Registers
        if(addr >= 0xC00) { // Palette
            *(uint32_t*)buf = Pal[addr-0xC00];
        }
        return 0;
    } else {
        addr -= 0x1000;
        if (addr+len > FBRECT.w*FBRECT.h)
			return 1;
        memcpy(buf, &Framebuffer[addr], len);
        return 0;
    }
}

bool FBDraw() {
    if (!Dirty)
		return false;
    uint64_t dirty_index = (DirtyY1*FBRECT.w)+DirtyX1;
    uint64_t pixbuf_index = 0;
    int w = DirtyX2-DirtyX1+1;
	int h = DirtyY2-DirtyY1+1;
    for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
            Pixelbuffer[pixbuf_index++] = Pal[Framebuffer[dirty_index+x]&0xFF];
        }
        dirty_index += FBRECT.w;
    }
    SDL_Rect rect = {
		.x = DirtyX1,
		.y = DirtyY1,
		.w = w,
		.h = h,
	};
    SDL_UpdateTexture(FBTEX, &rect, Pixelbuffer, rect.w * 4);
    Dirty = false;
    return true;
}

bool FBInit() {
    IceBusBanks[15].Used = true;
    IceBusBanks[15].Read = FBRead;
    IceBusBanks[15].Write = FBWrite;
    Framebuffer = malloc(FBRECT.w*FBRECT.h);
    Pixelbuffer = malloc(FBRECT.w*FBRECT.h*4);
    memset(Pixelbuffer,0,FBRECT.w*FBRECT.h*4);
    for (int i = 0; i < FBRECT.w*FBRECT.h; i++) {
        Framebuffer[i] = rand()&0xFF;
    }
    Dirty = true;
    DirtyX2 = FBRECT.w-1;
	DirtyY2 = FBRECT.h-1;
    return true;
}