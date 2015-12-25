#include "common.h"
#include "protmem.h"
#include "debug.h"

#ifdef __PS4__
	#include <kernel.h>
#endif

#ifndef MAP_TYPE
	#define MAP_TYPE 0x0f
#endif

void *protectedMemoryAlign(void *mem, unsigned long alignment)
{
	return (void *)((uintptr_t)((uint8_t *)mem + alignment + sizeof(void *)) & ~(alignment - 1));
}

ProtectedMemory *protectedMemoryCreate(Elf *elf)
{
	ProtectedMemory *memory;

	if(elf == NULL)
		return NULL;

	memory = (ProtectedMemory *)calloc(1, sizeof(ProtectedMemory));

	//FIXME: MANY MORE ERROR STATES, oh so many more ... Oo

	memory->size = elfMemorySize(elf);
	memory->alignment = elfLargestAlignment(elf);
	memory->pageSize = sysconf(_SC_PAGESIZE); //PageSize; // getpagesize(); //sysconf(_SC_PAGESIZE);
	memory->alignedSize = ((memory->size + memory->alignment) / memory->pageSize + 1) * memory->pageSize;

	#if defined(__PS4__) || defined(ElfLdrEmulatePS4Memory)
		#if defined(__PS4__)
			sceKernelJitCreateSharedMemory(0, memory->alignedSize, PROT_READ | PROT_WRITE | PROT_EXEC, &memory->executableHandle);
			sceKernelJitCreateAliasOfSharedMemory(memory->executableHandle, PROT_READ | PROT_WRITE, &memory->writableHandle);
			//sceKernelJitMapSharedMemory(memory->writableHandle, PROT_CPU_READ | PROT_CPU_WRITE, &writable);
			memory->executable = mmap(NULL, memory->alignedSize, PROT_READ | PROT_EXEC, MAP_SHARED, memory->executableHandle, 0);
			memory->writable = mmap(NULL, memory->alignedSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_TYPE, memory->writableHandle, 0);
		#else
			//int memoryFile = open("/dev/zero", O_RDWR);
			memory->executableHandle = shm_open("/elfldr", O_CREAT|O_TRUNC|O_RDWR, 0755);
			ftruncate(memory->executableHandle, memory->alignedSize);
			memory->writableHandle = memory->executableHandle;
			memory->executable = mmap(NULL, memory->alignedSize, PROT_READ | PROT_EXEC, MAP_FILE | MAP_SHARED, memory->executableHandle, 0);
			memory->writable = mmap(NULL, memory->alignedSize, PROT_READ | PROT_WRITE, MAP_FILE | MAP_SHARED, memory->writableHandle, 0);
		#endif

		memory->executableAligned = protectedMemoryAlign(memory->executable, memory->alignment);
		memory->writableAligned = (void *)((uintptr_t)memory->writable + ((uintptr_t)memory->executableAligned - (uintptr_t)memory->executable));
	#else
		//if(!(writable = aligned_alloc(memory->alignment, memory->size)))
		if(posix_memalign(&memory->writableAligned, memory->alignment, memory->size))
			return NULL;
		if(mprotect(memory->writableAligned, memory->size, PROT_READ | PROT_WRITE | PROT_EXEC))
			return NULL;
		memory->executableAligned = memory->writableAligned;
	#endif

	return memory;
}

int protectedMemoryDestroy(ProtectedMemory *memory)
{
	#if defined(__PS4__) || defined(ElfLdrEmulatePS4Memory)
		//FIXME unmap writable executable
		//int munmap(void *addr, size_t len);
		#if defined(__PS4__)
			//FIXME unjit ...
		#else
			shm_unlink("/elfldr");
			//close(memoryFile);
		#endif
	#else
		free(memory->writableAligned);
	#endif
	free(memory);
	return 1;
}

void protectedMemoryDebugPrint(FILE *debug, ProtectedMemory *memory)
{
	if(memory == NULL)
		return;

	debugPrint(debug, "struct memory [%p]\n{\n", (void *)memory);
	debugPrint(debug, "\twritable -> %p\n", memory->writable);
	debugPrint(debug, "\texecutable -> %p\n", memory->executable);
	debugPrint(debug, "\twritableAligned -> %p\n", memory->writableAligned);
	debugPrint(debug, "\texecutableAligned -> %p\n", memory->executableAligned);
	debugPrint(debug, "\twritableHandle -> %i\n", memory->writableHandle);
	debugPrint(debug, "\texecutableHandle -> %i\n", memory->executableHandle);

	debugPrint(debug, "\tsize -> %"PRIu64" = 0x%"PRIx64"\n", memory->size, memory->size);
	debugPrint(debug, "\talignment -> %"PRIu64" = 0x%"PRIx64"\n", memory->alignment, memory->alignment);
	debugPrint(debug, "\tpageSize -> %ld = 0x%lX\n", memory->pageSize, memory->pageSize);
	debugPrint(debug, "\talignedSize -> %"PRIu64" = 0x%"PRIx64"\n", memory->alignedSize, memory->alignedSize);
	debugPrint(debug, "}\n");
}
