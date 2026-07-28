/* delay.c */

#include <stdio.h>
#include <dos.h>

/* delay - timed delay for n/10 seconds */
void
delay(int n)
{
    long count;
    long far *clock = (long far *) 0x46C;
	
    count = *clock + (n * 18L) / 10;
    while(*clock < count)
        continue;
}
