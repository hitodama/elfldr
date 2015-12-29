#ifndef Debug_H
#define Debug_H

#if defined(DEBUG) || defined(Debug)
	int dfprintf(FILE *stream, const char *format, ...);
	#define debugPrint dfprintf
#else
	#define debugPrint(...)
#endif

#endif
