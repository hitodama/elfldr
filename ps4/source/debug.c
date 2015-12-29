#include <stdarg.h>
#include <stdio.h>

#include "debug.h"

int dfprintf(FILE *stream, const char *format, ...)
{
	int r;
    va_list args;
    va_start(args, format);
    r = vfprintf(stream, format, args);
	fflush(stream);
    va_end(args);
	return r;
}
