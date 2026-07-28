/* tokio.h */

#ifdef _MSC_VER
#include <io.h>
#else
#include <unistd.h>
#define _open  open
#define _close close
#define _read  read
#define _write write
#define _lseek lseek
#define _write write
#endif

#ifndef O_BINARY
#define O_BINARY 0
#endif

