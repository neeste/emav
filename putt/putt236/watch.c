#include <stdio.h>

/* watch.c */

int main(void)
{
    printf("Test Printer Port Button or press any key to quit.\n");
    while (!kbhit())
	printf("%s\r", button_down() ? "on " : "off");
}
