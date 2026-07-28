/* derror.c */

#ifdef MENU
#include <menu.h>
#else
#include <stdio.h>
#endif /* MENU */

void
derror(char *s, char *t)
{
#ifdef DSP_ERROR_MSG
        fprintf(stderr, "%s\n%s\n", s, t);
#endif
}
