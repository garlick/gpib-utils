/* 0=success, 1=failure, 2=notrun */
#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>

#include "vxi11_device.h"
#include "test.h"

#define TEST_DESC "3488 open/close of unknown host"

int
main(int argc, char *argv[])
{
	char *prog = basename(argv[0]);
	int res;
	vxi11dev_t v;
	int exit_val = 0;

	if (argc > 1) {
		printf("%s\n", TEST_DESC);
		exit(0);
	}

	v = vxi11_create();
	if (!v) {
		fprintf(stderr, "%s: vxi11_create failed\n", prog);
		exit(1);
	}
	res = vxi11_open(v, TEST_NAME_BADHOST, 0);
	if (res == 0) {
		fprintf(stderr, "%s: should have failed\n", prog);
		exit_val = 1;
		goto done;
	}
	vxi11_close(v);
done:
	vxi11_destroy(v);
	exit(exit_val);
}
