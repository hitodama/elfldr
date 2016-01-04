#ifndef Debug_H
#define Debug_H

enum{ DebugPort = 5052 };

#if defined(Debug) || defined(DEBUG)

int debugOpen(FILE *file);
int debugPrint(const char *format, ...);
int debugClose();

#else

#define debugOpen(...) 1
#define debugPrint(...) 1
#define debugClose(...) 0

#endif

#endif
