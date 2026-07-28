// newext.c

#include <string.h>

// apply new extension to a filename
void
newext(char *f, char *e)
{
    while (*f != '.' && *f != '\0')
	f++;
    *f++ = '.';
    while (*e != '\0')
	*f++ = *e++;
    *f++ = '\0';
}


// append blank spaces to end of string
void
fillinblank(char *s, int n)
{
    int     i;

    s[n] = 0;
    i = strlen(s);
    while (i < n)
	s[i++] = ' ';
}

// trims trailing whitespace from end of string
void 
trim(char *s)
{
    char *t = s;

    while (*s)			    // find EOS
	s++;
    while (s > t && s[-1] <= ' ')   // back up over white space
	s--;
    *s = '\0';
}
