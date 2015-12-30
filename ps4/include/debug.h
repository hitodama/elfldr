#ifndef Debug_H
#define Debug_H

enum{ DebugPort = 5052 };

#if defined(DEBUG) || defined(Debug)
	#include <stdio.h>

	int debugOpen(int port);
	int debugPrint(const char *format, ...);
	int debugClose();
#else
	#define debugPrint(...) 1
	#define debugOpen(...) 1
	#define debugClose(...) 1
#endif

#endif
