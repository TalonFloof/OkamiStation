#pragma once

extern int MousePressed;

void KbdPush(int sdlCode, int released);
void MouseUpdate(int8_t relX, int8_t relY, uint32_t button);
void OIPBInit();