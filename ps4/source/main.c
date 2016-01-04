#include "common.h"
#include "elfloader.h"
#include "protmem.h"
#include "util.h"
#include "debug.h"

#if defined(BinaryLoader) && !defined(__PS4__)
	#error BinaryLoader can not be build on x86-64
#endif

/* Types */

typedef int (*Runnable)(int, char **);

typedef struct MainAndMemory
{
	Runnable main;
	ProtectedMemory *memory;
}
MainAndMemory;

/* Constants */

enum{ ServerPort = 5053 }; //hex(P) + hex(S)
enum{ ServerRetry = 20 };
enum{ ServerTimeout = 1 };
enum{ ServerBacklog = 10 };
//enum{ PageSize = 16 * 1024 };

enum{ MemoryPlain = 0, MemoryEmulate = 1 };
enum{ ElfInputFile = 0, ElfInputServer = 1 };
enum{ ThreadingNone = 0, Threading = 1 };
enum{ DebugOff = 0, DebugOn = 1 };

/* Globals */

static int elfInputMode, memoryMode, debugMode, threadingMode;

/* Binary loader (backwards compatibility) */

#ifdef BinaryLoader
int main(int argc, char **argv)
#else
int binaryLoaderMain(int argc, char **argv)
#endif
{
	int server, client;
	uint8_t *payload = (uint8_t *)Payload;
	ssize_t r;

	signal(SIGPIPE, SIG_IGN);

	if(!debugOpen(utilPrintServer(DebugPort)))
		return EXIT_FAILURE;

	debugPrint("debugOpen(utilPrintServer(%i))", DebugPort);

	debugPrint("Mode -> BinaryLoader\n");

	debugPrint("utilServerCreate(%i, %i, %i) -> ", ServerPort, ServerRetry, ServerTimeout);
	if((server = utilServerCreate(ServerPort, ServerBacklog, ServerRetry, ServerTimeout)) < 0)
	{
		debugPrint("Could not create server (return: %i)", server);
		return EXIT_FAILURE;
	}
	debugPrint("%i\n", server);

	debugPrint("accept(%i, NULL, NULL) -> ", server);
	if((client = accept(server, NULL, NULL)) < 0)
	{
		debugPrint("Could not accept client (return: %i)", client);
		close(server);
		return EXIT_FAILURE;
	}
	debugPrint("%i\n", client);

	while((r = read(client, payload, 4096)) > 0)
	{
		debugPrint("Read %"PRIi64" (0x%"PRIx64") bytes to %p \n", (int64_t)r, (uint64_t)r, (void *)payload);
		payload += r;
	}

	debugPrint("close(%i) -> ", client);
	debugPrint("%i\n", close(client));
	debugPrint("close(%i) -> ", server);
	debugPrint("%i\n", close(server));

	debugPrint("Executing binary at %p", payload);

	debugPrint("debugClose()\n");
	debugClose();

	return EXIT_SUCCESS;
}

// pragmantic impl.
int getLoaderArguments(int argc, char **argv, int *memoryMode, int *elfInputMode, int *threadingMode, int *debugMode)
{
	int i;
	int r = 0;

	for(i = 1; i < argc; ++i)
	{
		if(memoryMode != NULL)
		{
			if(strcmp(argv[i], "--memory-plain") == 0)
				*memoryMode = MemoryPlain;
			if(strcmp(argv[i], "--memory-emulate") == 0)
				*memoryMode = MemoryEmulate;
		}

		if(elfInputMode != NULL)
		{
			if(strcmp(argv[i], "--file") == 0)
				*elfInputMode = ElfInputFile;
			if(strcmp(argv[i], "--server") == 0)
				*elfInputMode = ElfInputServer;
		}

		if(threadingMode != NULL)
		{
			if(strcmp(argv[i], "--threading") == 0)
				*threadingMode = Threading;
			if(strcmp(argv[i], "--no-threading") == 0)
				*threadingMode = ThreadingNone;
		}

		if(debugMode != NULL)
		{
			if(strcmp(argv[i], "--debug") == 0)
				*debugMode = DebugOn;
			if(strcmp(argv[i], "--no-debug") == 0)
				*debugMode = DebugOff;
		}

		if(memcmp(argv[i], "--", 2) != 0)
			r = i;
	}

	return r;
}

