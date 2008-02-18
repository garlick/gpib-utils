/* 0=success, 1=failure, 2=notrun */
#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>

#include "vxi11_device.h"
#include "test.h"

#define TEST_DESC "3488 50x open/close with abort setup"

int
main(int argc, char *argv[])
{
	char *prog = basename(argv[0]);
	int res;
	vxi11dev_t v;
	int exit_val = 0;
	char tmpstr[128];
	int i;

	if (argc > 1) {
		printf("%s\n", TEST_DESC);
		exit(0);
	}

	v = vxi11_create();
	if (!v) {
		fprintf(stderr, "%s: vxi11_create failed\n", prog);
		exit(1);
	}
	for (i = 0; i < 50; i++) {
		res = vxi11_open(v, TEST_NAME, 1);
		if (res != 0) {
			sprintf(tmpstr, "open iteration no. %d", i + 1);
			vxi11_perror(v, res, tmpstr);
			exit_val = 1;
			goto done;
		}
		vxi11_close(v);
	}
done:
	vxi11_destroy(v);
	exit(exit_val);
}
