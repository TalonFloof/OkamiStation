#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#if UINT_MAX < 4294967295U
#error "Target doesn't support 32-bit operations!"
#endif

void Error(long int line, long int col, char *str) {
  fprintf(stderr, "\x1b[31m%li:%li - %s\x1b[0m\n", line, col, str);
  exit(1);
}

void Assemble(char* name, char* data) {
    unsigned long int len = strlen(data);
    unsigned long int i = 0;
    unsigned long int base = 0;
    while(i<len) {
        if(data[i]) {
          
        }
    }
}

int main(int argc, char **argv) { return 0; }