/* elf utils */

Elf *elfCreateFromSocket(int client)
{
	Elf *elf;
	uint64_t s;

	void * m = utilAllocUnsizeableFileFromDescriptor(client, &s);
	if(m == NULL)
		return NULL;
	elf = elfCreate(m, s);
	if(!elfLoaderIsLoadable(elf))
	{
		free(m);
		elf = NULL;
	}

	return elf;
}

Elf *elfCreateFromFile(char *path)
{
	Elf *elf;
	uint64_t s;

	void * m = utilAllocFile(path, &s);
	if(m == NULL)
		return NULL;
	elf = elfCreate(m, s);
	if(!elfLoaderIsLoadable(elf))
	{
		free(m);
		elf = NULL;
	}

	return elf;
}

/* elf loader surface (view) wrappers*/

int elfLoaderServerCreate()
{
	int server;

	debugPrint("%s: utilServerCreate(%i, %i, %i) -> ", __func__, ServerPort, ServerRetry, ServerTimeout);
	if((server = utilServerCreate(ServerPort, ServerRetry, ServerTimeout, ServerBacklog)) < 0)
		debugPrint("Server creation failed %i", server);
	else
		debugPrint("%i\n", server);

	return server;
}

Elf *elfLoaderServerAcceptElf(int server)
{
	int client;
	Elf *elf;

	if(server < 0)
	{
		debugPrint("%s: Server is not a file descriptor", __func__);
		return NULL;
	}

	debugPrint("%s: accept(%i, NULL, NULL) -> ", __func__, server);
	if((client = accept(server, NULL, NULL)) < 0)
	{
		close(server);
		debugPrint("Accept failed %i", client);
		return NULL;
	}
	debugPrint("%i\n", client);

	debugPrint("%s: elfCreateFromSocket(%i) -> ", __func__, client);
	elf = elfCreateFromSocket(client);

	if(elf == NULL)
		debugPrint("File could not be read or doesn't seem to be an ELF\n");
	else
		debugPrint("%p\n", (void *)elf);

	debugPrint("%s: close(%i) -> ", __func__, client);
	debugPrint("%i\n", close(client));

	return elf;
}

void elfLoaderServerDestroy(int server)
{
	if(server < 0)
	{
		debugPrint("%s: Server is not a file descriptor", __func__);
		return;
	}

	debugPrint("%s: close(%i) -> ", __func__, server);
	debugPrint("%i\n", close(server));
}

Elf *elfLoaderCreateElfFromPath(char *file)
{
	Elf *elf;

	if(file == NULL)
	{
		debugPrint("%s: File path is NULL", __func__);
		return NULL;
	}

	debugPrint("%s: elfCreateFromFile(%s) -> ", __func__, file);
	if((elf = elfCreateFromFile(file)) == NULL)
		debugPrint("File doesn't seem to be an ELF\n");
	else
		debugPrint("%p\n", (void *)elf);

	return elf;
}

ProtectedMemory *elfLoaderMemoryCreate(Elf *elf)
{
	uint64_t size, alignment;
	ProtectedMemory *memory;

	if(elf == NULL)
	{
		debugPrint("%s: Elf is NULL", __func__);
		return NULL;
	}

	size = elfMemorySize(elf);
	alignment = elfLargestAlignment(elf);

	debugPrint("%s: protectedMemoryCreate(%"PRIu64", %"PRIu64") -> ", __func__, size, alignment);

	if(memoryMode == MemoryEmulate)
		memory = protectedMemoryCreateEmulation(size, alignment);
	else
		memory = protectedMemoryCreate(size, alignment);

	if(memory == NULL)
		debugPrint("Memory Setup failed\n", __func__);
	else
		debugPrint("%p\n", (void *)memory);

	return memory;
}

