/* tcreat.c */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "tokio.h"
#include "token.h"

#define BLKSIZ 256

/*
 * Creates a token file and returns a pointer to a tokenfile structure.
 * Initializes the token file structure to reasonable values.
 *
 */
TOKENFILE *
tcreat(char *file_name)
{
    TOKENFILE *t = 0;
    int     fd, i;
    int     oflag = O_RDWR | O_BINARY | O_CREAT | O_TRUNC;
    int     pmode = S_IREAD | S_IWRITE;

    fd = _open(file_name, oflag, pmode);
    if (fd == -1) {
	return (NULL);
    }
    t = (TOKENFILE *) malloc(sizeof(*t));

/*
 * Initialize the file descriptor and token count
 */
    t->fd = 0;
    t->token = 0;
/*
 * Initialize the file header
 */
    for (i = 0; i < 5; i++)
	t->header.blank1[i] = 0;
    t->header.blocks = 0;
    for (i = 0; i < 52; i++)
	t->header.blank2[i] = 0;
    t->header.chans = 1;
    t->header.blank3 = 0;
    t->header.power10 = 3;
    t->header.isf = 1;		/* 10 K sampling freq */
    t->header.ftype = -32000;	/* ILS sampled data */
    t->header.stype = 11111;	/* it's a token file */
    for (i = 0; i < 128; i++)
	t->header.pattern[i] = 0;
    t->header.epoch = 1;
    for (i = 0; i < 14; i++)
	t->header.cal[i] = 1L;
    for (i = 0; i < 196; i++)
	t->header.label[i] = 0;
    t->header.lablen = 0;
/*
 * Initialize the first block of the trailer (called the hitch)
 */
    t->hitch.tokens = 0;
    t->hitch.curtok = 0;
    t->hitch.toksiz = 256;	/* size of token in 16 bit words */
    (void) strncpy(t->hitch.intval, "     ", 6);
    (void) strncpy(t->hitch.delay, "     ", 6);
    (void) strncpy(t->hitch.dur, "     ", 6);
    (void) strncpy(t->hitch.comment, " ", COMNTSIZE);

/* Initialize the individual token info block (called a car) */

    t->car.tokeno = 1;
    t->car.block = 1;		/* point to first block after the header */
    t->car.index = 0;		/* point to the first position in that block */
    t->car.rep = 0;
    t->car.reps = 0;
    t->car.swprej = 0;
    t->car.target = 0;
    t->car.toksize = 0;		/* reserved for variable length tokens later */
    t->car.atten1 = 0;
    t->car.atten2 = 0;
    t->car.dccomp = 0;
    t->car.maxval = 0;
    t->car.tokmax = 0;
    t->car.toktime = 0;
    t->car.rejt1 = 0;
    t->car.rejt2 = 0;
    t->car.rejlo = 0;
    t->car.rejhi = 0;
    for (i = 0; i < FNAMESIZE; i++)
	t->car.sname[i] = 0;
    for (i = 0; i < COMNTSIZE; i++)
	t->car.tokcmt[i] = 0;
    for (i = 0; i < 6; i++)
	t->car.tag[i] = 0;

/* Initialize pointer to token data buffer, file descriptor, & filename  */

    t->buf = 0;
    t->fd = fd;
    (void) memcpy(t->fname, file_name, FNAMESIZE);

    return (t);
}
