/* 0=success, 1=failure, 2=notrun */
#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>

#include "vxi11_device.h"
#include "test.h"

#define TEST_DESC "vxi11dev_t create/destroy"

int
main(int argc, char *argv[])
{
	char *prog = basename(argv[0]);
	vxi11dev_t v;

	if (argc > 1) {
		printf("%s\n", TEST_DESC);
		exit(0);
	}

	vxi11_set_device_debug(1);
	v = vxi11_create();
	if (!v) {
		fprintf(stderr, "%s: vxi11_create failed\n", prog);
		exit(1);
	}
	vxi11_destroy(v);

	exit(0);
}
