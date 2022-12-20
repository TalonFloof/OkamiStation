#ifndef _EW_FIFO_H
#define _EW_FIFO_H

#include <assert.h>
#include <stdatomic.h>
#include <stdlib.h>

typedef struct {
  atomic_bool isClearing;
  size_t head;
  size_t tail;
  size_t size;
  void** data;
} FIFOQueue;

static void* FIFORead(FIFOQueue* queue) {
  while (atomic_load(&(queue->isClearing)) != 0) {
  };
  if (queue->tail == queue->head) {
    return NULL;
  }
  void* handle = queue->data[queue->tail];
  queue->data[queue->tail] = NULL;
  queue->tail = (queue->tail + 1) % queue->size;
  return handle;
}

static int FIFOWrite(FIFOQueue* queue, void* handle) {
  if (((queue->head + 1) % queue->size) == queue->tail) {
    return -1;
  }
  queue->data[queue->head] = handle;
  queue->head = (queue->head + 1) % queue->size;
  return 0;
}

#endif