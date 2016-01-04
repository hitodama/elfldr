#include "common.h"
#include "protmem.h"
#include "util.h"
#include "debug.h"

#ifdef __PS4__
	#include <kernel.h>
#endif

#ifndef MAP_TYPE
	#define MAP_TYPE 0x0f
#endif

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

void *protectedMemoryAlign(void *mem, unsigned long alignment)
{
	return (void *)((uintptr_t)((uint8_t *)mem + alignment + sizeof(void *)) & ~(alignment - 1));
}

//FIXME: MANY MORE ERROR STATES, oh so many more ... Oo

void protectedMemorySetCommon(ProtectedMemory *memory, uint64_t size, uint64_t alignment)
{
	memory->size = size;
	memory->alignment = alignment;
	memory->pageSize = sysconf(_SC_PAGESIZE);
	memory->alignedSize = ((memory->size + memory->alignment) / memory->pageSize + 1) * memory->pageSize;
}

ProtectedMemory *protectedMemoryCreateEmulation(uint64_t size, uint64_t alignment)
{
	ProtectedMemory *memory;

	memory = (ProtectedMemory *)calloc(1, sizeof(ProtectedMemory));
	protectedMemorySetCommon(memory, size, alignment);

	//int memoryFile = open("/dev/zero", O_RDWR);
	memory->executableHandle = shm_open("/elfloader", O_CREAT|O_TRUNC|O_RDWR, 0755);
	ftruncate(memory->executableHandle, memory->alignedSize);
	memory->writableHandle = memory->executableHandle;
	memory->executable = mmap(NULL, memory->alignedSize, PROT_READ | PROT_EXEC, MAP_FILE | MAP_SHARED, memory->executableHandle, 0);
	memory->writable = mmap(NULL, memory->alignedSize, PROT_READ | PROT_WRITE, MAP_FILE | MAP_SHARED, memory->writableHandle, 0);
	memory->executableAligned = protectedMemoryAlign(memory->executable, memory->alignment);
	memory->writableAligned = (void *)((uintptr_t)memory->writable + ((uintptr_t)memory->executableAligned - (uintptr_t)memory->executable));

	return memory;
}

#ifdef __PS4__
ProtectedMemory *protectedMemoryCreatePS4(uint64_t size, uint64_t alignment)
{
	ProtectedMemory *memory;

	memory = (ProtectedMemory *)calloc(1, sizeof(ProtectedMemory));
	protectedMemorySetCommon(memory, size, alignment);

	sceKernelJitCreateSharedMemory(0, memory->alignedSize, PROT_READ | PROT_WRITE | PROT_EXEC, &memory->executableHandle);
	sceKernelJitCreateAliasOfSharedMemory(memory->executableHandle, PROT_READ | PROT_WRITE, &memory->writableHandle);
	//sceKernelJitMapSharedMemory(memory->writableHandle, PROT_CPU_READ | PROT_CPU_WRITE, &writable);
	memory->executable = mmap(NULL, memory->alignedSize, PROT_READ | PROT_EXEC, MAP_SHARED, memory->executableHandle, 0);
	memory->writable = mmap(NULL, memory->alignedSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_TYPE, memory->writableHandle, 0);
	memory->executableAligned = protectedMemoryAlign(memory->executable, memory->alignment);
	memory->writableAligned = (void *)((uintptr_t)memory->writable + ((uintptr_t)memory->executableAligned - (uintptr_t)memory->executable));

	return memory;
}
#endif

ProtectedMemory *protectedMemoryCreatePlain(uint64_t size, uint64_t alignment)
{
	ProtectedMemory *memory;

	memory = (ProtectedMemory *)calloc(1, sizeof(ProtectedMemory));
	protectedMemorySetCommon(memory, size, alignment);

	//if(!(memory->writableAligned = aligned_alloc(memory->alignment, memory->size)))
	if(posix_memalign(&memory->writableAligned, memory->alignment, memory->size))
		return NULL;
	if(mprotect(memory->writableAligned, memory->size, PROT_READ | PROT_WRITE | PROT_EXEC))
		return NULL;
	memory->executableAligned = memory->writableAligned;

	return memory;
}

ProtectedMemory *protectedMemoryCreate(uint64_t size, uint64_t alignment)
{
	#ifdef __PS4__
		return protectedMemoryCreatePS4(size, alignment);
	#else
		return protectedMemoryCreatePlain(size, alignment);
	#endif
}

#if defined(__PS4__)
int protectedMemoryDestroyPS4(ProtectedMemory *memory)
{
	int r = 0;
	r |= munmap(memory->writable, memory->alignedSize);
	r |= munmap(memory->executable, memory->alignedSize);
	if(close(memory->writableHandle) == EOF)
		r = -1;
	if(close(memory->executableHandle) == EOF)
		r = -1;
	return r;
}
#endif

int protectedMemoryDestroyEmulation(ProtectedMemory *memory)
{
	int r = 0;
	r |= munmap(memory->writable, memory->alignedSize);
	r |= munmap(memory->executable, memory->alignedSize);
	r |= shm_unlink("/elfloader");
	//close(memoryFile);
	return r;
}

int protectedMemoryDestroyPlain(ProtectedMemory *memory)
{
	free(memory->writableAligned);
	return 0;
}

int protectedMemoryDestroy(ProtectedMemory *memory)
{
	int r;
	#if defined(__PS4__)
		r = protectedMemoryDestroyPS4(memory);
	#else
		r = protectedMemoryDestroyPlain(memory);
	#endif
	free(memory);
	return r;
}

void *protectedMemoryGetWritable(ProtectedMemory *memory)
{
	return memory->writableAligned;
}

void *protectedMemoryGetExecutable(ProtectedMemory *memory)
{
	return memory->executableAligned;
}

void protectedMemoryDebugPrint(ProtectedMemory *memory)
{
	if(memory == NULL)
		return;

	debugPrint("struct ProtectedMemory [%p]\n{\n", (void *)memory);
	debugPrint("\twritable -> %p\n", memory->writable);
	debugPrint("\texecutable -> %p\n", memory->executable);
	debugPrint("\twritableAligned -> %p\n", memory->writableAligned);
	debugPrint("\texecutableAligned -> %p\n", memory->executableAligned);
	debugPrint("\twritableHandle -> %i\n", memory->writableHandle);
	debugPrint("\texecutableHandle -> %i\n", memory->executableHandle);

	debugPrint("\tsize -> %"PRIu64" = 0x%"PRIx64"\n", memory->size, memory->size);
	debugPrint("\talignment -> %"PRIu64" = 0x%"PRIx64"\n", memory->alignment, memory->alignment);
	debugPrint("\tpageSize -> %ld = 0x%lX\n", memory->pageSize, memory->pageSize);
	debugPrint("\talignedSize -> %"PRIu64" = 0x%"PRIx64"\n", memory->alignedSize, memory->alignedSize);
	debugPrint("}\n");
}
