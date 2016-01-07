#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "elfloader.h"
#include "protmem.h"
#include "util.h"
#include "debug.h"
#include "config.h"

#ifdef __PS4__
#include <kernel.h>
#endif

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

static ElfLoaderConfig *config;

/* Constants */

enum{ StandardIOPort = 5052 };
enum{ ServerPort = 5053 }; //hex(P) + hex(S)
enum{ ServerRetry = 20 };
enum{ ServerTimeout = 1 };
enum{ ServerBacklog = 10 };

/* Binary loader (backwards compatibility) */

#ifdef __PS4__
#include <kernel.h>

FILE *__stdinp;
FILE **__stdinp_addr;
FILE *__stdoutp;
FILE **__stdoutp_addr;
FILE *__stderrp;
FILE **__stderrp_addr;
int __isthreaded;
int *__isthreaded_addr;
#endif

#ifdef BinaryLoader
int main(int argc, char **argv)
#else
int binaryLoaderMain(int argc, char **argv)
#endif
{
	int server, client;
	uint8_t *payload = (uint8_t *)Payload;
	ssize_t r;
	int ret;

	int stdfd[3];
	fpos_t stdpos[3];

	signal(SIGPIPE, SIG_IGN);

	#ifdef __PS4__
	int libc = sceKernelLoadStartModule("libSceLibcInternal.sprx", 0, NULL, 0, 0, 0);
	sceKernelDlsym(libc, "__stdinp", (void **)&__stdinp_addr);
	sceKernelDlsym(libc, "__stdoutp", (void **)&__stdoutp_addr);
	sceKernelDlsym(libc, "__stderrp", (void **)&__stderrp_addr);
	sceKernelDlsym(libc, "__isthreaded", (void **)&__isthreaded_addr);
	__stdinp = *__stdinp_addr;
	__stdoutp = *__stdoutp_addr;
	__stderrp = *__stderrp_addr;
	__isthreaded = *__isthreaded_addr;
	#endif

	config = malloc(sizeof(ElfLoaderConfig));
	configFromDefines(config);

	if(config->debugMode == DebugOn)
	{
		int debug = utilSingleAcceptServer(StandardIOPort);
		utilStandardIORedirect(debug, stdfd, stdpos);
		close(debug);
		debugEnable();
	}

	debugPrint("debugOpen(%i)\n", StandardIOPort);

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
	ret = close(client);
	debugPrint("%i\n", ret);
	debugPrint("close(%i) -> ", server);
	ret = close(client);
	debugPrint("%i\n", ret);

	debugPrint("Executing binary at %p", (void *)payload);

	debugPrint("debugClose()\n");

	utilStandardIOReset(stdfd, stdpos);

	free(config);

	return EXIT_SUCCESS;
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

	debugPrint("utilServerCreate(%i, %i, %i) -> ", ServerPort, ServerRetry, ServerTimeout);
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
	int ret;

	if(server < 0)
	{
		debugPrint("Server is not a file descriptor");
		return NULL;
	}

	debugPrint("accept(%i, NULL, NULL) -> ", server);
	if((client = accept(server, NULL, NULL)) < 0)
	{
		close(server);
		debugPrint("Accept failed %i", client);
		return NULL;
	}
	debugPrint("%i\n", client);

	debugPrint("elfCreateFromSocket(%i) -> ", client);
	elf = elfCreateFromSocket(client);

	if(elf == NULL)
		debugPrint("File could not be read or doesn't seem to be an ELF\n");
	else
		debugPrint("%p\n", (void *)elf);

	debugPrint("close(%i) -> ", client);
	ret = close(client);
	debugPrint("%i\n", ret);

	return elf;
}

void elfLoaderServerDestroy(int server)
{
	if(server < 0)
	{
		debugPrint("Server is not a file descriptor");
		return;
	}

	debugPrint("close(%i) -> ", server);
	debugPrint("%i\n", close(server));
}

Elf *elfLoaderCreateElfFromPath(char *file)
{
	Elf *elf;

	if(file == NULL)
	{
		debugPrint("File path is NULL");
		return NULL;
	}

	debugPrint("elfCreateFromFile(%s) -> ", file);
	if((elf = elfCreateFromFile(file)) == NULL)
		debugPrint("File could not be read or doesn't seem to be an ELF\n");
	else
		debugPrint("%p\n", (void *)elf);

	return elf;
}

