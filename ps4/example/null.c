//#include <stdlib.h>

#ifdef __PS4__
int _main(void)
#else
//int main(int argc, char **argv)
int _main(void)
#endif
{
	//return EXIT_SUCCESS;
	return 0x42;
}

//gcc ../example/null.c -o e -I include/ -pie -O3 -Wall -pedantic -std=c11 -Wl,--build-id=none -m64 -mcmodel=large -ffreestanding -nostdlib -nostdinc -fno-builtin
