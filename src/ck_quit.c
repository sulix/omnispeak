
#include <stdio.h>
#include <stdlib.h>

void Quit(const char *msg) {
	if (!msg)
	{
		printf("Thanks for playing Commander Keen!\n");
		exit(0);
	}
	else
	{
		//__asm__("int $3");
		fprintf(stderr, "%s\n", msg);
		exit(-1);
	}
}
