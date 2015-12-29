#include "common.h"
#include "elfldr.h"
#include "protmem.h"
#include "util.h"
#include "debug.h"

#if defined(__PS4__) && !defined(ElfLdrServer)
	#define ElfLdrServer 1
#endif

/*
#if defined(BinLdr) && !defined(__PS4__)
	#error BinLdr can't be defined without __PS4__
#endif
*/

#if defined(DEBUG) || defined(Debug)
	#ifdef ElfLdrServer
		#define debugOpen utilPrintServer
		#define debugClose fclose
	#else
		#define debugOpen(...) stderr
		#define debugClose(...)
	#endif
#else
	#define debugOpen(...) (FILE *)1
	#define debugClose(...)
#endif

enum{ DebugPort = 5052 };
//hex(P) + hex(S) + 4
enum{ ServerPort = 5053 };
enum{ ServerRetry = 20 };
enum{ ServerTimeout = 1 };
enum{ ServerBacklog = 10 };
//enum{ PageSize = 16 * 1024 };

#ifdef BinLdr

//#ifndef Payload
//	#define Payload 0x926400000 // You should define this in the Makefile
//#endif

int main(int argc, char **argv)
{
	int server, client;
	uint8_t *payload = (uint8_t *)Payload;
	ssize_t r;
	FILE *debug;

	signal(SIGPIPE, SIG_IGN);

	if((debug = debugOpen(DebugPort)) == NULL)
		return 1;
	debugPrint(debug, "debugOpen() -> %p\n", debug);

	debugPrint(debug, "Mode -> BinLdr\n");

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
		debugPrint(debug, "Read %i (0x%x) bytes to %p \n", read, (uint64_t)r, (void *)payload);
		payload += r;
	}

	close(client);
	close(server);

	debugPrint(debug, "debugClose(%p)\n", debug);
	debugClose(debug);

	return 4;
}

//#endif
#else
//#ifdef ElfLdr

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

int main(int argc, char **argv)
{
	#ifdef ElfLdrServer
		int server, client;
	#endif
	Elf *elf;
	ProtectedMemory *memory;
	int (*run)(void);
	int64_t ret;
	FILE *debug;

	signal(SIGPIPE, SIG_IGN);

	/* Get Elf from somewhere */

	if((debug = debugOpen(DebugPort)) == NULL)
		return EXIT_FAILURE;

	debugPrint(debug, "debugOpen(%i) -> %p\n", DebugPort, (void *)debug);

	debugPrint(debug, "Mode -> ElfLdr\n");

	#ifdef ElfLdrServer
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
	#else
		if(argc < 1)
		{
			debugPrint(debug, "%s <ELF file>\n", argv[0]);
			debugClose(debug);
			return EXIT_FAILURE;
		}

		debugPrint(debug, "elfCreateFromFile(%s) -> ", argv[1]);
		if((elf = elfCreateFromFile(argv[1])) == NULL)
		{
			debugPrint(debug, "File doesn't seem to be an ELF\n");
			debugClose(debug);
			return EXIT_FAILURE;
		}
		debugPrint(debug, "%p\n", (void *)elf);
	#endif

	/* Got Elf here, Get Memory, Load, Run, ..., Free */

	debugPrint(debug, "protectedMemoryCreate(%p) -> ", (void *)elf);
	if((memory = protectedMemoryCreate(elf)) == NULL)
	{
		debugPrint(debug, "Memory Setup failed\n");
		protectedMemoryDebugPrint(debug, memory);
		debugClose(debug);
		return EXIT_FAILURE;
	}
	debugPrint(debug, "%p\n", (void *)memory);

	debugPrint(debug, "elfLdrLoad(%p, %p, %p) -> ", (void *)elf, memory->writableAligned, memory->executableAligned);
	if((ret = elfLdrLoad(elf, memory->writableAligned, memory->executableAligned)) < 0)
	{
		debugPrint(debug, "Elf could not be loaded (%"PRIx64")\n", ret);
		protectedMemoryDebugPrint(debug, memory);
		debugClose(debug);
		return EXIT_FAILURE;
	}
	debugPrint(debug, "%"PRId64"\n", ret);

	protectedMemoryDebugPrint(debug, memory);

	run = (int (*)(void))((uint8_t *)memory->executableAligned + elfEntry(elf));
	debugPrint(debug, "run() [%p + 0x%"PRIx64" = %p] -> ", memory->executableAligned, elfEntry(elf), (void *)run);
	ret = run();
	debugPrint(debug, "%"PRId64" = %"PRIu64" = 0x%"PRIx64"\n", ret,  ret, ret);

	debugPrint(debug, "protectedMemoryDestroy(%p) -> ", (void *)memory);
	if((ret = protectedMemoryDestroy(memory)) < 0)
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

#endif
