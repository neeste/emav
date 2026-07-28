/* topen.c */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "tokio.h"
#include "token.h"

#define OPMOD       (O_RDONLY|O_BINARY)
#define PMODE       (S_IREAD|S_IWRITE)

#define ILS -32000  // ILS sampled-data file type

/*
 * Opens existing token file and returns a pointer to a tokenfile structure.
 */
TOKENFILE *
topen(char *file_name)
{
    TOKENFILE *t = 0;
    int     fd;

    fd = _open(file_name, OPMOD, PMODE);
    if (fd == -1) {
	return (NULL);		/* unsuccessful */
    }
    t = (TOKENFILE *) malloc(sizeof(*t));
    t->fd = fd;
    trdheader(t);
    if (t->header.ftype != ILS) {
	_close(fd);
	free(t);
	return (0);
    }
    trdhitch(t);
    if (t->hitch.curtok > 0)
	trdcar(t->hitch.curtok, t);
    return (t);
}
