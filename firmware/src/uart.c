#include "uart.h"

void UART_PutChar(uint8_t chr) { *((uint8_t*)0x11000000) = chr; }

void UART_PutString(const char* str) {
  while (*str != 0) {
    UART_PutChar((uint8_t)(*str++));
  }
}