/* sfile.c - system dependent file routines */

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#ifdef WIN32
#include <windows.h>
#endif /* WIN32 */
#ifdef linux
#include <stdlib.h>
#include <dirent.h>
int amatch();
#endif /* linux */

#define ATTRIB (_A_NORMAL | _A_RDONLY | _A_HIDDEN)

void fillinblank(char *, int);

extern char line[];
extern char o_file_name[];
extern char pr_port[];
extern char tokstr[];

#ifdef DJ
int ncfn = 12;
#else
int ncfn = 24;
#endif /* DJ */

int
count_files()
{
#ifdef WIN32
    int     nf = 0;
    HANDLE  hnd;
    WIN32_FIND_DATA dest;

    hnd = FindFirstFile(o_file_name, &dest);
    if (hnd != INVALID_HANDLE_VALUE) {
        nf = 1;
        while (FindNextFile(hnd, &dest))
            nf++;
    }
    return (nf);
#endif /* WIN32 */
#ifdef linux
    struct dirent **namlst;
    int i, n, c = 0;

    n = scandir(".", &namlst, 0, alphasort);
    if (n > 0) {
        for (i = 0; i < n; i++) {
	    if (amatch(namlst[i]->d_name, o_file_name)) {
		c++;
	    }
            free(namlst[i]);
        }
        free(namlst);
    }
return (c);
#endif /* linux */
}

void
take_files_bwt(int start, int end, char *files)
{
#ifdef WIN32
    char   *s;
    int     i;
    HANDLE  hnd;
    WIN32_FIND_DATA dest;

    hnd = FindFirstFile(o_file_name, &dest);
    for (i = 0; i < start; i++)
	(void) FindNextFile(hnd, &dest);
    for (; i < end; i++) {
	s = files + (i - start) * (ncfn + 1);
	strncpy(s, dest.cFileName, ncfn);
	fillinblank(s, ncfn);
	(void) FindNextFile(hnd, &dest);
    }
#endif /* WIN32 */
#ifdef linux
    char   *s;
    struct dirent **namlst;
    int i, n, c = 0;

    n = scandir(".", &namlst, 0, alphasort);
    if (n > 0) {
        for (i = 0; i < n; i++) {
	    if (amatch(namlst[i]->d_name, o_file_name)) {
		if (c >= start && c < end) {
		    s = files + (c - start) * (ncfn + 1);
		    strncpy(s, namlst[i]->d_name, ncfn);
		    fillinblank(s, ncfn);
		}
		c++;
	    }
            free(namlst[i]);
        }
        free(namlst);
    }
#endif /* linux */
}

int
today_file(char *fn)
{
#ifdef WIN32    /* NYI */
    return (0);
#endif /* WIN32 */
#ifdef linux
    return (0);
#endif /* linux */
}

void
file_flush(FILE *fp)
{
#ifdef WIN32
    fflush(fp);
#endif /* WIN32 */
#ifdef DJ
    _close(_dup(_fileno(fp)));
#endif /* DJ */
#ifdef linux
    fflush(fp);
#endif /* linux */
}
