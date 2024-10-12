
#pragma once

#include "io_buffer.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define INLINE static inline

typedef struct {
  size_t length;
  size_t elementSize;
  size_t writeIndex;
  size_t readIndex;
  size_t numberElements;
  uint8_t *storage;
} CircularBuffer;

/* ============================================ Public functions declaration */

void CircularBuffer_init(CircularBuffer *const self, size_t const length,
                         size_t const elementSize, uint8_t *const storage);

void CircularBuffer_deinit(CircularBuffer *const self);

/**
 * @brief Creates new buffer. Allocates memory that must be freed with IOBuffer
 * destroy.
 *
 * @param[in] length: Max number of elements that can fit in the buffer.
 * @param[in] elementSize: Size in bytes of one buffer element.
 * @return CircularBuffer instance, NULL if memory allocation errors.
 */
CircularBuffer *CircularBuffer_create(size_t const length,
                                      size_t const elementSize);

/**
 * @brief Destroys instance. Frees all allocated memory during creation.
 *
 * @param[in] self: CircularBuffer instance.
 */
void CircularBuffer_destroy(CircularBuffer *self);

/**
 * @brief Returns number of elements currently stored in buffer.
 *
 * @param[in] self: CircularBuffer instance.
 * @return Number of elements currently stored in buffer.
 */
INLINE size_t
CircularBuffer_getNumberElements(CircularBuffer const *const self);

/**
 * @brief Returns size in bytes of one buffer element.
 *
 * @param[in] self: CircularBuffer instance.
 * @return Size in bytes of one buffer element.
 */
INLINE size_t CircularBuffer_getElementSize(CircularBuffer const *const self);

/**
 * @brief Pushes one element into the buffer. Caller must make sure that
 * element points to enough memory. If element is NULL the buffer's element
 * slot is filled with zeroes.
 *
 * @param[in] self: CircularBuffer instance.
 * @param[in] element: Pointer to element to push.
 * @return Result of the operation. If number of written bytes is different
 * from size of the element check errorCode in BufferError.
 */
INLINE BufferError CircularBuffer_push(CircularBuffer *const self,
                                       uint8_t const *const element);

/**
 * @brief Pops one element form the buffer into elementDst. Caller must make
 * sure that elementDst points to enough memory. If elementDst is NULL the
 * element is symply discarded.
 *
 * @param[in] self: CircularBuffer instance.
 * @param[out] elementDst: Pointer to memory to fill with element to pop.
 * @return Result of the operation. If number of read bytes is different from
 * size of the element check errorCode in BufferError.
 */
INLINE BufferError CircularBuffer_pop(CircularBuffer *const self,
                                      uint8_t *const elementDst);

/**
 * @brief Copies the linearized data in the buffer to an output buffer.
 * Linearized data is all data between buffer's head and tail. Caller must
 * make sure that elementDst points to enough memory.
 *
 * @param[in] self: CircularBuffer instance.
 * @param[out] outData: Pointer to memory to fill with buffer's elements.
 * @return Result of the operation. If number of read bytes is different from
 * size of the element times the number of elements in the buffer check
 * errorCode in BufferError.
 */
BufferError CircularBuffer_unravel(CircularBuffer const *const self,
                                   uint8_t *const outData);

/**
 * @brief Drops all buffer's element.
 *
 * @param[in] self: CircularBuffer instance.
 */
INLINE void CircularBuffer_flush(CircularBuffer *const self);

/**
 * @brief Drops N buffer elements. If N is greater than current number of
 * elements drops all elements.
 *
 * @param[in] self: CircularBuffer instance.
 * @param[in] N: Number of buffer elements to drop.
 */
INLINE void CircularBuffer_flushN(CircularBuffer *const self, uint32_t const N);

/* ============================================== Public functions definition*/

INLINE BufferError CircularBuffer_push(CircularBuffer *const self,
                                       uint8_t const *const element) {
  if (element) {
    memcpy(&self->storage[self->writeIndex * self->elementSize], element,
           self->elementSize);
  } else {
    memset(&self->storage[self->writeIndex * self->elementSize], 0,
           self->elementSize);
  }
  self->writeIndex++;
  self->writeIndex = (self->writeIndex == self->length) ? 0 : self->writeIndex;
  self->numberElements++;
  return (BufferError){.result = self->elementSize,
                       .errorCode = BUFFER_ERROR_OK};
}

INLINE BufferError CircularBuffer_pop(CircularBuffer *const self,
                                      uint8_t *const elementDst) {
  BufferError err = {.result = 0, .errorCode = BUFFER_ERROR_OK};
  if (self->numberElements == 0) {
    err.result = 0;
    err.errorCode = BUFFER_ERROR_EMPTY;
    return err;
  }
  if (elementDst) {
    memcpy(elementDst, &self->storage[self->readIndex * self->elementSize],
           self->elementSize);
  }
  self->readIndex++;
  self->readIndex = (self->readIndex == self->length) ? 0 : self->readIndex;
  self->numberElements--;
  err.result = self->elementSize;
  err.errorCode = BUFFER_ERROR_OK;
  return err;
}

INLINE size_t
CircularBuffer_getNumberElements(CircularBuffer const *const self) {
  return self->numberElements;
}

INLINE size_t CircularBuffer_getElementSize(CircularBuffer const *const self) {
  return self->elementSize;
}

INLINE void CircularBuffer_flush(CircularBuffer *const self) {
  self->readIndex = 0;
  self->writeIndex = 0;
  self->numberElements = 0;
  return;
}

INLINE void CircularBuffer_flushN(CircularBuffer *const self,
                                  uint32_t const N) {
  if (N >= self->numberElements) {
    CircularBuffer_flush(self);
    return;
  }
  self->readIndex += N;
  self->readIndex %= self->length;
  self->numberElements -= N;
  return;
}

#undef INLINE
