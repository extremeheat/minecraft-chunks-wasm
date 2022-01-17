#pragma once

#ifdef WEBASSEMBLY
extern "C" {
typedef __SIZE_TYPE__ size_t;
void* malloc(size_t size);
void free(void *ptr);
}
#define NULL 0
#endif