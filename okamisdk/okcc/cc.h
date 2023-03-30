#ifndef _CC_H
#define _CC_H
#include <stdio.h>

void ccErr(char* file, unsigned long line, unsigned long col, char* err);
char* readFile(char* path, unsigned long* size);

#endif