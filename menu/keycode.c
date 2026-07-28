/* keycode.c */

#include <stdio.h>
#ifdef _MSC_VER
#include <bios.h>
#else /* _MSC_VER */
#ifdef DOS
#include <conio.h>
#define _kbhit kbhit
#define _getch getch
#else /* DOS */
int _kbhit(void);
int _getch(void);
#endif /* DOS */
#endif /* _MSC_VER */

int
main()
{
    int c, d, k = 0;

    while (k != 3) {
#ifdef _MSC_VER
	if (_bios_keybrd(_NKEYBRD_READY)) {
	    k = _bios_keybrd(_NKEYBRD_READ);
#else /* _MSC_VER */
	if (_kbhit()) {
	    k = _getch();
#endif /* _MSC_VER */
	    c = (k >> 8) & 0xFF;
	    d = k & 0xFF;
	    if ((k & 0xFF) == 0xE0 ||
	        (k & 0xFF) == 0x00) {
                printf("%04X = (FN | %d)\n", k, c);
            } else if (d > ' ') {
                printf("%04X = %d = %c\n", k, d, d);
            } else if (d == ' ') {
                printf("%04X = %d = [Space]\n", k, d);
            } else {
                printf("%04X = %d = ^%c\n", k, d, d + '@');
            }
        }
    }
    return (0);
}
