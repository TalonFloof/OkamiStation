#define _KARASU_IMPL
#include "karasuFB.h"
#include "koribus.h"
#include <string.h>
#include <stdlib.h>

uint8_t* framebuffer;
uint32_t outputTexture[1024*768];
SDL_Texture *FBTexture;
extern SDL_Renderer *ScreenRenderer;
uint32_t KarasuPalette[256] = {0x000000,0x000040,0x000080,0x0000c0,0x0000ff,0x002400,0x002440,0x002480,0x0024c0,0x0024ff,0x004900,0x004940,0x004980,0x0049c0,0x0049ff,0x006d00,0x006d40,0x006d80,0x006dc0,0x006dff,0x009200,0x009240,0x009280,0x0092c0,0x0092ff,0x00b600,0x00b640,0x00b680,0x00b6c0,0x00b6ff,0x00db00,0x00db40,0x00db80,0x00dbc0,0x00dbff,0x00ff00,0x00ff40,0x00ff80,0x00ffc0,0x00ffff,0x330000,0x330040,0x330080,0x3300c0,0x3300ff,0x332400,0x332440,0x332480,0x3324c0,0x3324ff,0x334900,0x334940,0x334980,0x3349c0,0x3349ff,0x336d00,0x336d40,0x336d80,0x336dc0,0x336dff,0x339200,0x339240,0x339280,0x3392c0,0x3392ff,0x33b600,0x33b640,0x33b680,0x33b6c0,0x33b6ff,0x33db00,0x33db40,0x33db80,0x33dbc0,0x33dbff,0x33ff00,0x33ff40,0x33ff80,0x33ffc0,0x33ffff,0x660000,0x660040,0x660080,0x6600c0,0x6600ff,0x662400,0x662440,0x662480,0x6624c0,0x6624ff,0x664900,0x664940,0x664980,0x6649c0,0x6649ff,0x666d00,0x666d40,0x666d80,0x666dc0,0x666dff,0x669200,0x669240,0x669280,0x6692c0,0x6692ff,0x66b600,0x66b640,0x66b680,0x66b6c0,0x66b6ff,0x66db00,0x66db40,0x66db80,0x66dbc0,0x66dbff,0x66ff00,0x66ff40,0x66ff80,0x66ffc0,0x66ffff,0x990000,0x990040,0x990080,0x9900c0,0x9900ff,0x992400,0x992440,0x992480,0x9924c0,0x9924ff,0x994900,0x994940,0x994980,0x9949c0,0x9949ff,0x996d00,0x996d40,0x996d80,0x996dc0,0x996dff,0x999200,0x999240,0x999280,0x9992c0,0x9992ff,0x99b600,0x99b640,0x99b680,0x99b6c0,0x99b6ff,0x99db00,0x99db40,0x99db80,0x99dbc0,0x99dbff,0x99ff00,0x99ff40,0x99ff80,0x99ffc0,0x99ffff,0xcc0000,0xcc0040,0xcc0080,0xcc00c0,0xcc00ff,0xcc2400,0xcc2440,0xcc2480,0xcc24c0,0xcc24ff,0xcc4900,0xcc4940,0xcc4980,0xcc49c0,0xcc49ff,0xcc6d00,0xcc6d40,0xcc6d80,0xcc6dc0,0xcc6dff,0xcc9200,0xcc9240,0xcc9280,0xcc92c0,0xcc92ff,0xccb600,0xccb640,0xccb680,0xccb6c0,0xccb6ff,0xccdb00,0xccdb40,0xccdb80,0xccdbc0,0xccdbff,0xccff00,0xccff40,0xccff80,0xccffc0,0xccffff,0xff0000,0xff0040,0xff0080,0xff00c0,0xff00ff,0xff2400,0xff2440,0xff2480,0xff24c0,0xff24ff,0xff4900,0xff4940,0xff4980,0xff49c0,0xff49ff,0xff6d00,0xff6d40,0xff6d80,0xff6dc0,0xff6dff,0xff9200,0xff9240,0xff9280,0xff92c0,0xff92ff,0xffb600,0xffb640,0xffb680,0xffb6c0,0xffb6ff,0xffdb00,0xffdb40,0xffdb80,0xffdbc0,0xffdbff,0xffff00,0xffff40,0xffff80,0xffffc0,0xffffff,0x000000,0x111111,0x222222,0x333333,0x444444,0x555555,0x666666,0x777777,0x888888,0x999999,0xaaaaaa,0xbbbbbb,0xcccccc,0xdddddd,0xeeeeee,0xffffff};
uint32_t FBMode = 0;

