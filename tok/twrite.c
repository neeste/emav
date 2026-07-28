/* twrite.c */

#include <stdio.h>
#include <stdint.h>
#include "tokio.h"
#include "token.h"

/*
 * twrdata - write token file data identified by itok from buffer at pbuf.
 * Assumes that the hitch and car structures are valid.
 */
int
twrdata(short *pbuf, int itok, TOKENFILE *t)
{
    int     i, nblocks, nbytes;

    nblocks = (t->hitch.toksiz + 255) / 256;
    nbytes = 0;
    _lseek(t->fd, (long) t->car.block << 9, SEEK_SET);
    for (i = 0; i < nblocks; i++) {
	nbytes += _write(t->fd, (char *) pbuf, 512);	/* write token data */
	pbuf += 256;
    }
    return (nbytes);
}

/*
 * Writes token number t.car.tokeno, and the block of                 
 * information (called the car) concerning that token.  The file      
 * must have already been created by the tcreat routine.  The pointer 
 * to the block of token data, t.car.block will be calculated and     
 * set automatically.  It is important to set the pointer to the      
 * token data buffer before calling (e.g. t.buf = &buffer).     	      
 */
int
twrite(short *pbuf, int itok, TOKENFILE *t) /* write selected token */
{
    int     nblocks;

    if (itok < 1)
	return (0);
    nblocks = (t->hitch.toksiz + 255) / 256;
    t->token = itok;
    t->buf = pbuf;
    t->car.tokeno = itok;
    t->car.block = ((t->car.tokeno - 1) * nblocks) + 1;
    t->car.index = 0;
    t->hitch.curtok = itok;

    twrhitch(t);
    twrcar(itok, t);
    return (twrdata(pbuf, itok, t));
}

/*
 * twrheader - write token file header
 * Assumes that the header and file descriptor t->fd are valid.
 */
void
twrheader(TOKENFILE *t)
{
    _lseek(t->fd, (long) 0, SEEK_SET);
    _write(t->fd, (char *) &(t->header), sizeof(t->header));
}

/*
 * twrhitch - write token file hitch.
 * Assumes that the header and hitch structures are valid.
 */
void
twrhitch(TOKENFILE *t)
{
    _lseek(t->fd, (long) (t->header.blocks + 1) << 9, SEEK_SET);
    _write(t->fd, (char *) &(t->hitch), sizeof(t->hitch));	/* write hitch */
}


/*
 * twrcar - write token file car identified by itok.
 * Assumes that the header and car structures are valid.
 */
void
twrcar(int itok, TOKENFILE *t)
{
    _lseek(t->fd, (long) (t->header.blocks + 1 + itok) << 9, SEEK_SET);
    _write(t->fd, (char *) &(t->car), sizeof(t->car));	/* write car */
}

