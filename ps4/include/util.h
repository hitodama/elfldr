#ifndef Util_H
#define Util_H

#include <stdint.h>

FILE *fddupopen(int fd, const char *mode);
int utilServerCreate(uint16_t port, int backlog, int try, unsigned int sec);
FILE *utilPrintServer(int port);
void *utilAllocUnsizeableFileFromDescriptor(int fd, uint64_t *size);
void *utilAllocFileAligned(char *file, uint64_t *size, uint64_t alignment);
#define utilAllocFile(file, size) utilAllocFileAligned(file, size, 1)

#endif