bool Dirty = false;

uint64_t DirtyX1 = 0;
uint64_t DirtyX2 = 0;
uint64_t DirtyY1 = 0;
uint64_t DirtyY2 = 0;

static inline void MarkDirty(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2) {
	Dirty = true;

	if (x1 < DirtyX1)
		DirtyX1 = x1;
	if (y1 < DirtyY1)
		DirtyY1 = y1;
	if (x2 > DirtyX2)
		DirtyX2 = x2;
	if (y2 > DirtyY2)
		DirtyY2 = y2;
}

int KarasuWrite(uint32_t addr, uint32_t len, void *buf) {
    if (addr < 0x1000) { // Registers
        if(addr == 0x0) {
            FBMode = *(uint32_t*)buf;
        } else if(addr >= 0xC00) { // Palette
            KarasuPalette[(addr-0xC00)/4] = *(uint32_t*)buf;
        }
        return 1;
    } else {
        addr -= 0x1000;
        if (addr+len > 1024*768)
			return 0;
		uint32_t x = addr%1024;
		uint32_t y = addr/1024;
        uint32_t x1 = (addr+len-1)%1024;
		uint32_t y1 = (addr+len-1)/1024;
        memcpy(framebuffer+addr, (uint8_t*)buf, len);
        MarkDirty(x,y,x1,y1);
        return 1;
    }
    return 0;
}

int KarasuRead(uint32_t addr, uint32_t len, void *buf) {
    if (addr < 0x1000) { // Registers
        if(addr == 0x0) {
            *(uint32_t*)buf = FBMode;
        } else if(addr >= 0xC00) { // Palette
            *(uint32_t*)buf = KarasuPalette[(addr-0xC00)/4];
        }
        return 1;
    } else {
        addr -= 0x1000;
        if (addr+len > 1024*768)
			return 0;
        memcpy(buf, framebuffer+addr, len);
        return 1;
    }
    return 0;
}

void KarasuInit() {
    framebuffer = malloc(1024*768);
    memset((void*)framebuffer,0,1024*768);
    KoriBusBanks[8].Used = true;
    KoriBusBanks[8].Read = KarasuRead;
    KoriBusBanks[8].Write = KarasuWrite;
    FBTexture = SDL_CreateTexture(
        ScreenRenderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        1024,
        768
    );
    SDL_SetTextureScaleMode(FBTexture, SDL_ScaleModeNearest);
    if(FBTexture == 0) {
        fprintf(stderr, SDL_GetError());
        fprintf(stderr, "\n");
        exit(1);
    }
    DirtyX2 = 1023;
	DirtyY2 = 767;
    Dirty = true;
}

void KarasuUploadFrame() {
    if (!Dirty)
        return;
    uint64_t dirty_index = (DirtyY1*1024)+DirtyX1;
    uint64_t pixbuf_index = 0;
    int w = DirtyX2-DirtyX1+1;
	int h = DirtyY2-DirtyY1+1;
    if(FBMode == 0) {
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                outputTexture[pixbuf_index++] = KarasuPalette[((framebuffer[dirty_index+(x/8)]>>(7-(x%8))) & 1)?255:0];
            }
            dirty_index += 128;
        }
    } else if(FBMode == 1) {
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                outputTexture[pixbuf_index++] = KarasuPalette[framebuffer[dirty_index+x]];
            }
            dirty_index += 1024;
        }
    }
    SDL_Rect rect = {
		.x = DirtyX1,
		.y = DirtyY1,
		.w = w,
		.h = h,
	};
    if(SDL_UpdateTexture(FBTexture, &rect, outputTexture, rect.w * 4) != 0) {
        fprintf(stderr, "Texture Upload Error: %s\n", SDL_GetError());
        abort();
    }
    DirtyX1 = -1;
	DirtyX2 = 0;
	DirtyY1 = -1;
	DirtyY2 = 0;
	Dirty = false;
}