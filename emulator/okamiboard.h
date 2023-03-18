#pragma once
#include <stdint.h>
#include "koribus.h"

typedef int (*PortReadFn)(uint32_t port, uint32_t length, uint32_t *value);
typedef int (*PortWriteFn)(uint32_t port, uint32_t length, uint32_t value);

typedef struct {
    int isPresent;
    PortReadFn read;
    PortWriteFn write;
} OkamiPort;

extern OkamiPort OkamiPorts[256];

void OkamiBoardInit();
void OkamiBoardSaveNVRAM();