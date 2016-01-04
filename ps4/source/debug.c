#include "common.h"
#include "util.h"

#include "debug.h"

#if defined(Debug) || defined(DEBUG)

static FILE *debug;

int debugOpen(FILE *file)
{
	debug = file;
	if(debug == NULL)
		return 0;
	return 1;
}

int debugPrint(const char *format, ...)
{
	va_list args;
	int r;

	if(debug == NULL)
		return 0;

    va_start(args, format);
    r = vfprintf(debug, format, args);
	fflush(debug); // force flush
    va_end(args);

	return r;
}

int debugClose()
{
	if(debug == NULL)
		return 0;
	return fclose(debug);
	debug = NULL;
}

#endif
