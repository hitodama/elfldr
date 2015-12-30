#ifndef ProtMem_H
#define ProtMem_H

#include "elfloader.h"

typedef struct ProtectedMemory ProtectedMemory;

void *protectedMemoryGetWritable(ProtectedMemory *memory);
void *protectedMemoryGetExecutable(ProtectedMemory *memory);

ProtectedMemory *protectedMemoryCreate(uint64_t size, uint64_t alignment);
int protectedMemoryDestroy(ProtectedMemory *memory);
void protectedMemoryDebugPrint(ProtectedMemory *memory);

#endif
