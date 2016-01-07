#define _XOPEN_SOURCE 700
#define __BSD_VISIBLE 1
#define _DEFAULT_SOURCE 1

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/mman.h>

#include "protmem.h"

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
	int writableHandle;
	int executableHandle;
	size_t size;
}
ProtectedMemory;

ProtectedMemory *protectedMemoryAllocate(size_t size)
{
	ProtectedMemory *memory;
	size_t pageSize;

	memory = (ProtectedMemory *)malloc(sizeof(ProtectedMemory));
	pageSize = sysconf(_SC_PAGESIZE);
	memory->size = (size / pageSize + 1) * pageSize;

	return memory;
}

ProtectedMemory *protectedMemoryCreateEmulation(size_t size)
{
	ProtectedMemory *memory = protectedMemoryAllocate(size);

	//int memoryFile = open("/dev/zero", O_RDWR);
	memory->executableHandle = shm_open("/elfloader", O_CREAT|O_TRUNC|O_RDWR, 0755);
	ftruncate(memory->executableHandle, memory->size);
	memory->writableHandle = memory->executableHandle;
	memory->executable = mmap(NULL, memory->size, PROT_READ | PROT_EXEC, MAP_FILE | MAP_SHARED, memory->executableHandle, 0);
	memory->writable = mmap(NULL, memory->size, PROT_READ | PROT_WRITE, MAP_FILE | MAP_SHARED, memory->writableHandle, 0);

	return memory;
}

#ifdef __PS4__
ProtectedMemory *protectedMemoryCreatePS4(size_t size)
{
	ProtectedMemory *memory = protectedMemoryAllocate(size);

	sceKernelJitCreateSharedMemory(0, memory->size, PROT_READ | PROT_WRITE | PROT_EXEC, &memory->executableHandle);
	sceKernelJitCreateAliasOfSharedMemory(memory->executableHandle, PROT_READ | PROT_WRITE, &memory->writableHandle);
	//sceKernelJitMapSharedMemory(memory->writableHandle, PROT_CPU_READ | PROT_CPU_WRITE, &writable);
	memory->executable = mmap(NULL, memory->size, PROT_READ | PROT_EXEC, MAP_SHARED, memory->executableHandle, 0);
	memory->writable = mmap(NULL, memory->size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_TYPE, memory->writableHandle, 0);

	return memory;
}
#endif

ProtectedMemory *protectedMemoryCreatePlain(size_t size)
{
	ProtectedMemory *memory = protectedMemoryAllocate(size);
	size_t pageSize = sysconf(_SC_PAGESIZE);

	//if(!(memory->writable = aligned_alloc(memory->alignment, memory->size)))
	if(posix_memalign(&memory->writable, pageSize, memory->size))
		return NULL;
	if(mprotect(memory->writable, memory->size, PROT_READ | PROT_WRITE | PROT_EXEC))
		return NULL;
	memory->executable = memory->writable;

	return memory;
}

ProtectedMemory *protectedMemoryCreate(size_t size)
{
	#ifdef __PS4__
		return protectedMemoryCreatePS4(size);
	#else
		return protectedMemoryCreatePlain(size);
	#endif
}

#if defined(__PS4__)
int protectedMemoryDestroyPS4(ProtectedMemory *memory)
{
	int r = 0;
	r |= munmap(memory->writable, memory->size);
	r |= munmap(memory->executable, memory->size);
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
	r |= munmap(memory->writable, memory->size);
	r |= munmap(memory->executable, memory->size);
	r |= shm_unlink("/elfloader");
	//close(memoryFile);
	return r;
}

int protectedMemoryDestroyPlain(ProtectedMemory *memory)
{
	free(memory->writable);
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

void *protectedMemoryWritable(ProtectedMemory *memory)
{
	return memory->writable;
}

void *protectedMemoryExecutable(ProtectedMemory *memory)
{
	return memory->executable;
}

size_t protectedMemorySize(ProtectedMemory *memory)
{
	return memory->size;
}
