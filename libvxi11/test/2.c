/* 0=success, 1=failure, 2=notrun */
#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>

#include "vxi11.h"
#include "test.h"

int
main(int argc, char *argv[])
{
	char *prog = basename(argv[0]);
	vxi11_handle_t vp;
	long err;

	if (argc == 2) {
		printf("%s: open and close the device by addr\n", prog);
		exit(0);
	}

	vp = vxi11_open(TEST_GWADDR, TEST_DEVNAME, 0, 0, 0, 0, &err);
	if (!vp) {
		fprintf(stderr, "%s: open: %s\n", prog, vxi11_strerror(err));
		exit(1);
	}

	vxi11_close(vp);

	exit(0);
}
