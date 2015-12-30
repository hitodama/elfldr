#include "common.h"
#include "util.h"

#include "debug.h"

#if defined(DEBUG) || defined(Debug)

static FILE *debugStream;

int debugOpen(int port)
{
	if(port == 0)
	{
		int fd = dup(STDERR_FILENO);
		if(fd < 0)
			return -1;
		debugStream = fdopen(fd, "wb");
	}
	else
		debugStream = utilPrintServer(port);

	if(debugStream == NULL)
		return -2;
	return 1;
}

int debugPrint(const char *format, ...)
{
	va_list args;
	int r;

	if(debugStream == NULL)
		return -1;

    va_start(args, format);
    r = vfprintf(debugStream, format, args);
	fflush(debugStream); // force flush
    va_end(args);

	return r;
}

int debugClose()
{
	if(debugStream == NULL)
		return -1;

	if(fclose(debugStream) != 0)
		return -1;

	return 1;
}

#endif
