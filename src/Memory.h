#pragma once

#ifndef WEBASSEMBLY
#include <memory.h>
#include <stdlib.h>

template<typename T>
inline T* Allocate(int size) {
  return static_cast<T*>(calloc(size, sizeof(T)));
}

inline void Deallocate(void *ptr) {
  free(ptr);
}

#include <stdio.h>
void assert(bool condition, const char *message = nullptr) {
  if (!condition) {
    if (message) {
      printf("Assertion failed: %s\n", message);
    } else {
      printf("Assertion failed\n");
    }
    abort();
  }
}
#else

#include "walloc.h"

void memcpy(void *dest, void *src, size_t n) {
  char *csrc = (char *)src;
  char *cdest = (char *)dest;
  for (int i=0; i<n; i++) cdest[i] = csrc[i];
}

// via https://codereview.stackexchange.com/questions/151019/implementing-realloc-in-c
void* realloc(void *ptr, size_t originalLength, size_t newLength) {
   if (newLength == 0)
   {
      free(ptr);
      return NULL;
   }
   else if (!ptr)
   {
      return malloc(newLength);
   }
   else if (newLength <= originalLength)
   {
      return ptr;
   }
   else
   {
      assert((ptr) && (newLength > originalLength));
      void *ptrNew = malloc(newLength);
      if (ptrNew)
      {
          memcpy(ptrNew, ptr, originalLength);
          free(ptr);
      }
      return ptrNew;
    }
}

void abort() {
  while (1) {
    // oops
  }
}

void assert(bool condition) {
  if (!condition) {
    abort();
  }
}

#endif
