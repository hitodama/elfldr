#include <string.h>

#include "config.h"

void configFromDefines(ElfLoaderConfig *config)
{
	/* set defaults at build time */

	#ifdef ElfLoaderServer
		config->elfInputMode = ElfInputServer;
	#else
		config->elfInputMode = ElfInputFile;
	#endif

	#ifdef ElfLoaderEmulatePS4Memory
		config->memoryMode = MemoryEmulate;
	#else
		config->memoryMode = MemoryPlain;
	#endif

	#if defined(ElfLoaderStandardIORedirectWait)
		config->standardIORedirectMode = StandardIORedirectWait;
	#elif defined(ElfLoaderStandardIORedirectLazy)
		config->standardIORedirectMode = StandardIORedirectLazy;
	#else
		config->standardIORedirectMode = StandardIORedirectNone;
	#endif

	#ifdef Debug
		config->debugMode = DebugOn;
		if(config->standardIORedirectMode == StandardIORedirectNone)
			config->standardIORedirectMode = StandardIORedirectWait;
	#else
		config->debugMode = DebugOff;
	#endif

	#ifdef ElfLoaderThreading
		config->threadingMode = Threading;
	#else
		config->threadingMode = ThreadingNone;
	#endif

	#ifdef __PS4__
		config->elfInputMode = ElfInputServer;
		config->memoryMode = MemoryPlain;
	#endif
}

void configFromArguments(ElfLoaderConfig *config, int argc, char **argv)
{
	int i;

	for(i = 1; i < argc; ++i)
	{
		if(strcmp(argv[i], "--memory-plain") == 0)
			config->memoryMode = MemoryPlain;
		if(strcmp(argv[i], "--memory-emulate") == 0)
			config->memoryMode = MemoryEmulate;

		if(strcmp(argv[i], "--file") == 0)
			config->elfInputMode = ElfInputFile;
		if(strcmp(argv[i], "--server") == 0)
			config->elfInputMode = ElfInputServer;

		if(strcmp(argv[i], "--threading") == 0)
			config->threadingMode = Threading;
		if(strcmp(argv[i], "--no-threading") == 0)
			config->threadingMode = ThreadingNone;

		if(strcmp(argv[i], "--debug") == 0)
			config->debugMode = DebugOn;
		if(strcmp(argv[i], "--no-debug") == 0)
			config->debugMode = DebugOff;

		if(strcmp(argv[i], "--stdio-none") == 0)
			config->standardIORedirectMode = StandardIORedirectNone;
		if(strcmp(argv[i], "--stdio-wait") == 0)
			config->standardIORedirectMode = StandardIORedirectWait;
		if(strcmp(argv[i], "--stdio-lazy") == 0)
			config->standardIORedirectMode = StandardIORedirectLazy;

		if(memcmp(argv[i], "--", 2) != 0)
			config->inputFile = argv[i];
	}
	if(config->debugMode == DebugOn && config->standardIORedirectMode == StandardIORedirectNone)
		config->standardIORedirectMode = StandardIORedirectWait;

	if(config->elfInputMode == ElfInputFile)
		config->threadingMode = ThreadingNone;
}
