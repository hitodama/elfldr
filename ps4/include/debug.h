#ifndef Debug_H
#define Debug_H

#include <stdarg.h>

void debugEnable();
void debugDisable();
int debugPrint_(const char *format, ...);

#define debugPrint(...) \
	do \
	{ \
		debugPrint_("[%s|%d]: ", __func__, __LINE__); \
		debugPrint_(__VA_ARGS__); \
	} \
	while(0)

#endif
