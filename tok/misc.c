/* misc.c */

#include <stdio.h>
#include <ctype.h>
#include <conio.h>

#define putstr(s) fprintf(stdout,s)

/*
 * Wait for key press
 */
void
waitkp()
{
    putstr("\n\nPress any key to continue...\n");
    (void) getch();
}

/*
 * get integer from command line
 */
int 
getint(char *prompt)
{
    long    atol();
    char    str[80];

    putstr(prompt);
    (void) gets(str);
    return ((int) atol(str));
}

/*
 * get single digit choice
 */
int 
getchoice(char *prompt)
{
    int     c;

    putstr(prompt);
    c = getch();
    putstr("\n");
    return (c - '0');
}

/*
 * get single character (fold to upper case)
 */
int 
getchup(char *prompt)
{
    int     c;

    putstr(prompt);
    c = getch();
    putstr("\n");
    return (toupper(c));
}

/*
 * get floating point from command line
 */
double 
getfloat(char *prompt)
{
    double  atof();
    char    str[80];

    putstr(prompt);
    (void) gets(str);
    return ((double) atof(str));
}
/*
 * clear the screen (vt100 terminal)
 */
void
clr()
{
    putstr("\033[H\033[0J");
}

/*
 * Get a yes or no answer
 */
int 
yesno(char *a, char *b)
{
    int     c;

    printf(a, b);
    printf(" (y or n) [n]: ");
    c = getch();
    if (toupper(c) == 'Y') {
	putstr("YES\n");
	return (1);
    } else {
	putstr("NO\n");
	return (0);
    }
}

void
beep()
{
    (void) putch(7);
}

/*
 * get long integer from command line
 */
int32_t 
getlong(char *prompt)
{
    long    atol();
    char    str[80];

    putstr(prompt);
    (void) gets(str);
    return (atol(str));
}
