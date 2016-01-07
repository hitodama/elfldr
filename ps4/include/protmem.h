#ifndef ProtMem_H
#define ProtMem_H

typedef struct ProtectedMemory ProtectedMemory;

void *protectedMemoryWritable(ProtectedMemory *memory);
void *protectedMemoryExecutable(ProtectedMemory *memory);
size_t protectedMemorySize(ProtectedMemory *memory);

ProtectedMemory *protectedMemoryCreate(size_t size);
ProtectedMemory *protectedMemoryCreateEmulation(size_t size);
int protectedMemoryDestroy(ProtectedMemory *memory);
int protectedMemoryDestroyEmulation(ProtectedMemory *memory);

void protectedMemoryDebugPrint(ProtectedMemory *memory);

#endif
