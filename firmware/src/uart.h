#ifndef _EWFM_UART_H
#define _EWFM_UART_H 1

#include <stdint.h>

void UART_PutChar(uint8_t chr);
void UART_PutString(const char* str);

#endif