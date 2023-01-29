#define _KARASU_IMPL
#include "karasuFB.h"
#include "koribus.h"
#include <string.h>
#include <stdlib.h>

uint8_t framebuffer[1024*768];
uint8_t finalFBTexture[1024*768*3];

void KarasuInit() {
    memset((void*)&finalFBTexture,0,sizeof(finalFBTexture));
    memset((void*)&framebuffer,0,sizeof(framebuffer));
    KoriBusBanks[8].Used = true;
    
}