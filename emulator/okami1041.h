#pragma once

extern int shouldCacheStall;

void triggerTrap(uint32_t type, uint32_t addr, bool afterInc);
void reset();
void next();