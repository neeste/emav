/* ============================= rdils.c ==================================
   read an ILS format simulus file
   ======================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#define _read read
#define _open open
#define _close close
#endif /* WIN32 */

#ifndef O_BINARY
#define O_BINARY  0
#endif
#define OMODE (O_RDONLY|O_BINARY)

#define ILS -32000
#define BLKSIZ 256

/* read an ILS file into a buffer */
int
rdils(char *filnam, short *buffer, int32_t buflen, int32_t *prate, int32_t *pnpts)
{
    int fd, j, blocks;
    int32_t i, rate;
    short *ptr, *ils_header, rate_exp;

    fd = _open(filnam, OMODE);	/* open the file */
    if (fd < 0)
	return (fd);

    ils_header = buffer;
    /* read ILS header */
    _read(fd, (char *) ils_header, BLKSIZ * sizeof(short));
    if (ils_header[62] != ILS) {
	_close(fd);
	return (ILS);
    }
    *pnpts = ils_header[5] * BLKSIZ;
    rate = ils_header[61];
    rate_exp = ils_header[60];
    while (rate_exp--)
	rate *= 10;
    *prate = rate;
    blocks = buflen / BLKSIZ;
    if (ils_header[5] < blocks)
	blocks = ils_header[5];

    ptr = buffer;
    for (j = 0; j < blocks; j++) {
	/* read data */
	_read(fd, (char *) ptr, BLKSIZ * sizeof(short));
	ptr += BLKSIZ;
    }
    _close(fd);

    for (i = blocks * BLKSIZ; i < buflen; i++)
	buffer[i] = 0;

    return (blocks);
}
