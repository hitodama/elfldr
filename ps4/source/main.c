#include "common.h"
#include "elfloader.h"
#include "protmem.h"
#include "util.h"
#include "debug.h"

#if defined(__PS4__) && !defined(ElfLoaderServer)
	#define ElfLoaderServer 1
#endif

/*
#if defined(BinaryLoader) && !defined(__PS4__)
	#error BinaryLoader can't be defined without __PS4__
#endif
*/

//hex(P) + hex(S) + 4
enum{ ServerPort = 5053 };
enum{ ServerRetry = 20 };
enum{ ServerTimeout = 1 };
enum{ ServerBacklog = 10 };
//enum{ PageSize = 16 * 1024 };

/* Type */
typedef int (*Runnable)(int, char **);

Elf *elfCreateFromSocket(int client)
{
	uint64_t s;
	void * m = utilAllocUnsizeableFileFromDescriptor(client, &s);
	if(m == NULL)
		return NULL;
	return elfCreate(m, s);
}

Elf *elfCreateFromFile(char *path)
{
	uint64_t s;
	void * m = utilAllocFile(path, &s);
	if(m == NULL)
		return NULL;
	return elfCreate(m, s);
}

int binaryLoaderMain(int argc, char **argv)
{
	int server, client;
	uint8_t *payload = (uint8_t *)Payload;
	ssize_t r;

	signal(SIGPIPE, SIG_IGN);

	if(debugOpen(DebugPort) < 0)
		return 1;
	debugPrint("debugOpen(%i)", DebugPort);

	debugPrint("Mode -> BinaryLoader\n");

	debugPrint("utilServerCreate(%i, %i, %i) -> ", ServerPort, ServerRetry, ServerTimeout);
	if((server = utilServerCreate(ServerPort, ServerBacklog, ServerRetry, ServerTimeout)) < 0)
		return 2;
	debugPrint("%i\n", server);

	debugPrint("accept(%i, NULL, NULL) -> ", server);
	if((client = accept(server, NULL, NULL)) < 0)
	{
		close(server);
		return 3;
	}
	debugPrint("%i\n", client);

	while((r = read(client, payload, 4096)) > 0)
	{
		debugPrint("Read %i (0x%x) bytes to %p \n", read, (uint64_t)r, (void *)payload);
		payload += r;
	}

	close(client);
	close(server);

	debugPrint("debugClose()\n");
	debugClose();

	return 4;
}

int main(int argc, char **argv)
{
	int server, client; // only used in serverMode
	int serverMode;
	Elf *elf;
	uint64_t size, alignment;
	ProtectedMemory *memory;
	Runnable run;
	int64_t ret;

	#ifdef BinaryLoader
		return binaryLoaderMain(argc, argv);
	#endif

	serverMode = 0;
	#ifdef ElfLoaderServer
		serverMode = 1;
	#endif

	signal(SIGPIPE, SIG_IGN);

	/* Get Elf from somewhere */

	if(serverMode)
	{
		if(debugOpen(DebugPort) < 0)
			return 1;
		debugPrint("debugOpen(%i)", DebugPort);

		debugPrint("Mode -> ElfLoader\n");

		debugPrint("utilServerCreate(%i, %i, %i) -> ", ServerPort, ServerRetry, ServerTimeout);
		if((server = utilServerCreate(ServerPort, ServerRetry, ServerTimeout, ServerBacklog)) < 0)
		{
			debugPrint("Server creation failed %i", server);
			return EXIT_FAILURE;
		}
		debugPrint("%i\n", server);

		debugPrint("accept(%i, NULL, NULL) -> ", server);
		if((client = accept(server, NULL, NULL)) < 0)
		{
			close(server);
			debugPrint("Accept failed %i", client);
			return EXIT_FAILURE;
		}
		debugPrint("%i\n", client);

		debugPrint("elfCreateFromSocket(%i) -> ", client);
		elf = elfCreateFromSocket(client);
		debugPrint("%p\n", (void *)elf);

		close(client);
		close(server);

		if(elf == NULL)
		{
			debugPrint("File doesn't seem to be an ELF\n");
			debugClose();
			return EXIT_FAILURE;
		}
	}
	else
	{
		if(debugOpen(0) < 0)
			return 1;
		debugPrint("debugOpen(0) -> stderr");

		debugPrint("Mode -> ElfLoader\n");

		if(argc < 1)
		{
			debugPrint("%s <ELF file>\n", argv[0]);
			debugClose();
			return EXIT_FAILURE;
		}

		debugPrint("elfCreateFromFile(%s) -> ", argv[1]);
		if((elf = elfCreateFromFile(argv[1])) == NULL)
		{
			debugPrint("File doesn't seem to be an ELF\n");
			debugClose();
			return EXIT_FAILURE;
		}
		debugPrint("%p\n", (void *)elf);
	}

	/* Got Elf here, Get Memory, Load, Run, ..., Free */

	size = elfMemorySize(elf);
	alignment = elfLargestAlignment(elf);

	debugPrint("protectedMemoryCreate(%"PRIu64", %"PRIu64") -> ", size, alignment);
	if((memory = protectedMemoryCreate(size, alignment)) == NULL)
	{
		debugPrint("Memory Setup failed\n");
		protectedMemoryDebugPrint(memory);
		debugClose();
		return EXIT_FAILURE;
	}
	debugPrint("%p\n", (void *)memory);

	// FIXME: depricate for 3 method calls
	debugPrint("elfLoaderLoad(%p, %p, %p) -> ", (void *)elf, protectedMemoryGetWritable(memory), protectedMemoryGetExecutable(memory));
	if((ret = elfLoaderLoad(elf, protectedMemoryGetWritable(memory), protectedMemoryGetExecutable(memory))) < 0)
	{
		debugPrint("Elf could not be loaded (%"PRIx64")\n", ret);
		protectedMemoryDebugPrint(memory);
		debugClose();
		return EXIT_FAILURE;
	}
	debugPrint("%"PRId64"\n", ret);

	protectedMemoryDebugPrint(memory);

	run = (Runnable)((uint8_t *)protectedMemoryGetExecutable(memory) + elfEntry(elf));
	debugPrint("run() [%p + 0x%"PRIx64" = %p] -> ", protectedMemoryGetExecutable(memory), elfEntry(elf), (void *)run);
	ret = run(0, NULL);
	debugPrint("%"PRId64" = %"PRIu64" = 0x%"PRIx64"\n", ret,  ret, ret);

	debugPrint("protectedMemoryDestroy(%p) -> ", (void *)memory);
	if((ret = protectedMemoryDestroy(memory)) < 0)
	{
		protectedMemoryDebugPrint(memory);
		debugPrint("Memory could not be completely freed (%"PRIx64")\n", ret);
	}
	debugPrint("%"PRId64"\n", ret);

	debugPrint("elfDestroyAndFree(%p)\n", (void *)elf);
	elfDestroyAndFree(elf);

	debugPrint("debugClose()\n");
	debugClose();

	return EXIT_SUCCESS;
}
