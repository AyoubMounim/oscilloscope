
#pragma once

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define INLINE static inline

typedef struct {
  uint8_t *storage;
  size_t writeIndex;
  size_t readIndex;
  size_t currentSize;
  size_t maxSize;
} Buffer;

/* ============================================ Public functions declaration */

INLINE Buffer *Buffer_create(size_t const size);

INLINE size_t Buffer_writeByte(Buffer *const self, uint8_t *src);
INLINE size_t Buffer_readByte(Buffer *const self, uint8_t *dst);
INLINE size_t Buffer_readByteAsync(Buffer *const self, uint8_t *dst);

INLINE size_t Buffer_write(Buffer *const self, uint8_t *src, size_t dataSize);
INLINE size_t Buffer_read(Buffer *const self, uint8_t *dst, size_t dataSize);
INLINE size_t Buffer_readAsync(Buffer *const self, uint8_t *dst,
                               size_t dataSize);

INLINE size_t Buffer_next(Buffer *const self, uint8_t *dst, size_t dataSize);
INLINE size_t Buffer_nextAsync(Buffer *const self, uint8_t *dst,
                               size_t dataSize);

/* ============================================= Public functions definition */

Buffer *Buffer_create(size_t const size) {
  Buffer *buff = (Buffer *)calloc(1, sizeof(Buffer));
  if (!buff) {
    return NULL;
  }
  buff->storage = (uint8_t *)calloc(1, size);
  if (!buff->storage) {
    free(buff);
    return NULL;
  }
  buff->maxSize = size;
  buff->writeIndex = buff->readIndex = buff->currentSize = 0;
  return buff;
}

size_t Buffer_writeByte(Buffer *const self, uint8_t *src) {
  if (self->currentSize == self->maxSize) {
    assert(0 && "Overwriting buffer data");
  }
  self->storage[self->writeIndex] = *src;
  self->currentSize++;
  self->writeIndex++;
  if (self->writeIndex == self->maxSize) {
    self->writeIndex = 0;
  }
  return 1;
}

size_t Buffer_readByte(Buffer *const self, uint8_t *dst) { assert(0); }

size_t Buffer_readByteAsync(Buffer *const self, uint8_t *dst) {
  if (self->currentSize == 0) {
    return 0;
  }
  if (dst != NULL) {
    *dst = self->storage[self->readIndex];
  }
  self->currentSize--;
  self->readIndex++;
  if (self->readIndex == self->maxSize) {
    self->readIndex = 0;
  }
  return 1;
}

size_t Buffer_write(Buffer *const self, uint8_t *src, size_t dataSize) {
  size_t written = 0;
  for (size_t i = 0; i < dataSize; i++) {
    written += Buffer_writeByte(self, src + i);
  }
  return written;
}

size_t Buffer_read(Buffer *const self, uint8_t *dst, size_t dataSize) {
  assert(0);
}

size_t Buffer_readAsync(Buffer *const self, uint8_t *dst, size_t dataSize) {
  size_t read = 0;
  for (size_t i = 0; i < dataSize; i++) {
    if (Buffer_readByteAsync(self, dst + i) == 0) {
      break;
    }
    read++;
  }
  return read;
}

size_t Buffer_next(Buffer *const self, uint8_t *dst, size_t dataSize) {
  assert(0);
}

size_t Buffer_nextAsync(Buffer *const self, uint8_t *dst, size_t dataSize) {
  if (self->currentSize < dataSize) {
    return 0;
  }
  size_t read = 0;
  for (size_t i = 0; i < dataSize; i++) {
    uint8_t *destination = (dst != NULL) ? dst + i : NULL;
    read += Buffer_readByteAsync(self, destination);
  }
  assert(read == dataSize);
  return read;
}

#undef INLINE
