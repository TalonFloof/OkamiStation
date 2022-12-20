#ifndef _EW_FIFO_H
#define _EW_FIFO_H

#include <assert.h>
#include <pthread.h>
#include <stdlib.h>

typedef struct {
  pthread_mutex_t mutex;
  size_t head;
  size_t tail;
  size_t size;
  void** data;
} FIFOQueue;

static void* FIFORead(FIFOQueue* queue) {
  pthread_mutex_lock(&(queue->mutex));
  if (queue->tail == queue->head) {
    pthread_mutex_unlock(&(queue->mutex));
    return NULL;
  }
  void* handle = queue->data[queue->tail];
  queue->data[queue->tail] = NULL;
  queue->tail = (queue->tail + 1) % queue->size;
  pthread_mutex_unlock(&(queue->mutex));
  return handle;
}

static int FIFOWrite(FIFOQueue* queue, void* handle) {
  pthread_mutex_lock(&(queue->mutex));
  if (((queue->head + 1) % queue->size) == queue->tail) {
    pthread_mutex_unlock(&(queue->mutex));
    return -1;
  }
  queue->data[queue->head] = handle;
  queue->head = (queue->head + 1) % queue->size;
  pthread_mutex_unlock(&(queue->mutex));
  return 0;
}

#endif