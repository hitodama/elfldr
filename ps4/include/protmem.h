#ifndef ProtMem_H
#define ProtMem_H

#include "elfldr.h"

typedef struct ProtectedMemory
{
	void *writable;
	void *executable;
	void *writableAligned;
	void *executableAligned;
	int writableHandle;
	int executableHandle;

	uint64_t size;
	uint64_t alignment;
	long pageSize;
	uint64_t alignedSize;
}
ProtectedMemory;

//void *elfLdrAlignMemory(void *mem, unsigned long alignment);
ProtectedMemory *protectedMemoryCreate(Elf *elf);
int protectedMemoryDestroy(ProtectedMemory *memory);
void protectedMemoryDebugPrint(FILE *debug, ProtectedMemory *memory);

#endif
