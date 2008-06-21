/* 0=success, 1=failure, 2=notrun */
#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include <string.h>

#include "vxi11_device.h"
#include "test.h"

#define TEST_DESC "3488 id query, no locking"

int
main(int argc, char *argv[])
{
	char *prog = basename(argv[0]);
	int res;
	vxi11dev_t v;
	int exit_val = 0;
	char buf[128];

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
	if ((res = vxi11_open(v, TEST_NAME, 0)) != 0) {
		vxi11_perror(v, res, "vxi11_open");
		exit_val = 1;
		goto done;
	}
	if ((res = vxi11_writestr(v, "ID?\n")) != 0) {
		vxi11_perror(v, res, "vxi11_writestr");
		exit_val = 1;
		goto done;
	}
	if ((res = vxi11_readstr(v, buf, sizeof(buf))) != 0) {
		vxi11_perror(v, res, "vxi11_readstr");
		exit_val = 1;
		goto done;
	}
	if (strcmp(buf, "HP3488A\r\n") != 0) {
		fprintf(stderr, "%s: unexpected response to id query: %s", 
				prog, buf);
		exit_val = 1;
		goto done;
	}
done:
	vxi11_close(v);
	vxi11_destroy(v);
	exit(exit_val);
}