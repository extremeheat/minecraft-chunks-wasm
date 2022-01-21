#pragma once

#ifndef WEBASSEMBLY
#include <assert.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>

template <typename T>
inline T *Allocate(int size) {
  return static_cast<T *>(calloc(size, sizeof(T)));
}

inline void Deallocate(void *ptr) { free(ptr); }

void *reallocate(void *ptr, size_t old_size, size_t new_size) {
  return realloc(ptr, new_size);
}

#define WASM_EXPORT

#endif

#ifdef WEBASSEMBLY
#include "walloc.h"

extern "C" {

void* memset(void* dest, int val, size_t len) {
  auto ptr = (unsigned char*)dest;
  while (len-- > 0)
    *ptr++ = val;
  return dest;
}

void abort() {
  while (1) {
    // oops
  }
}

void assert(bool condition, const char *message = 0) {
  if (!condition) {
    // abort();
  }
}

void* memcpy(void *dest, void *src, size_t n) {
  char *csrc = (char *)src;
  char *cdest = (char *)dest;
  for (int i = 0; i < n; i++) cdest[i] = csrc[i];
  return dest;
}

// via
// https://codereview.stackexchange.com/a/151038
void *realloc(void *ptr, size_t originalLength, size_t newLength) {
  if (newLength == 0) {
    free(ptr);
    return NULL;
  } else if (!ptr) {
    return malloc(newLength);
  } else if (newLength <= originalLength) {
    return ptr;
  } else {
    assert((ptr) && (newLength > originalLength));
    void *ptrNew = malloc(newLength);
    if (ptrNew) {
      memcpy(ptrNew, ptr, originalLength);
      free(ptr);
    }
    return ptrNew;
  }
}
}

#define reallocate realloc

template <typename T>
inline T *Allocate(int size) {
  auto allocSize = size * sizeof(T);
  auto allocated = (char *)malloc(allocSize);
  // zero out the memory
  for (int i = 0; i < allocSize; i++) {
    allocated[i] = 0;
  }
  return reinterpret_cast<T *>(allocated);
}

inline void Deallocate(void *ptr) { free(ptr); }

#define printf(...)

void* operator new(unsigned long size) {
  void *ptr = malloc(size);
  return ptr;
}

#define WASM_EXPORT __attribute__((visibility("default")))

#endif
