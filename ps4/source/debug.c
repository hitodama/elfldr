#include "common.h"
#include "util.h"

#include "debug.h"

static int debugIsEnabled;

void debugEnable()
{
	debugIsEnabled = 1;
}

void debugDisable()
{
	debugIsEnabled = 0;
}

int debugPrint_(const char *format, ...)
{
	va_list args;
	int r;

	if(debugIsEnabled == 0)
		return 0;

    va_start(args, format);
    r = vfprintf(stderr, format, args);
	//fflush(stderr); // force flush
    va_end(args);

	return r;
}
