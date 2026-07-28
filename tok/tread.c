/* tread.c */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "tokio.h"
#include "token.h"

/*
 * Reads token number t.car.tokeno, and the block of                 
 * information (called the car) concerning that token.  The file      
 * must have already been opened by the topen routine.  The pointer 
 * to the block of token data, t.car.block will be calculated and     
 * set automatically.  pbuf points to the token data buffer.      
 */
int
tread(short *pbuf, int itok, TOKENFILE *t)  /* read selected token */
{
    if (itok < 1)
		return (-1);
    trdcar(itok, t);
    return (trddata(pbuf, itok, t));
}

/*
 * trdheader - read the token file header.
 * Assumes the file has been opened and that t->fd is a valid file descriptor.
 */
void
trdheader(TOKENFILE *t)
{
    _lseek(t->fd, 0L, 0);
    _read(t->fd, (char *) &(t->header), sizeof(t->header));	/* read header */
}

/*
 * trdhitch - read the token file hitch.
 * Assumes the header has been read.
 */
void
trdhitch(TOKENFILE *t)
{
    _lseek(t->fd, (long) (t->header.blocks + 1) << 9, 0);
    _read(t->fd, (char *) &(t->hitch), sizeof(t->hitch));	/* read hitch */
}

/*
 * trdcar - read the token file car identified by itok.
 * Assumes the hitch has been read.
 */
void
trdcar(int itok, TOKENFILE *t)
{
    _lseek(t->fd, (long) (t->header.blocks + 1 + itok) << 9, SEEK_SET);
    _read(t->fd, (char *) &(t->car), sizeof(t->car));	/* read car */
    if (sizeof(t->car) > 202) {   /* fix 64-bit structure alignment */
        int sz = sizeof(t->car);
        char *car = (char *) &t->car.tokeno;
        memmove(car +   4, car +   2, sz -   4); /* block   */
        memmove(car +  40, car +  38, sz -  40); /* toktime */
        memmove(car + 184, car + 182, sz - 184); /* scale   */
    }
}

/*
 * trddata - read token file data identified by itok into a buffer at pbuf.
 * Assumes the hitch and car have been read.
 */
int
trddata(short *pbuf, int itok, TOKENFILE *t)
{
    int     i, nblocks, nbytes;

    nblocks = (t->hitch.toksiz + 255) / 256;
    nbytes = 0;
    _lseek(t->fd, (long) t->car.block << 9, SEEK_SET);
    for (i = 0; i < nblocks; i++) {
	nbytes += _read(t->fd, (char *) pbuf, 512);	/* read token data */
	pbuf += 256;
    }
    return (nbytes);
}
