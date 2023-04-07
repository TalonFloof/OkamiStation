#include "okamiboard.h"
#include "htc.h"
#include <SDL2/SDL.h>

typedef struct {
	unsigned char code;
} KeyInfo;

static KeyInfo ScancodeMapping[SDL_NUM_SCANCODES] = {
    [SDL_SCANCODE_0] = { 0x01 },
	[SDL_SCANCODE_1] = { 0x02 },
	[SDL_SCANCODE_2] = { 0x03 },
	[SDL_SCANCODE_3] = { 0x04 },
	[SDL_SCANCODE_4] = { 0x05 },
	[SDL_SCANCODE_5] = { 0x06 },
	[SDL_SCANCODE_6] = { 0x07 },
	[SDL_SCANCODE_7] = { 0x08 },
	[SDL_SCANCODE_8] = { 0x09 },
	[SDL_SCANCODE_9] = { 0x0a },
    [SDL_SCANCODE_A] = { 0x0b },
	[SDL_SCANCODE_B] = { 0x0c },
	[SDL_SCANCODE_C] = { 0x0d },
	[SDL_SCANCODE_D] = { 0x0e },
	[SDL_SCANCODE_E] = { 0x0f },
	[SDL_SCANCODE_F] = { 0x10 },
	[SDL_SCANCODE_G] = { 0x11 },
	[SDL_SCANCODE_H] = { 0x12 },
	[SDL_SCANCODE_I] = { 0x13 },
	[SDL_SCANCODE_J] = { 0x14 },
	[SDL_SCANCODE_K] = { 0x15 },
	[SDL_SCANCODE_L] = { 0x16 },
	[SDL_SCANCODE_M] = { 0x17 },
	[SDL_SCANCODE_N] = { 0x18 },
	[SDL_SCANCODE_O] = { 0x19 },
	[SDL_SCANCODE_P] = { 0x1a },
	[SDL_SCANCODE_Q] = { 0x1b },
	[SDL_SCANCODE_R] = { 0x1c },
	[SDL_SCANCODE_S] = { 0x1d },
	[SDL_SCANCODE_T] = { 0x1e },
	[SDL_SCANCODE_U] = { 0x1f },
	[SDL_SCANCODE_V] = { 0x20 },
	[SDL_SCANCODE_W] = { 0x21 },
	[SDL_SCANCODE_X] = { 0x22 },
	[SDL_SCANCODE_Y] = { 0x23 },
	[SDL_SCANCODE_Z] = { 0x24 },
	[SDL_SCANCODE_SEMICOLON] = { 0x25 },
	[SDL_SCANCODE_SPACE]     = { 0x26 },
	[SDL_SCANCODE_TAB]       = { 0x27 },
	[SDL_SCANCODE_MINUS]        = { 0x28 },
	[SDL_SCANCODE_EQUALS]       = { 0x29 },
	[SDL_SCANCODE_LEFTBRACKET]  = { 0x2A },
	[SDL_SCANCODE_RIGHTBRACKET] = { 0x2B },
	[SDL_SCANCODE_BACKSLASH]    = { 0x2C },
	[SDL_SCANCODE_NONUSHASH]    = { 0x2C },
	[SDL_SCANCODE_SLASH]      = { 0x2D },
	[SDL_SCANCODE_PERIOD]     = { 0x2E },
	[SDL_SCANCODE_APOSTROPHE] = { 0x2F },
	[SDL_SCANCODE_COMMA]      = { 0x30 },
	[SDL_SCANCODE_GRAVE]      = { 0x31 },
	[SDL_SCANCODE_RETURN]    = { 0x32 },
	[SDL_SCANCODE_BACKSPACE] = { 0x33 },
	[SDL_SCANCODE_CAPSLOCK]  = { 0x34 },
	[SDL_SCANCODE_ESCAPE]    = { 0x35 },
	[SDL_SCANCODE_LEFT]     = { 0x36 },
	[SDL_SCANCODE_RIGHT]    = { 0x37 },
	[SDL_SCANCODE_DOWN]     = { 0x38 },
	[SDL_SCANCODE_UP]       = { 0x39 },
	[SDL_SCANCODE_LCTRL]  = { 0x3A },
	[SDL_SCANCODE_RCTRL]  = { 0x3B },
	[SDL_SCANCODE_LSHIFT] = { 0x3C },
	[SDL_SCANCODE_RSHIFT] = { 0x3D },
	[SDL_SCANCODE_LALT]   = { 0x3E },
	[SDL_SCANCODE_RALT]   = { 0x3F },
	[SDL_SCANCODE_KP_DIVIDE]   = { 0x2D },
	[SDL_SCANCODE_KP_MINUS]    = { 0x28 },
	[SDL_SCANCODE_KP_ENTER]    = { 0x32 },
	[SDL_SCANCODE_KP_0]        = { 0x01 },
	[SDL_SCANCODE_KP_1]        = { 0x02 },
	[SDL_SCANCODE_KP_2]        = { 0x03 },
	[SDL_SCANCODE_KP_3]        = { 0x04 },
	[SDL_SCANCODE_KP_4]        = { 0x05 },
	[SDL_SCANCODE_KP_5]        = { 0x06 },
	[SDL_SCANCODE_KP_6]        = { 0x07 },
	[SDL_SCANCODE_KP_7]        = { 0x08 },
	[SDL_SCANCODE_KP_8]        = { 0x09 },
	[SDL_SCANCODE_KP_9]        = { 0x0a },
	[SDL_SCANCODE_KP_PERIOD]   = { 0x2E },
};

