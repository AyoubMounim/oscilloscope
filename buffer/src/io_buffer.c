
#include "buffer/io_buffer.h"
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define dassert(exp) assert(exp)

void IOBuffer_init(IOBuffer *const self, uintptr_t const storage,
                   size_t const storageSize) {
  self->bufferBegin = storage;
  self->bufferSize = storageSize;
  self->readPos = 0;
  self->writePos = 0;
  self->eof = false;
  pthread_mutex_init(&self->readMutex, NULL);
  pthread_cond_init(&self->readCond, NULL);
  return;
}

void IOBuffer_deinit(IOBuffer *const self) {
  self->readPos = 0;
  self->writePos = 0;
  self->eof = false;
  return;
}

void IOBuffer_reset(IOBuffer *const self) {
  self->readPos = 0;
  self->writePos = 0;
  self->eof = false;
  return;
}

IOBuffer *IOBuffer_create(size_t const size) {
  uint8_t *storage = calloc(1, size);
  if (!storage) {
    return NULL;
  }
  IOBuffer *buff = calloc(1, sizeof(IOBuffer));
  if (!buff) {
    return NULL;
  }
  IOBuffer_init(buff, (uintptr_t)storage, size);
  return buff;
}

void IOBuffer_destroy(IOBuffer *self) {
  if (!self) {
    return;
  }
  IOBuffer_deinit(self);
  free((void *)self->bufferBegin);
  pthread_mutex_destroy(&self->readMutex);
  // pthread_cond_destroy(&self->readCond); // TODO: find out how to destroy the
  // cond variable.
  self = NULL;
  return;
}

BufferError IOBuffer_write(IOBuffer *const self, uint8_t const *const dataSrc,
                           size_t const dataSize) {
  BufferError err = {.result = 0, .errorCode = BUFFER_ERROR_OK};
  if (dataSize > self->bufferSize) {
    err.errorCode = BUFFER_ERROR_DATA_TOO_BIG;
    return err;
  }
  if (self->writePos < self->readPos) {
    size_t dataToWrite = (dataSize <= self->readPos - self->writePos - 1)
                             ? dataSize
                             : self->readPos - self->writePos - 1;
    memcpy((void *)(self->bufferBegin + self->writePos), dataSrc, dataToWrite);
    self->writePos += dataToWrite;
    err.result = dataToWrite;
    err.errorCode =
        (dataToWrite == dataSize) ? BUFFER_ERROR_OK : BUFFER_ERROR_DATA_DROPPED;
  } else {
    uintptr_t overflow = 0;
    size_t bytesWritter = 0;
    if ((self->writePos + dataSize) > self->bufferSize) {
      overflow = self->writePos + dataSize - self->bufferSize;
    }
    memcpy((void *)(self->bufferBegin + self->writePos), dataSrc,
           dataSize - overflow);
    bytesWritter += dataSize - overflow;
    if (self->readPos == 0) {
      overflow = 0;
    } else {
      overflow = (overflow <= self->readPos - 1) ? overflow : self->readPos - 1;
    }
    memcpy((void *)(self->bufferBegin), dataSrc + bytesWritter, overflow);
    bytesWritter += overflow;
    self->writePos += bytesWritter;
    self->writePos = (self->writePos < self->bufferSize)
                         ? self->writePos
                         : self->writePos - self->bufferSize;
    err.result = bytesWritter;
    err.errorCode = (bytesWritter == dataSize) ? BUFFER_ERROR_OK
                                               : BUFFER_ERROR_DATA_DROPPED;
  }
  pthread_cond_broadcast(&self->readCond);
  return err;
}