int elfLoaderMemoryDestroySilent(ProtectedMemory *memory)
{
	int r;

	if(memory == NULL)
		return -1;

	if(memoryMode == MemoryEmulate)
		r = protectedMemoryDestroyEmulation(memory);
	else
		r = protectedMemoryDestroy(memory);

	return r;
}

int elfLoaderMemoryDestroy(ProtectedMemory *memory)
{
	int r;

	if(memory == NULL)
	{
		debugPrint("%s: Memory is NULL", __func__);
		return -1;
	}

	debugPrint("%s: protectedMemoryDestroy(%p) -> ", __func__, (void *)memory);
	r = elfLoaderMemoryDestroySilent(memory);

	if(r < 0)
	{
		protectedMemoryDebugPrint(memory);
		debugPrint("Memory could not be completely freed - ");
	}
	debugPrint("%i\n", r);

	return r;
}

Runnable elfLoaderRunSetup(Elf *elf, ProtectedMemory *memory)
{
	Runnable run;
	int64_t r;

	if(elf == NULL || memory == NULL)
	{
		debugPrint("%s: Elf (%p)  or memory (%p) NULL\n", __func__, elf, memory);
		return NULL;
	}

	// FIXME: depricate for 3 method calls
	debugPrint("%s: elfLoaderLoad(%p, %p, %p) -> ", __func__, (void *)elf, protectedMemoryGetWritable(memory), protectedMemoryGetExecutable(memory));
	if((r = elfLoaderLoad(elf, protectedMemoryGetWritable(memory), protectedMemoryGetExecutable(memory))) < 0)
	{
		debugPrint("Elf could not be loaded - %"PRIx64"\n", r);
		run = NULL;
	}
	else
	{
		debugPrint("%"PRId64"\n", r);
		run = (Runnable)((uint8_t *)protectedMemoryGetExecutable(memory) + elfEntry(elf));
	}

	debugPrint("%s: elfDestroyAndFree(%p)\n", __func__, (void *)elf);
	elfDestroyAndFree(elf); // we don't need the "file" anymore

	return run;
}

void elfLoaderRunSync(Elf *elf)
{
	ProtectedMemory *memory;
	Runnable run;
	int64_t r;

	char *elfName = "elf";
	char *elfArgv[2] = { elfName, NULL };
	int elfArgc = sizeof(elfArgv) / sizeof(elfArgv[0]) - 1;

	if(elf == NULL)
	{
		debugPrint("%s: Elf is NULL", __func__);
		return;
	}

	memory = elfLoaderMemoryCreate(elf);
	protectedMemoryDebugPrint(memory);

	run = elfLoaderRunSetup(elf, memory);
	if(run != NULL)
	{
		debugPrint("%s: run(%i, {\"%s\", NULL}) [%p + elfEntry = %p] -> ", __func__, elfArgc, elfArgv[0], protectedMemoryGetExecutable(memory), (void *)run);
		r = run(elfArgc, elfArgv);
		debugPrint("%"PRId64" = %"PRIu64" = 0x%"PRIx64"\n", r,  r, r);
	}

	elfLoaderMemoryDestroy(memory);
}

void *elfLoaderRunAsyncMain(void *mainAndMemory)
{
	MainAndMemory *mm;
	int r;

	char *elfName = "elf";
	char *elfArgv[2] = { elfName, NULL };
	int elfArgc = sizeof(elfArgv) / sizeof(elfArgv[0]) - 1;

	if(mainAndMemory == NULL)
		return NULL;

	mm = (MainAndMemory *)mainAndMemory;
	r = mm->main(elfArgc, elfArgv);
	//debugPrint("%"PRId64" = %"PRIu64" = 0x%"PRIx64"\n", r,  r, r);
	elfLoaderMemoryDestroySilent(mm->memory);
	free(mm);

	return NULL;
}

