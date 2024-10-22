// gcc -o echo_input echo_input.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char **argv)
{
	printf("echo_input test line\n");
	fflush(stdout);
	
	//printf("checks=fail\n");
	printf("checks=pass\n");
	fflush(stdout);
	
	int counter = 1;
	
	// keep program running and output argument every 1 second
	while (1) {
		usleep(200000); // 0.2 seconds
		printf("instance = %d, counter = %d, argument = %s\n", atoi(argv[1]), counter++, argv[2]);
		fflush(stdout);
	}
	
	return 1;
}