BufferError IOBuffer_read(IOBuffer *const self, uint8_t *const dataDst,
                          size_t const dataSize) {
  BufferError err = {.result = 0, .errorCode = BUFFER_ERROR_OK};
  while (self->readPos == self->writePos) {
    if (self->eof) {
      err.result = 0;
      err.errorCode = BUFFER_ERROR_EOF;
      return err;
    }
    pthread_mutex_lock(&self->readMutex);
    pthread_cond_wait(&self->readCond, &self->readMutex);
    pthread_mutex_unlock(&self->readMutex);
  }
  if (self->readPos < self->writePos) {
    // read operations
    size_t sizeToRead = (self->writePos - self->readPos < dataSize)
                            ? self->writePos - self->readPos
                            : dataSize;
    memcpy(dataDst, (void *)(self->bufferBegin + self->readPos), sizeToRead);
    // update state
    self->readPos += sizeToRead;
    err.result = sizeToRead;
    err.errorCode = BUFFER_ERROR_OK;
  } else {
    // read operations
    uintptr_t overflow = 0;
    size_t bytesRead = 0;
    if ((self->readPos + dataSize) > self->bufferSize) {
      overflow = self->readPos + dataSize - self->bufferSize;
    }
    memcpy(dataDst, (void *)(self->bufferBegin + self->readPos),
           dataSize - overflow);
    bytesRead += dataSize - overflow;
    overflow = (overflow <= self->writePos) ? overflow : self->writePos;
    memcpy(dataDst + bytesRead, (void *)(self->bufferBegin), overflow);
    bytesRead += overflow;
    self->readPos += bytesRead;
    self->readPos = (self->readPos < self->bufferSize)
                        ? self->readPos
                        : self->readPos - self->bufferSize;
    err.result = bytesRead;
    err.errorCode = BUFFER_ERROR_OK;
  }
  return err;
}

BufferError IOBuffer_readAsync(IOBuffer *const self, uint8_t *const dataDst,
                               size_t const dataSize) {
  BufferError err = {.result = 0, .errorCode = BUFFER_ERROR_OK};
  if (self->readPos == self->writePos) {
    err.result = 0;
    err.errorCode = (self->eof) ? BUFFER_ERROR_EOF : BUFFER_ERROR_EMPTY;
    return err;
  }
  if (self->readPos < self->writePos) {
    size_t sizeToRead = (self->writePos - self->readPos < dataSize)
                            ? self->writePos - self->readPos
                            : dataSize;
    memcpy(dataDst, (void *)(self->bufferBegin + self->readPos), sizeToRead);
    self->readPos += sizeToRead;
    err.result = sizeToRead;
    err.errorCode = BUFFER_ERROR_OK;
  } else {
    uintptr_t overflow = 0;
    if ((self->readPos + dataSize) > self->bufferSize) {
      overflow = self->readPos + dataSize - self->bufferSize;
    }
    memcpy(dataDst, (void *)(self->bufferBegin + self->readPos),
           dataSize - overflow);
    self->readPos += dataSize - overflow;
    self->readPos = (self->readPos == self->bufferSize) ? 0 : self->readPos;
    err.result = dataSize - overflow;
    if (overflow) {
      size_t sizeToRead =
          (self->writePos < dataSize) ? self->writePos : overflow;
      memcpy(dataDst + dataSize - overflow, (void *)(self->bufferBegin),
             sizeToRead);
      self->readPos = sizeToRead;
      err.result += sizeToRead;
      err.errorCode = BUFFER_ERROR_OK;
    }
  }
  return err;
}

BufferError IOBuffer_next(IOBuffer *const self, uint8_t *const dataDst,
                          size_t const dataSize) {
  BufferError res = {.result = 0, .errorCode = BUFFER_ERROR_OK};
  size_t read = 0;
  while (true) {
    BufferError err = IOBuffer_read(self, dataDst + read, dataSize - read);
    if (err.errorCode == BUFFER_ERROR_EOF) {
      read += err.result;
      res.errorCode = BUFFER_ERROR_EOF;
      break;
    }
    if (err.errorCode != BUFFER_ERROR_OK) {
      continue;
    }
    read += err.result;
    if (read == dataSize) {
      res.errorCode = BUFFER_ERROR_OK;
      break;
    }
  }
  res.result = read;
  return res;
}

BufferError IOBuffer_nextAsync(IOBuffer *const self, uint8_t *const dataDst,
                               size_t const dataSize) {
  BufferError res = {.result = 0, .errorCode = BUFFER_ERROR_OK};
  BufferError err = IOBuffer_readAsync(self, dataDst, dataSize);
  res.errorCode = err.errorCode;
  if (err.errorCode == BUFFER_ERROR_EOF) {
    res.result = err.result;
  } else if (err.errorCode == BUFFER_ERROR_OK && err.result == dataSize) {
    res.result = dataSize;
  }
  return res;
}