void elfLoaderRunAsync(Elf *elf)
{
	pthread_t thread;
	MainAndMemory *mm;

	if(elf == NULL)
	{
		debugPrint("%s: Elf is NULL", __func__);
		return;
	}

 	mm = (MainAndMemory *)malloc(sizeof(MainAndMemory));
	mm->memory = elfLoaderMemoryCreate(elf);
	protectedMemoryDebugPrint(mm->memory);
	mm->main = elfLoaderRunSetup(elf, mm->memory);

	if(mm->main != NULL)
	{
		debugPrint("%s: run [%p + elfEntry = %p]\n", __func__, protectedMemoryGetExecutable(mm->memory), (void *)mm->main);
		pthread_create(&thread, NULL, elfLoaderRunAsyncMain, mm);
	}
	else
	{
		elfLoaderMemoryDestroy(mm->memory);
		free(mm);
	}
}

#ifdef BinaryLoader
int elfLoaderMain(int argc, char **argv)
#else
int main(int argc, char **argv)
#endif
{
	int server; // only used in ElfInputServer
	int argvFile; // only used in ElfInputFile
	Elf *elf;

	elfInputMode = ElfInputFile;
	memoryMode = MemoryEmulate;
	debugMode = DebugOff;
	threadingMode = ThreadingNone;

	#ifdef ElfLoaderServer
		elfInputMode = ElfInputServer;
	#endif

	#ifdef ElfLoaderEmulatePS4Memory
		memoryMode = MemoryEmulate;
	#endif

	#ifdef Debug
		debugMode = DebugOn;
	#endif

	#ifdef ElfLoaderThreading
		threadingMode = Threading;
	#endif

	#ifdef __PS4__
		elfInputMode = ElfInputServer;
		memoryMode = MemoryPlain;
		argvFile = 0; // dummy to supress warning
	#else
		argvFile = getLoaderArguments(argc, argv, &memoryMode, &elfInputMode, &threadingMode, &debugMode);
		if(elfInputMode == ElfInputFile)
		{
	 		if(argvFile == 0)
			{
				fprintf(stderr, "No file provided as argument in file mode.\n");
				return EXIT_FAILURE;
			}
			threadingMode = ThreadingNone;
		}
	#endif

	signal(SIGPIPE, SIG_IGN);

	if(debugMode == DebugOn)
	{
		if(elfInputMode == ElfInputServer)
		{
			if(!debugOpen(utilPrintServer(DebugPort)))
				return EXIT_FAILURE;
			debugPrint("%s: debugOpen(utilPrintServer(%i))\n", __func__, DebugPort);
		}
		else
		{
			if(!debugOpen(fddupopen(STDERR_FILENO, "wb")))
				return EXIT_FAILURE;
			debugPrint("%s: debugOpen(fddupopen(STDERR_FILENO, \"wb\"))\n", __func__);
		}

		debugPrint("%s: Mode -> ElfLoader [input: %i, memory: %i, thread: %i, debug: %i]\n",
			__func__, elfInputMode, memoryMode, threadingMode, debugMode);
	}

	if(elfInputMode == ElfInputServer)
	{
		server = elfLoaderServerCreate();
		if(threadingMode == Threading)
		{
			for(;;)
			{
				elf = elfLoaderServerAcceptElf(server);
 				if(elf == NULL) // to stop, send a non-elf file - cheesy I know
					break;
				elfLoaderRunAsync(elf);
			}
		}
		else
		{
			elf = elfLoaderServerAcceptElf(server);
			elfLoaderRunSync(elf);
		}
		elfLoaderServerDestroy(server);
	}
	else // if(elfInputMode == ElfInputFile)
	{
		elf = elfLoaderCreateElfFromPath(argv[argvFile]);
		elfLoaderRunSync(elf);
	}

	debugPrint("%s: debugClose()\n", __func__);
	debugClose();

	return EXIT_SUCCESS;
}
