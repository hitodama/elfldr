#include "common.h"
#include "elfloader.h"
#include "protmem.h"
#include "util.h"
#include "debug.h"

#if defined(BinaryLoader) && !defined(__PS4__)
	#error BinaryLoader can currently not be build on x86-64
#endif

//enum{ DebugPort = 5052 };
//hex(P) + hex(S)
enum{ ServerPort = 5053 };
enum{ ServerRetry = 20 };
enum{ ServerTimeout = 1 };
enum{ ServerBacklog = 10 };
//enum{ PageSize = 16 * 1024 };
enum{ MemoryModePlain = 0, MemoryModeEmulate = 1 };
enum{ ElfInputModeFile = 0, ElfInputModeServer = 1 };
enum{ DebugModeOff = 0, DebugModeOn = 1 };

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

#ifdef BinaryLoader
int main(int argc, char **argv)
#else
int binaryLoaderMain(int argc, char **argv)
#endif
{
	FILE *debug;
	int server, client;
	uint8_t *payload = (uint8_t *)Payload;
	ssize_t r;

	signal(SIGPIPE, SIG_IGN);

	if((debug = utilPrintServer(DebugPort)) == NULL)
		return 1;
	debugPrint(debug, "debugOpen(%i)", DebugPort);

	debugPrint(debug, "Mode -> BinaryLoader\n");

	debugPrint(debug, "utilServerCreate(%i, %i, %i) -> ", ServerPort, ServerRetry, ServerTimeout);
	if((server = utilServerCreate(ServerPort, ServerBacklog, ServerRetry, ServerTimeout)) < 0)
		return 2;
	debugPrint(debug, "%i\n", server);

	debugPrint(debug, "accept(%i, NULL, NULL) -> ", server);
	if((client = accept(server, NULL, NULL)) < 0)
	{
		close(server);
		return 3;
	}
	debugPrint(debug, "%i\n", client);

	while((r = read(client, payload, 4096)) > 0)
	{
		debugPrint(debug, "Read %"PRIi64" (0x%"PRIx64") bytes to %p \n", (int64_t)r, (uint64_t)r, (void *)payload);
		payload += r;
	}

	close(client);
	close(server);

	debugPrint(debug, "debugClose(%p)\n", (void *)debug);
	debugClose(debug);

	return 4;
}

int getLoaderArguments(int argc, char **argv, int *memoryMode, int *elfInputMode, int *debugMode)
{
	int i;
	int r = 0;

	for(i = 1; i < argc; ++i)
	{
		if(memoryMode != NULL)
		{
			if(strcmp(argv[i], "--memory-plain") == 0)
				*memoryMode = MemoryModePlain;
			if(strcmp(argv[i], "--memory-emulate") == 0)
				*memoryMode = MemoryModeEmulate;
		}

		if(elfInputMode != NULL)
		{
			if(strcmp(argv[i], "--file") == 0)
				*elfInputMode = ElfInputModeFile;
			if(strcmp(argv[i], "--server") == 0)
				*elfInputMode = ElfInputModeServer;
		}

		if(elfInputMode != NULL)
		{
			if(strcmp(argv[i], "--debug") == 0)
				*debugMode = DebugModeOn;
			if(strcmp(argv[i], "--no-debug") == 0)
				*debugMode = DebugModeOff;
		}

		if(memcmp(argv[i], "--", 2) != 0)
			r = i;
	}

	return r;
}

