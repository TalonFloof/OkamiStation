#include "koribus.h"

KoriBusBank KoriBusBanks[16];

bool KoriBusInit() {
    for (int i = 0; i < 16; i++) {
		KoriBusBanks[i].Used = 0;
        KoriBusBanks[i].Read = 0;
        KoriBusBanks[i].Write = 0;
	}
    return true;
}