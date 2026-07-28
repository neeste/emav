/* tclose.c */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "tokio.h"
#include "token.h"

void
tclose(TOKENFILE *t)
{
    _close(t->fd);
    free((char *) t);
}