uint8_t KbdScancodes[32];
uint8_t KbdRead = 0;
uint8_t KbdWrite = 0;

uint32_t MouseBuffer[32];
uint8_t MouseRead = 0;
uint8_t MouseWrite = 0;
int MousePressed = 0;

void KbdPush(int sdlCode, int released) {
	if(released) {
		KbdScancodes[KbdWrite] = (uint8_t)(-((int8_t)ScancodeMapping[sdlCode].code));
	} else {
		KbdScancodes[KbdWrite] = ScancodeMapping[sdlCode].code;
	}
	KbdWrite = (KbdWrite + 1) % 32;
	if(KbdRead == KbdWrite) { // Ring Buffer Error
		KbdRead = 0;
		KbdWrite = 0;
	}
	HTCInterrupt(1);
}

void MouseUpdate(int8_t relX, int8_t relY, uint32_t button) {
	MouseBuffer[MouseWrite] = ((uint32_t)((uint8_t)relX)) | (((uint32_t)((uint8_t)relY)) << 8) | (button << 16);
	MouseWrite = (MouseWrite + 1) % 32;
	if(MouseRead == MouseWrite) { // Ring Buffer Error
		MouseRead = 0;
		MouseWrite = 0;
	}
	HTCInterrupt(2);
}

int OIPBRead(uint32_t port, uint32_t length, uint32_t *value) {
    if(port == 0x10) {
		if(KbdRead != KbdWrite) {
			*value = KbdScancodes[KbdRead];
			KbdRead = (KbdRead + 1) % 32;
		} else {
			*value = 0;
		}
		return 1;
    } else if(port == 0x11) {
		if(MouseRead != MouseWrite) {
			*value = MouseBuffer[MouseRead];
			MouseRead = (MouseRead + 1) % 32;
		} else {
			*value = 0;
		}
		return 1;
	}
	return 0;
}

int OIPBWrite(uint32_t port, uint32_t length, uint32_t value) {
    if(port == 0x10) {
        return 0;
    }
	return 0;
}

void OIPBInit() {
    memset(KbdScancodes,0,32);
	memset(MouseBuffer,0,sizeof(uint32_t)*32);
    for(int i=0x10; i < 0x13; i++) {
        OkamiPorts[i].isPresent = 1;
        OkamiPorts[i].read = OIPBRead;
		OkamiPorts[i].write = OIPBWrite;
    }
}