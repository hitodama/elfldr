int baz = 42;
static int beng = 128;

typedef int (*pfffft)(double n);

int bar(double n)
{
	static double moo;
	moo = baz;
	moo += n + 3.14159;
	return (int)moo;
}

char foo(pfffft bar)
{
	static int goo = 55;
	baz /= 2;
	goo += 1;
	if(goo % 2)
		beng = baz;
	return (char)bar((float)(goo == baz) + (double)baz);
}

#ifdef __PS4__
int _main(void)
#else
//int main(int argc, char **argv)
int _main(void)
#endif
{
	static char goo;
	int i = foo(bar);
	while(i > 0)
	{
		baz++;
		--i;
		goo = baz + i;
	}
	return goo;
}

//gcc ../example/stress.c -o e -I include/ -pie -O3 -Wall -pedantic -std=c11 -Wl,--build-id=none -m64 -mcmodel=large -ffreestanding -nostdlib -nostdinc -fno-builtin
