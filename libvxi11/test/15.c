/* 0=success, 1=failure, 2=notrun */
#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include <string.h>

#include "vxi11_device.h"
#include "test.h"

#define TEST_DESC "3488 open and read id string 50x with locking"
#define ITER 50

int
main(int argc, char *argv[])
{
	char *prog = basename(argv[0]);
	int res;
	vxi11dev_t v[ITER];
	int exit_val = 0;
	char tmpstr[128];
	char buf[128];
	int i;

	if (argc > 1) {
		printf("%s\n", TEST_DESC);
		exit(0);
	}

	for (i = 0; i < ITER; i++) {
		v[i] = vxi11_create();
		if (!v[i]) {
			fprintf(stderr, "%s: vxi11_create failed\n", prog);
			exit(1);
		}
		res = vxi11_open(v[i], TEST_NAME, 0);
		if (res != 0) {
			sprintf(tmpstr, "open iteration no. %d", i + 1);
			vxi11_perror(v[i], res, tmpstr);
			exit_val = 1;
			goto done;
		}
	}
	for (i = 0; i < ITER; i++) {
		vxi11_set_lockpolicy(v[i], 1, 2000);
		if ((res = vxi11_writestr(v[i], "ID?\n")) != 0) {
			sprintf(tmpstr, "vxi11_writestr iter no. %d", i + 1);
			vxi11_perror(v[i], res, tmpstr);
			exit_val = 1;
			goto done;
		}
		if ((res = vxi11_readstr(v[i], buf, sizeof(buf))) != 0) {
			sprintf(tmpstr, "vxi11_readstr iter no. %d", i + 1);
			vxi11_perror(v[i], res, tmpstr);
			exit_val = 1;
			goto done;
		}
		if (strcmp(buf, "HP3488A\r\n") != 0) {
			fprintf(stderr, "%s: unexpected response to id query on iter %d: %s", 
							prog, i, buf);
			exit_val = 1;
			goto done;
		}
		vxi11_set_lockpolicy(v[i], 0, 2000);
	}
	for (i = 0; i < ITER; i++) {
		vxi11_close(v[i]);
		vxi11_destroy(v[i]);
	}
done:
	exit(exit_val);
}
