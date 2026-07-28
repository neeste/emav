/* match.c */

#include <stdio.h>
#include <string.h>
#include <ctype.h>

static int
omatch(char *s, char p)
{
    if (p == '*' || p == '?' 
	|| tolower(p) == tolower(*s)) // case insenitive
	return (1);
    return (0);
}

// match string to pattern
int
amatch(char *s, char *p)
{
    int     i, j, k, o, b;

    o = 0;
    for (j = 0; p[j] != '\0'; j++)
	if (p[j] == '*') {
	    for (i = o; s[i] != '\0' && i >= 0; i++)
                if (omatch(s + i, p[j]) == 0)
		    break;
	    j++;
	    for (k = 0; i >= o; i--)
		if ((k = amatch(s + i, p + j)) > 0)
		    break;
	    return (k);
	} else if ((b = omatch(s + o, p[j])) > 0) {
	    o += b;
	} else {
	    return (0);
	}
    return (1);
}

