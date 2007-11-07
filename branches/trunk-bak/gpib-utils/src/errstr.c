#include <stdio.h>

#include "errstr.h"

char *
finderr(errstr_t *tab, int num)
{
    errstr_t *tp;
    static char tmpbuf[64];

    (void)sprintf(tmpbuf, "error %d", num);
    for (tp = &tab[0]; tp->str; tp++)
        if (tp->num == num)
            break;

    return tp->str ? tp->str : tmpbuf;
}
