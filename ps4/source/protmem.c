#ifdef ElfLoaderStandalone

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

typedef struct PS4ProtectedMemory
{
	void *writable;
	void *executable;
	size_t size;
}
PS4ProtectedMemory;

PS4ProtectedMemory *ps4ProtectedMemoryAllocate(size_t size)
{
	PS4ProtectedMemory *memory;
	size_t pageSize;

	memory = (PS4ProtectedMemory *)malloc(sizeof(PS4ProtectedMemory));
	if(memory == NULL)
		return NULL;

	pageSize = sysconf(_SC_PAGESIZE);
	memory->size = ((size - 1) / pageSize + 1) * pageSize;

	return memory;
}

PS4ProtectedMemory *ps4ProtectedMemoryCreateEmulation(size_t size)
{
	int handle;
	PS4ProtectedMemory *memory;

	if(size == 0)
		return NULL;

 	memory = ps4ProtectedMemoryAllocate(size);
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
PS4ProtectedMemory *ps4ProtectedMemoryCreatePS4(size_t size)
{
	int executableHandle, writableHandle;
	PS4ProtectedMemory *memory;

	if(size == 0)
		return NULL;

 	memory = ps4ProtectedMemoryAllocate(size);
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

PS4ProtectedMemory *ps4ProtectedMemoryCreatePlain(size_t size)
{
	PS4ProtectedMemory *memory = ps4ProtectedMemoryAllocate(size);
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

PS4ProtectedMemory *ps4ProtectedMemoryCreate(size_t size)
{
	#ifdef __PS4__
		return ps4ProtectedMemoryCreatePS4(size);
	#else
		return ps4ProtectedMemoryCreatePlain(size);
	#endif
}

#if defined(__PS4__)
int ps4ProtectedMemoryDestroyPS4(PS4ProtectedMemory *memory)
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

int ps4ProtectedMemoryDestroyEmulation(PS4ProtectedMemory *memory)
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

int ps4ProtectedMemoryDestroyPlain(PS4ProtectedMemory *memory)
{
	if(memory == NULL)
		return -1;
	free(memory->writable);
	free(memory);
	return 0;
}

int ps4ProtectedMemoryDestroy(PS4ProtectedMemory *memory)
{
	if(memory == NULL)
		return -1;
	#if defined(__PS4__)
		return üps4ProtectedMemoryDestroyPS4(memory);
	#else
		return ps4ProtectedMemoryDestroyPlain(memory);
	#endif
}

void *ps4ProtectedMemoryWritableAddress(PS4ProtectedMemory *memory)
{
	if(memory == NULL)
		return NULL;
	return memory->writable;
}

void *ps4ProtectedMemoryExecutableAddress(PS4ProtectedMemory *memory)
{
	if(memory == NULL)
		return NULL;
	return memory->executable;
}

size_t ps4ProtectedMemorySize(PS4ProtectedMemory *memory)
{
	if(memory == NULL)
		return 0;
	return memory->size;
}

#endif
