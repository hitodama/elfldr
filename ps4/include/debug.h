#ifndef Debug_H
#define Debug_H

enum{ DebugPort = 5052 };

int debugPrint(FILE *file, const char *format, ...);
int debugClose(FILE *file);

#endif