ProtectedMemory *elfLoaderMemoryCreate(Elf *elf)
{
	size_t size;
	ProtectedMemory *memory;

	if(elf == NULL)
	{
		debugPrint("Elf is NULL");
		return NULL;
	}

	size = elfMemorySize(elf);

	debugPrint("protectedMemoryCreate(%zu) -> ", size);

	if(config->memoryMode == MemoryEmulate)
		memory = protectedMemoryCreateEmulation(size);
	else
		memory = protectedMemoryCreate(size);

	if(memory == NULL)
		debugPrint("Memory Setup failed\n");
	else
		debugPrint("%p\n", (void *)memory);

	return memory;
}

int elfLoaderMemoryDestroySilent(ProtectedMemory *memory)
{
	int r;

	if(memory == NULL)
		return -1;

	if(config->memoryMode == MemoryEmulate)
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
		debugPrint("Memory is NULL");
		return -1;
	}

	debugPrint("protectedMemoryDestroy(%p) -> ", (void *)memory);
	r = elfLoaderMemoryDestroySilent(memory);

	if(r < 0)
		debugPrint("Memory could not be completely freed - ");
	debugPrint("%i\n", r);

	return r;
}

Runnable elfLoaderRunSetup(Elf *elf, ProtectedMemory *memory)
{
	Runnable run;
	int r;

	if(elf == NULL || memory == NULL)
	{
		debugPrint("Elf (%p)  or memory (%p) NULL\n", (void *)elf, (void *)memory);
		return NULL;
	}

	// FIXME: depricate for 3 method calls
	debugPrint("elfLoaderLoad(%p, %p, %p) -> ", (void *)elf, protectedMemoryWritable(memory), protectedMemoryExecutable(memory));
	run = NULL;
	if((r = elfLoaderLoad(elf, protectedMemoryWritable(memory), protectedMemoryExecutable(memory))) < 0)
		debugPrint("Elf could not be loaded - %i\n", r);
	else
	{
		debugPrint("%i\n", r);
		run = (Runnable)((uint8_t *)protectedMemoryExecutable(memory) + elfEntry(elf));
	}

	debugPrint("elfDestroyAndFree(%p)\n", (void *)elf);
	elfDestroyAndFree(elf); // we don't need the "file" anymore

	return run;
}

void elfLoaderRunSync(Elf *elf)
{
	ProtectedMemory *memory;
	Runnable run;
	int r;

	char *elfName = "elf";
	char *elfArgv[2] = { elfName, NULL };
	int elfArgc = sizeof(elfArgv) / sizeof(elfArgv[0]) - 1;

	if(elf == NULL)
	{
		debugPrint("Elf is NULL");
		return;
	}

	memory = elfLoaderMemoryCreate(elf);
	run = elfLoaderRunSetup(elf, memory);

	if(run != NULL)
	{
		debugPrint("run(%i, {\"%s\", NULL}) [%p + elfEntry = %p] -> ", elfArgc, elfArgv[0], protectedMemoryExecutable(memory), (void *)run);
		r = run(elfArgc, elfArgv);
		debugPrint("%i\n", r);
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
	{
		debugPrint("mainAndMemory is NULL");
		return NULL;
	}

	mm = (MainAndMemory *)mainAndMemory;
	r = mm->main(elfArgc, elfArgv);
	debugPrint("Asynchonous Return %i\n", r);
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
		debugPrint("Elf is NULL");
		return;
	}

 	mm = (MainAndMemory *)malloc(sizeof(MainAndMemory));
	if(mm ==  NULL)
	{
		debugPrint("MainAndMemory allocation failed\n");
		debugPrint("elfDestroyAndFree(%p)\n", (void *)elf);
		elfDestroyAndFree(elf);
		return;
	}

	mm->memory = elfLoaderMemoryCreate(elf);
	mm->main = elfLoaderRunSetup(elf, mm->memory);

	if(mm->main != NULL)
	{
		debugPrint("run [%p + elfEntry = %p]\n", protectedMemoryExecutable(mm->memory), (void *)mm->main);
		pthread_create(&thread, NULL, elfLoaderRunAsyncMain, mm);
	}
	else
		free(mm);
}

