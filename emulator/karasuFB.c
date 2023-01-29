#define _KARASU_IMPL
#include "karasuFB.h"
#include "koribus.h"
#include <string.h>
#include <stdlib.h>

uint8_t finalFBTexture[1024*768*3];

void KarasuInit() {
    memset((void*)&finalFBTexture,0,sizeof(finalFBTexture));
    uint8_t resetBuffer[8*8*3]; // Simulate Random Screen Garbage
    for(int i=0; i < 8*8*3; i++) {
        resetBuffer[i] = random()%256;
    }
    finalFBTexture[0] = 0xFF;
    finalFBTexture[1] = 0xFF;
    finalFBTexture[2] = 0xFF;
    /*for(int i=0; i < 768;i += 8) {
        int action = random()%30;
        if(action == 0) {
            continue;
        } else if(action == 1) {
            for(int y=i; y < i+8; y++) {
                for(int x=0; x < 1024; x+=8) {
                    memcpy(((uint8_t*)finalFBTexture)+(y*(1024*3)+(x*3)),((uint8_t*)&resetBuffer)+((y-i)*(8*3)),8*3);
                }
            }
        } else if(action == 2) {
            uint8_t colorR = random()%256;
            uint8_t colorG = random()%256;
            uint8_t colorB = random()%256;
            for(int y=i; y < i+8; y++) {
                for(int x=0; x < 1024; x++) {
                    finalFBTexture[y*(1024*3)+(x*3)+0] = colorR;
                    finalFBTexture[y*(1024*3)+(x*3)+1] = colorG;
                    finalFBTexture[y*(1024*3)+(x*3)+2] = colorB;
                }
            }
        }
    }*/
    KoriBusBanks[8].Used = true;

}