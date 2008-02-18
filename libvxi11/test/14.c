/* 0=success, 1=failure, 2=notrun */
#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include <string.h>
#include <sys/time.h>
#include <signal.h>
#include <unistd.h>

#include "vxi11_device.h"
#include "test.h"

#define TEST_DESC "abort an I/O before it can timeout (3s)"

static vxi11dev_t v;

void alarm_handler(int arg)
{
	vxi11_abort(v);
}

int
main(int argc, char *argv[])
{
	char *prog = basename(argv[0]);
	int res;
	int exit_val = 0;
	char tmpstr[128];
	unsigned long msec;
	struct timeval t1, t2;

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
	res = vxi11_open(v, TEST_NAME_VACANT, 1);
	if (res != 0) {
		vxi11_perror(v, res, "vxi11_open");
		exit_val = 1;
		goto done;
	}
	vxi11_set_iotimeout(v, 10000); /* 10s */
	signal(SIGALRM, alarm_handler);
	alarm(3); /* 3s */
	gettimeofday(&t1, NULL);
	res = vxi11_readstr(v, tmpstr, sizeof(tmpstr));
	gettimeofday(&t2, NULL);
	vxi11_perror(v, res, "vxi11_readstr");
	if (res != 23) {
		fprintf(stderr, "readstr didn't return VXI11 error 23\n");
		exit_val = 1;
		goto done;
	}
	msec = _timersubms(&t2, &t1);
	if (msec > 5000) { 
		fprintf(stderr, "alarm-based abort probably didn't work\n");
		exit_val = 1;
		goto done;
	}
done:
	vxi11_close(v);
	vxi11_destroy(v);
	exit(exit_val);
}
