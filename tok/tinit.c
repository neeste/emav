/* tinit.c */

#include <stdio.h>
#include <stdint.h>
#include "tokio.h"
#include "token.h"

/*
 * Initialize an existing token file by writing a new header and hitch
 * from the tokenfile structure pointed to by t.
 */
void
tinit(TOKENFILE *t)
{
    int     bpt;		/* blocks per token */

    bpt = (t->hitch.toksiz + 255) / 256;
/*
 * t->header.blocks = number of blocks of data,
 * first block of trailer = t.header.blocks + 1
 */
    t->header.blocks = bpt * t->hitch.tokens;

    if (_lseek(t->fd, (long) 0, SEEK_SET) == -1L) {
	perror("tinit: lseek failed");
    }
    if (_write(t->fd, (char *) &(t->header), sizeof(t->header)) != sizeof(t->header)) {
	perror("tinit: error writing header");	/* write header */
    }
    if (_lseek(t->fd, (long) (t->header.blocks + 1) << 9, SEEK_SET) == -1L) {
	perror("tinit: lseek failed");
    }
    _write(t->fd, (char *) &(t->hitch), sizeof(t->hitch));	/* write hitch */


    _lseek(t->fd, (long) (t->header.blocks + 1 + t->hitch.tokens) << 9, 0);
    if (_write(t->fd, (char *) &(t->car), 512) != 512) {	/* write last car */
	printf("not enough contiguous free disk space\n");
    }
}
