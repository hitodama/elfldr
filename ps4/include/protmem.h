#ifndef ProtMem_H
#define ProtMem_H

#ifdef ElfLoaderStandalone

typedef struct PS4ProtectedMemory PS4ProtectedMemory;

void *ps4ProtectedMemoryWritableAddress(PS4ProtectedMemory *memory);
void *ps4ProtectedMemoryExecutableAddress(PS4ProtectedMemory *memory);
size_t ps4ProtectedMemorySize(PS4ProtectedMemory *memory);

PS4ProtectedMemory *ps4ProtectedMemoryCreate(size_t size);
PS4ProtectedMemory *ps4ProtectedMemoryCreateEmulation(size_t size);
int ps4ProtectedMemoryDestroy(PS4ProtectedMemory *memory);
int ps4ProtectedMemoryDestroyEmulation(PS4ProtectedMemory *memory);

void ps4ProtectedMemoryDebugPrint(PS4ProtectedMemory *memory);

#endif

#endif
