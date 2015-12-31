#include "common.h"
#include "util.h"

FILE *fddupopen(int fd, const char *mode)
{
	int t;
	FILE *r = NULL;

	if(mode == NULL)
		return NULL;

	if((t = dup(fd)) < 0)
		return NULL;

	if((r = fdopen(t, mode)) == NULL)
		close(t);

	return r;
}

int utilServerCreate(uint16_t port, int backlog, int try, unsigned int sec)
{
	int server;
	struct sockaddr_in serverAddress;
	int r;

	memset(&serverAddress, 0, sizeof(serverAddress));
	#ifdef __FreeBSD__ //parent of our __PS4__
	serverAddress.sin_len = sizeof(serverAddress);
	#endif
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddress.sin_port = htons(port);

	for(; try > 0; --try)
	{
		server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if(server < 0)
			sleep(sec);
	}

	if(server < 0)
		return -1;

	setsockopt(server, SOL_SOCKET, SO_REUSEADDR, (char *)&(int){ 1 }, sizeof(int));
	setsockopt(server, SOL_SOCKET, SO_REUSEPORT, (char *)&(int){ 1 }, sizeof(int));

	if((r = bind(server, (struct sockaddr *)&serverAddress, sizeof(serverAddress))) < 0)
	{
		close(server);
		return -2;
	}

	if((r = listen(server, backlog)) < 0)
	{
		close(server);
		return -3;
	}

	return server;
}

FILE *utilPrintServer(int port)
{
	int server, client;
	FILE *out;

	if((server = utilServerCreate(port, 1, 20, 1)) < 0)
		return NULL;

	if((client = accept(server, NULL, NULL)) < 0)
	{
		close(server);
		return NULL;
	}

	close(server);
	out = fddupopen(client, "wb");
	close(client);
	//setvbuf(in, NULL, _IOLBF, 0);
	setvbuf(out, NULL, _IONBF, 0);

	return out;
}

void *utilAllocUnsizeableFileFromDescriptor(int fd, uint64_t *size)
{
	int length = 0;
	int full = 4096;
	uint8_t *data = (void *)malloc(full);
	uint64_t s = 0;

	if(size != NULL)
		*size = 0;

	while((length = read(fd, data + s, full - s)) > 0)
	{
		s += length;
		if(s == full)
		{
			void *t;
			full *= 2;
			t = malloc(full);
			memcpy(t, data, s);
			free(data);
			data = t;
		}
	}

	if(size != NULL)
		*size = s;

	return data;
}

void *utilAllocFileAligned(char *file, uint64_t *size, uint64_t alignment)
{
	struct stat s;
	FILE *f;
	uint32_t *b;
	uint64_t sz;
	uint64_t i;

	if(size != NULL)
		*size = 0;

	if(stat(file, &s) < 0)
		return NULL;

 	sz = ((uint64_t)s.st_size * alignment) / alignment;
	b = (uint32_t *)malloc(sz * sizeof(uint8_t));

	if(b == NULL)
		return NULL;

	f = fopen(file, "rb");
	if(f == NULL)
	{
		free(b);
		return NULL;
	}
	fread(b, s.st_size, 1, f);
	fclose(f);

	if(size != NULL)
		*size = sz;

	for(i = s.st_size; i < sz; ++i)
		((uint32_t *)b)[i] = 0;

	return b;
}
