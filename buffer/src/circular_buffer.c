
#include "buffer/circular_buffer.h"
#include "buffer/io_buffer.h"
#include <stdlib.h>
#include <string.h>

void CircularBuffer_init(CircularBuffer *const self, size_t const length,
                         size_t const elementSize, uint8_t *const storage) {
  self->length = length;
  self->elementSize = elementSize;
  self->storage = storage;
  self->numberElements = 0;
  self->readIndex = 0;
  self->writeIndex = 0;
  return;
}

void CircularBuffer_deinit(CircularBuffer *const self) {
  self->numberElements = 0;
  self->readIndex = 0;
  self->writeIndex = 0;
  return;
}

CircularBuffer *CircularBuffer_create(size_t const length,
                                      size_t const elementSize) {
  uint8_t *storage = calloc(1, length * elementSize);
  if (!storage) {
    return NULL;
  }
  CircularBuffer *cb = calloc(1, sizeof(CircularBuffer));
  if (!cb) {
    free(storage);
    return NULL;
  }
  CircularBuffer_init(cb, length, elementSize, storage);
  return cb;
}

void CircularBuffer_destroy(CircularBuffer *self) {
  CircularBuffer_deinit(self);
  free(self->storage);
  free(self);
  self = NULL;
  return;
}

BufferError CircularBuffer_unravel(CircularBuffer const *const self,
                                   uint8_t *const outData) {
  BufferError err = {.result = 0, .errorCode = BUFFER_ERROR_OK};
  if (self->numberElements == 0) {
    err.result = 0;
    err.errorCode = BUFFER_ERROR_EMPTY;
    return err;
  }
  if (self->readIndex < self->writeIndex) {
    memcpy(outData, &self->storage[self->readIndex * self->elementSize],
           self->numberElements * self->elementSize);
    err.result += self->numberElements * self->elementSize;
  } else {
    memcpy(outData, &self->storage[self->readIndex * self->elementSize],
           (self->length - self->readIndex) * self->elementSize);
    memcpy(outData + (self->length - self->readIndex) * self->elementSize,
           self->storage, self->writeIndex * self->elementSize);
    err.result +=
        (self->length + self->writeIndex - self->readIndex) * self->elementSize;
  }
  err.result = self->numberElements; // TODO; maybe delete this line
  err.errorCode = BUFFER_ERROR_OK;
  return err;
}
