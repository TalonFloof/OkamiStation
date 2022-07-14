#pragma once
#include <stdint.h>

int IceBusRead(uint64_t addr, uint64_t len, uint8_t* buf);
int IceBusWrite(uint64_t addr, uint64_t len, uint8_t* buf);