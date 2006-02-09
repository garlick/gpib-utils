#include <unistd.h>
#include <errno.h>

int
read_all(int fd, unsigned char *buf, int len)
{
    int n;
    int bc = 0;

    do {
        do {
            n = read(fd, buf + bc, len - bc);
        } while (n < 0 && errno == EINTR);
        if (n > 0)
            bc += n;
    } while (n > 0 && bc < len);

    return bc;
}

int
write_all(int fd, unsigned char *buf, int len)
{
    int n;
    int bc = 0;

    do {
        do {
            n = write(fd, buf + bc, len - bc);
        } while (n < 0 && errno == EINTR);
        if (n > 0)
            bc += n;
    } while (n > 0 && bc < len);

    return bc;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
