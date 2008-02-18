#define TEST_NAME 		"gpibgw:gpib0,8"
#define TEST_NAME_BADHOST	"nonexistent:gpib0,8"
#define TEST_NAME_BADDEV 	"gpibgw:gpib0,300002"
#define TEST_NAME_VACANT 	"gpibgw:gpib0,9"

static inline unsigned long
_timersubms(struct timeval *a, struct timeval *b)
{
	struct timeval t;
	t.tv_sec =  a->tv_sec  - b->tv_sec;
	t.tv_usec = a->tv_usec - b->tv_usec;
	if (t.tv_usec < 0) {
		t.tv_sec--;
		t.tv_usec += 1000000;
	}
	return (t.tv_usec / 1000 + t.tv_sec * 1000);
}

