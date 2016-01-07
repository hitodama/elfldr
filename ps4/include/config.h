enum{ MemoryPlain = 0, MemoryEmulate = 1 };
enum{ ElfInputFile = 0, ElfInputServer = 1 };
enum{ ThreadingNone = 0, Threading = 1 };
enum{ DebugOff = 0, DebugOn = 1 };
enum{ StandardIORedirectNone = 0, StandardIORedirectWait = 1, StandardIORedirectLazy = 2 };

typedef struct ElfLoaderConfig
{
	int elfInputMode;
 	int memoryMode;
	int debugMode;
	int threadingMode;
	int standardIORedirectMode;
	char *inputFile;
}
ElfLoaderConfig;

void configFromDefines(ElfLoaderConfig *config);
void configFromArguments(ElfLoaderConfig *config, int argc, char **argv);