#ifdef BinaryLoader
int elfLoaderMain(int argc, char **argv)
#else
int main(int argc, char **argv)
#endif
{
	int elfInputMode, memoryMode, debugMode;
	FILE *debug;

	int server, client; // only used in ElfInputModeServer
	int argvFile; // only used in ElfInputModeFile
	Elf *elf;
	uint64_t size, alignment;
	ProtectedMemory *memory;
	Runnable run;
	int64_t ret;

	elfInputMode = ElfInputModeFile;
	memoryMode = MemoryModeEmulate;
	debugMode = DebugModeOff;

	#ifdef ElfLoaderServer
		elfInputMode = ElfInputModeServer;
	#endif

	#ifdef ElfLoaderEmulatePS4Memory
		memoryMode = MemoryModeEmulate;
	#endif

	#ifdef Debug
		debugMode = DebugModeOn;
	#endif

	#ifdef __PS4__
		elfInputMode = ElfInputModeServer;
		memoryMode = MemoryModePlain;
		argvFile = 0; // dummy to supress warning
	#else
		argvFile = getLoaderArguments(argc, argv, &memoryMode, &elfInputMode, &debugMode);
	#endif

	signal(SIGPIPE, SIG_IGN);

	/* Get Elf from somewhere */

	if(elfInputMode == ElfInputModeServer)
	{
		debug = (FILE *)NULL;
		if(debugMode)
		{
			if((debug = utilPrintServer(DebugPort)) == NULL)
				return EXIT_FAILURE;
			debugPrint(debug, "utilPrintServer(%i) -> %p\n", DebugPort, (void *)debug);
		}

		debugPrint(debug, "Mode -> ElfLoader (%i, %i)\n", elfInputMode, memoryMode);

		debugPrint(debug, "utilServerCreate(%i, %i, %i) -> ", ServerPort, ServerRetry, ServerTimeout);
		if((server = utilServerCreate(ServerPort, ServerRetry, ServerTimeout, ServerBacklog)) < 0)
		{
			debugPrint(debug, "Server creation failed %i", server);
			return EXIT_FAILURE;
		}
		debugPrint(debug, "%i\n", server);

		debugPrint(debug, "accept(%i, NULL, NULL) -> ", server);
		if((client = accept(server, NULL, NULL)) < 0)
		{
			close(server);
			debugPrint(debug, "Accept failed %i", client);
			return EXIT_FAILURE;
		}
		debugPrint(debug, "%i\n", client);

		debugPrint(debug, "elfCreateFromSocket(%i) -> ", client);
		elf = elfCreateFromSocket(client);
		debugPrint(debug, "%p\n", (void *)elf);

		close(client);
		close(server);

		if(elf == NULL)
		{
			debugPrint(debug, "File doesn't seem to be an ELF\n");
			debugClose(debug);
			return EXIT_FAILURE;
		}
	}
	else // if(elfInputMode == ElfInputModeFile)
	{
		debug = (FILE *)NULL;
		if(debugMode)
		{
			if((debug = fddupopen(STDERR_FILENO, "wb")) == NULL)
				return EXIT_FAILURE;
			debugPrint(debug, "fddupopen(STDERR_FILENO, \"wb\") -> %p\n", (void *)debug);
		}

		debugPrint(debug, "Mode -> ElfLoader (%i, %i)\n", elfInputMode, memoryMode);

		if(argc < 1)
		{
			debugPrint(debug, "%s <ELF file>\n", argv[0]);
			debugClose(debug);
			return EXIT_FAILURE;
		}

		debugPrint(debug, "elfCreateFromFile(%s) -> ", argv[argvFile]);
		if((elf = elfCreateFromFile(argv[argvFile])) == NULL)
		{
			debugPrint(debug, "File doesn't seem to be an ELF\n");
			debugClose(debug);
			return EXIT_FAILURE;
		}
		debugPrint(debug, "%p\n", (void *)elf);
	}

	/* Got Elf here, Get Memory, Load, Run, ..., Free */

	size = elfMemorySize(elf);
	alignment = elfLargestAlignment(elf);

	debugPrint(debug, "protectedMemoryCreate(%"PRIu64", %"PRIu64") -> ", size, alignment);

	if(memoryMode == MemoryModeEmulate)
		memory = protectedMemoryCreateEmulation(size, alignment);
	else
		memory = protectedMemoryCreate(size, alignment);

	if(memory == NULL)
	{
		debugPrint(debug, "Memory Setup failed\n");
		protectedMemoryDebugPrint(debug, memory);
		debugClose(debug);
		return EXIT_FAILURE;
	}
	debugPrint(debug, "%p\n", (void *)memory);

	// FIXME: depricate for 3 method calls
	debugPrint(debug, "elfLoaderLoad(%p, %p, %p) -> ", (void *)elf, protectedMemoryGetWritable(memory), protectedMemoryGetExecutable(memory));
	if((ret = elfLoaderLoad(elf, protectedMemoryGetWritable(memory), protectedMemoryGetExecutable(memory))) < 0)
	{
		debugPrint(debug, "Elf could not be loaded (%"PRIx64")\n", ret);
		protectedMemoryDebugPrint(debug, memory);
		debugClose(debug);
		return EXIT_FAILURE;
	}
	debugPrint(debug, "%"PRId64"\n", ret);

	protectedMemoryDebugPrint(debug, memory);

	run = (Runnable)((uint8_t *)protectedMemoryGetExecutable(memory) + elfEntry(elf));
	debugPrint(debug, "run() [%p + 0x%"PRIx64" = %p] -> ", protectedMemoryGetExecutable(memory), elfEntry(elf), (void *)run);
	ret = run(0, NULL); //1, (char **)&(char *){ "elf" }
	debugPrint(debug, "%"PRId64" = %"PRIu64" = 0x%"PRIx64"\n", ret,  ret, ret);

	debugPrint(debug, "protectedMemoryDestroy(%p) -> ", (void *)memory);
	if(memoryMode == MemoryModeEmulate)
		ret = protectedMemoryDestroyEmulated(memory);
	else
		ret = protectedMemoryDestroy(memory);

	if(ret < 0)
	{
		protectedMemoryDebugPrint(debug, memory);
		debugPrint(debug, "Memory could not be completely freed (%"PRIx64")\n", ret);
	}
	debugPrint(debug, "%"PRId64"\n", ret);

	debugPrint(debug, "elfDestroyAndFree(%p)\n", (void *)elf);
	elfDestroyAndFree(elf);

	debugPrint(debug, "debugClose(%p)\n", (void *)debug);
	debugClose(debug);

	return EXIT_SUCCESS;
}
