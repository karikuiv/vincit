#pragma once

#include <stdint.h>

struct MemoryStruct {
  char *memory;
  size_t size;
};

/*static*/ size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp);
int request(char *req, struct MemoryStruct *chunk);