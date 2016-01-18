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
	size_t size;
}
ProtectedMemory;

ProtectedMemory *protectedMemoryAllocate(size_t size)
{
	ProtectedMemory *memory;
	size_t pageSize;

	memory = (ProtectedMemory *)malloc(sizeof(ProtectedMemory));
	if(memory == NULL)
		return NULL;

	pageSize = sysconf(_SC_PAGESIZE);
	memory->size = ((size - 1) / pageSize + 1) * pageSize;

	return memory;
}

ProtectedMemory *protectedMemoryCreateEmulation(size_t size)
{
	int handle;
	ProtectedMemory *memory;

	if(size == 0)
		return NULL;

 	memory = protectedMemoryAllocate(size);
	if(memory == NULL)
		return NULL;

	//int memoryFile = open("/dev/zero", O_RDWR);
	handle = shm_open("/elfloader", O_CREAT|O_TRUNC|O_RDWR, 0755);
	if(handle < 0)
		goto e1;

	if(ftruncate(handle, memory->size) == -1)
		goto e2;

	memory->executable = mmap(NULL, memory->size, PROT_READ | PROT_EXEC, MAP_FILE | MAP_SHARED, handle, 0);
	if(memory->executable == MAP_FAILED)
		goto e2;
	memory->writable = mmap(NULL, memory->size, PROT_READ | PROT_WRITE, MAP_FILE | MAP_SHARED, handle, 0);
	if(memory->writable == MAP_FAILED)
		goto e3;

	close(handle);

	return memory;

	e3:
		munmap(memory->executable, memory->size);
	e2:
		close(handle);
		shm_unlink("/elfloader");
	e1:
		free(memory);

	return NULL;
}

#ifdef __PS4__
ProtectedMemory *protectedMemoryCreatePS4(size_t size)
{
	int executableHandle, writableHandle;
	ProtectedMemory *memory;

	if(size == 0)
		return NULL;

 	memory = protectedMemoryAllocate(size);
	if(memory == NULL)
		return NULL;

	sceKernelJitCreateSharedMemory(0, memory->size, PROT_READ | PROT_WRITE | PROT_EXEC, &executableHandle);
	if(executableHandle < 0)
		goto e1;
	sceKernelJitCreateAliasOfSharedMemory(executableHandle, PROT_READ | PROT_WRITE, &writableHandle);
	if(writableHandle < 0)
		goto e2;
	//sceKernelJitMapSharedMemory(memory->writableHandle, PROT_CPU_READ | PROT_CPU_WRITE, &writable);
	memory->executable = mmap(NULL, memory->size, PROT_READ | PROT_EXEC, MAP_SHARED, executableHandle, 0);
	if(memory->executable == MAP_FAILED)
		goto e3;
	memory->writable = mmap(NULL, memory->size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_TYPE, writableHandle, 0);
	if(memory->writable == MAP_FAILED)
		goto e4;

	close(executableHandle);
	close(writableHandle);

	return memory;

	e4:
		munmap(memory->executable, memory->size);
	e3:
		close(writableHandle);
	e2:
		close(executableHandle);
	e1:
		free(memory);

	return NULL;
}
#endif

ProtectedMemory *protectedMemoryCreatePlain(size_t size)
{
	ProtectedMemory *memory = protectedMemoryAllocate(size);
	size_t pageSize = sysconf(_SC_PAGESIZE);

	//if(!(memory->writable = aligned_alloc(memory->alignment, memory->size)))
	if(posix_memalign(&memory->writable, pageSize, memory->size) != 0)
	{
		free(memory);
		return NULL;
	}
	if(mprotect(memory->writable, memory->size, PROT_READ | PROT_WRITE | PROT_EXEC) < 0)
	{
		free(memory);
		return NULL;
	}
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
	if(memory == NULL)
		return -1;
	r |= munmap(memory->writable, memory->size);
	r |= munmap(memory->executable, memory->size);
	free(memory);
	return r;
}
#endif

int protectedMemoryDestroyEmulation(ProtectedMemory *memory)
{
	int r = 0;
	if(memory == NULL)
		return -1;
	r |= munmap(memory->writable, memory->size);
	r |= munmap(memory->executable, memory->size);
	r |= shm_unlink("/elfloader");
	//close(memoryFile);
	free(memory);
	return r;
}

int protectedMemoryDestroyPlain(ProtectedMemory *memory)
{
	if(memory == NULL)
		return -1;
	free(memory->writable);
	free(memory);
	return 0;
}

int protectedMemoryDestroy(ProtectedMemory *memory)
{
	if(memory == NULL)
		return -1;
	#if defined(__PS4__)
		return protectedMemoryDestroyPS4(memory);
	#else
		return protectedMemoryDestroyPlain(memory);
	#endif
}

void *protectedMemoryWritable(ProtectedMemory *memory)
{
	if(memory == NULL)
		return NULL;
	return memory->writable;
}

void *protectedMemoryExecutable(ProtectedMemory *memory)
{
	if(memory == NULL)
		return NULL;
	return memory->executable;
}

size_t protectedMemorySize(ProtectedMemory *memory)
{
	if(memory == NULL)
		return 0;
	return memory->size;
}
