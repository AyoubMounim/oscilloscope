
#pragma once

#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
  uintptr_t bufferBegin;
  volatile uintptr_t writePos;
  volatile uintptr_t readPos;
  size_t bufferSize;
  pthread_mutex_t readMutex;
  pthread_cond_t readCond;
  volatile bool eof;
} IOBuffer;

typedef enum {
  BUFFER_ERROR_OK,
  BUFFER_ERROR_EMPTY,
  BUFFER_ERROR_DATA_OVERWRITE,
  BUFFER_ERROR_DATA_DROPPED,
  BUFFER_ERROR_DATA_TOO_BIG,
  BUFFER_ERROR_EOF
} BufferErrorCode;

typedef struct {
  size_t result;
  BufferErrorCode errorCode;
} BufferError;

/* ============================================ Public functions declaration */

/**
 * @brief Set the EOF flag for the buffer and wakes up sleeping threads.
 *
 * @param[in] self: IOBuffer instance.
 */
static inline void IOBuffer_setEOF(IOBuffer *const self) {
  self->eof = true;
  pthread_cond_broadcast(&self->readCond);
  return;
}

void IOBuffer_init(IOBuffer *const self, uintptr_t const storage,
                   size_t const storageSize);

void IOBuffer_deinit(IOBuffer *const self);

/**
 * @brief Resets buffer to an empty state.
 *
 * @param[in] self: IOBuffer instance.
 */
void IOBuffer_reset(IOBuffer *const self);

/**
 * @brief Creates new buffer. Allocates memory that must be freed with IOBuffer
 * destroy.
 *
 * @param[in] size  Buffer size in bytes.
 * @return IOBuffer instance, NULL if memory allocation errors.
 */
IOBuffer *IOBuffer_create(size_t const size);

/**
 * @brief Destroys instance. Frees all allocated memory during creation.
 *
 * @param[in] self: IOBuffer instance.
 */
void IOBuffer_destroy(IOBuffer *self);

/**
 * @brief Writes at most dataSize bytes from the dataSrc to the buffer. Caller
 * must make sure that dataSrc points to enough memory space. Blocks execution
 * flow.
 *
 * @param[in] self: IOBuffer instance.
 * @param[out] dataSec: Pointer to memory space to copy into buffer.
 * @param[in] dataSize: Number of bytes to write.
 *
 * @return Result of the operation. If number of written bytes is different from
 * dataSize check errorCode in BufferError.
 */
BufferError IOBuffer_write(IOBuffer *const self, uint8_t const *const dataSrc,
                           size_t const dataSize);

/**
 * @brief Reads at most dataSize bytes from the buffer to dataDst. Caller must
 * make sure that dataDst points to enough memory space. EOF is signaled by
 * errorCode in the returned value. Blocks execution flow.
 *
 * @param[in] self: IOBuffer instance.
 * @param[out] dataDst: Pointer to memory space to fill with data.
 * @param[in] dataSize: Number of bytes to read.
 *
 * @return Result of the operation. Caller can use the data depending on the
 * errorCode in BufferError.
 */
BufferError IOBuffer_read(IOBuffer *const self, uint8_t *const dataDst,
                          size_t const dataSize);

/**
 * @brief Same as IOBuffer_read but does not block execution flow.
 *
 * @param[in] self: IOBuffer instance.
 * @param[out] dataDst: Pointer to memory space to fill with data.
 * @param[in] dataSize: Number of bytes to read.
 *
 * @return Result of the operation. Caller can use the data depending on the
 * error code in BufferError.
 */
BufferError IOBuffer_readAsync(IOBuffer *const self, uint8_t *const dataDst,
                               size_t const dataSize);

/**
 * @brief Reads exactly dataSize bytes form the buffer. Number of bytes read is
 * different form dataSize only if EOF is reached. Caller must
 * make sure that dataDst points to enough memory space. Blocks execution flow.
 *
 * @param[in] self
 * @param[out] dataDst: Pointer to memory space to fill with data.
 * @param[in] dataSize: number of bytes to read.
 *
 * @return Result of the operation. Caller can use the data depending on the
 * error code in BufferError.
 */
BufferError IOBuffer_next(IOBuffer *const self, uint8_t *const dataDst,
                          size_t const dataSize);

/**
 * @brief Same as IOBuffer_next but does not block execution flow. If not enough
 * data in buffer and EOF not reached reads 0 bytes.
 *
 * @param[in] self: IOBuffer instance.
 * @param[out] dataDst: Pointer to memory space to fill with data.
 * @param[in] dataSize: Number of bytes to read.
 *
 * @return Result of the operation. Caller can use the data depending on the
 * error code in BufferError.
 */
BufferError IOBuffer_nextAsync(IOBuffer *const self, uint8_t *const dataDst,
                               size_t const dataSize);
