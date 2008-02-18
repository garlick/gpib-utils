/* 0=success, 1=failure, 2=notrun */
#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include <string.h>
#include <sys/time.h>

#include "vxi11_device.h"
#include "test.h"

#define TEST_DESC "cause an I/O timeout (3s)"

int
main(int argc, char *argv[])
{
	char *prog = basename(argv[0]);
	int res;
	vxi11dev_t v;
	int exit_val = 0;
	char tmpstr[128];
	unsigned long msec;
	struct timeval t1, t2;

	if (argc > 1) {
		printf("%s\n", TEST_DESC);
		exit(0);
	}

	v = vxi11_create();
	if (!v) {
		fprintf(stderr, "%s: vxi11_create failed\n", prog);
		exit(1);
	}
	res = vxi11_open(v, TEST_NAME_VACANT, 0);
	if (res != 0) {
		vxi11_perror(v, res, "vxi11_open");
		exit_val = 1;
		goto done;
	}
	vxi11_set_iotimeout(v, 3000); /* 3s */
	/* this is going to timeout */
	gettimeofday(&t1, NULL);
	res = vxi11_readstr(v, tmpstr, sizeof(tmpstr));
	gettimeofday(&t2, NULL);
	vxi11_perror(v, res, "vxi11_readstr");
	if (res != 15) {
		fprintf(stderr, "readstr did not return VXI-11 err 15\n");
		exit_val = 1;
		goto done;
	}
	msec = _timersubms(&t2, &t1);
	if (msec > 4000) { /* allow 1s slop */
		fprintf(stderr, "3s timeout took too long: %lu msec\n", msec);
		exit_val = 1;
		goto done;
	}
done:
	vxi11_close(v);
	vxi11_destroy(v);
	exit(exit_val);
}
