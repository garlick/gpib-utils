/* 0=success, 1=failure, 2=notrun */
#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include <unistd.h>

#include "vxi11.h"
#include "test.h"

#define TRIES 100

int
main(int argc, char *argv[])
{
	char *prog = basename(argv[0]);
	vxi11_handle_t vp;
	long err;
	int i;

	if (argc == 2) {
		printf("%s: open and close device %d times\n", prog, TRIES);
		exit(0);
	}

	for (i = 0; i < TRIES; i++) {
		vp = vxi11_open(TEST_GWADDR, TEST_DEVNAME, 0, 0, 0, 0, &err);
		if (!vp) {
			fprintf(stderr, "%s: open(%d): %s\n", prog, i,
							 vxi11_strerror(err));
			exit(1);
		}
		vxi11_close(vp);
	}

	exit(0);
}
