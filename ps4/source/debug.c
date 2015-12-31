#include "common.h"
#include "util.h"

#include "debug.h"

int debugPrint(FILE *file, const char *format, ...)
{
	va_list args;
	int r;

	if(file == NULL)
		return -1;

    va_start(args, format);
    r = vfprintf(file, format, args);
	fflush(file); // force flush
    va_end(args);

	return r;
}

int debugClose(FILE *file)
{
	if(file == NULL)
		return -1;
	return fclose(file);
}
