/* 0=success, 1=failure, 2=notrun */
#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include <string.h>
#include <sys/time.h>

#include "vxi11_device.h"
#include "test.h"

#define TEST_DESC "3488 create locking deadlock and verify timeout (2s)"
#define ITER 2

int
main(int argc, char *argv[])
{
	char *prog = basename(argv[0]);
	int res;
	vxi11dev_t v[ITER];
	int exit_val = 0;
	int i;
	struct timeval t1, t2;
	unsigned long msec;

	if (argc > 1) {
		printf("%s\n", TEST_DESC);
		exit(0);
	}

	vxi11_set_device_debug(1);
	for (i = 0; i < ITER; i++) {
		v[i] = vxi11_create();
		if (!v[i]) {
			fprintf(stderr, "%s: vxi11_create failed\n", prog);
			exit(1);
		}
		vxi11_set_lockpolicy(v[i], 1, 2000);
		gettimeofday(&t1, NULL);
		res = vxi11_open(v[i], TEST_NAME, 0);
		gettimeofday(&t2, NULL);
		vxi11_perror(v[i], res, "vxi11_open");
		if (i == 0 && res != 0) {
			fprintf(stderr, "first iter should get lock\n");
			exit_val = 1;
			goto done;
		}
		if (i == 1 && res != 11) {
			fprintf(stderr, "2nd iter should get VXI11 err 11\n");
			exit_val = 1;
			goto done;
		}
		msec = _timersubms(&t2, &t1);
		if (i == 1 && msec > 3000) {
			fprintf(stderr, "lock timeout more than 3s: %lu\n", msec);
			exit_val = 1;
			goto done;
		}

	}
	for (i = 0; i < ITER; i++) {
		vxi11_close(v[i]);
		vxi11_destroy(v[i]);
	}
done:
	exit(exit_val);
}
