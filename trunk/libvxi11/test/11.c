/* 0=success, 1=failure, 2=notrun */
#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>

#include "vxi11_device.h"
#include "test.h"

#define TEST_DESC "3488 clear, then read status byte"

int
main(int argc, char *argv[])
{
	char *prog = basename(argv[0]);
	int res;
	vxi11dev_t v;
	int exit_val = 0;
	unsigned char status;

	if (argc > 1) {
		printf("%s\n", TEST_DESC);
		exit(0);
	}

	v = vxi11_create();
	if (!v) {
		fprintf(stderr, "%s: vxi11_create failed\n", prog);
		exit(1);
	}
	res = vxi11_open(v, TEST_NAME, 1);
	if (res != 0) {
		vxi11_perror(v, res, "vxi11_open");
		exit_val = 1;
		goto done;
	}
	res = vxi11_clear(v);
	if (res != 0) {
		vxi11_perror(v, res, "vxi11_clear");
		exit_val = 1;
		goto done;
	}
	res = vxi11_readstb(v, &status);
	if (res != 0) {
		vxi11_perror(v, res, "vxi11_readstb");
		exit_val = 1;
		goto done;
	}
	if (status != (0x10)) { /* 0x10 is ready */
		fprintf(stderr, "%s: status != 0x10 (0x%x)\n", prog, status);
		exit_val = 1;
		goto done;
	}
done:
	vxi11_close(v);
	vxi11_destroy(v);
	exit(exit_val);
}