void *elfLoaderSingleAcceptServerAsync(void *server)
{
	int client;
	int *s = (int *)server;
	int stdfd[3];
	fpos_t stdpos[3];

	if((*s = utilServerCreate(StandardIOPort, 1, 20, 1)) < 0)
		return NULL;
	client = accept(*s, NULL, NULL);
	if(*s >= 0)
		close(*s);
	if(client < 0)
		return NULL;
	utilStandardIORedirect(client, stdfd, stdpos);
	close(client);

	return NULL;
}

#ifdef BinaryLoader
int elfLoaderMain(int argc, char **argv)
#else
int main(int argc, char **argv)
#endif
{
	int server; // only used in ElfInputServer
	Elf *elf;

	int stdfd[3];
	fpos_t stdpos[3];
	int standardIOAsyncSocket;
	pthread_t standardIOAsyncSetup;

	#ifdef __PS4__
	int libc = sceKernelLoadStartModule("libSceLibcInternal.sprx", 0, NULL, 0, 0, 0);
	sceKernelDlsym(libc, "__stdinp", (void **)&__stdinp_addr);
	sceKernelDlsym(libc, "__stdoutp", (void **)&__stdoutp_addr);
	sceKernelDlsym(libc, "__stderrp", (void **)&__stderrp_addr);
	sceKernelDlsym(libc, "__isthreaded", (void **)&__isthreaded_addr);
	__stdinp = *__stdinp_addr;
	__stdoutp = *__stdoutp_addr;
	__stderrp = *__stderrp_addr;
	__isthreaded = *__isthreaded_addr;
	#endif

	signal(SIGPIPE, SIG_IGN);

	config = malloc(sizeof(ElfLoaderConfig));
	configFromDefines(config);

	#ifndef __PS4__
		configFromArguments(config, argc, argv);
 		if(config->elfInputMode == ElfInputFile && config->inputFile == NULL)
		{
			fprintf(stderr, "No file provided as argument in file mode.\n");
			free(config);
			return EXIT_FAILURE;
		}
	#endif

	if(config->debugMode == DebugOn)
		debugEnable();

	if(config->elfInputMode == ElfInputFile)
		debugPrint("debugOpen(STDERR_FILENO)\n");
	else
	{
		if(config->standardIORedirectMode == StandardIORedirectWait)
		{
			int debug = utilSingleAcceptServer(StandardIOPort);
			utilStandardIORedirect(debug, stdfd, stdpos);
			close(debug);
		}
		else if(config->standardIORedirectMode == StandardIORedirectLazy)
			pthread_create(&standardIOAsyncSetup, NULL, elfLoaderSingleAcceptServerAsync, &standardIOAsyncSocket);

		debugPrint("debugOpen(%i)\n", StandardIOPort);
	}

	debugPrint("Mode -> ElfLoader [input: %i, memory: %i, thread: %i, debug: %i, stdio: %i]\n",
		config->elfInputMode, config->memoryMode, config->threadingMode, config->debugMode, config->standardIORedirectMode);

	if(config->elfInputMode == ElfInputServer)
	{
		server = elfLoaderServerCreate();
		if(config->threadingMode == Threading)
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
			if(elf != NULL)
				elfLoaderRunSync(elf);
		}
		elfLoaderServerDestroy(server);
	}
	else // if(elfInputMode == ElfInputFile)
	{
		elf = elfLoaderCreateElfFromPath(config->inputFile);
		if(elf != NULL)
			elfLoaderRunSync(elf);
	}

	debugPrint("debugClose()\n");

	if(config->standardIORedirectMode == StandardIORedirectLazy)
	{
		int s = standardIOAsyncSocket;
		standardIOAsyncSocket = -1;
		shutdown(s, SHUT_RDWR);
		pthread_join(standardIOAsyncSetup, NULL);
	}

	if(config->standardIORedirectMode != StandardIORedirectNone && config->elfInputMode == ElfInputServer)
		utilStandardIOReset(stdfd, stdpos);

	//if(config->standardIORedirectMode == StandardIORedirectWait && config->elfInputMode == ElfInputServer)
	//	utilStandardIOReset(stdfd, stdpos);

	free(config);

	return EXIT_SUCCESS;
}
