/* button.c */

#ifdef NEVER
#include <conio.h>
#endif /* NEVER */

int
button_down()
{
#ifdef NEVER
    static struct {int buff, stat, ctrl;} ptr = {0x378, 0x379, 0x37A};
    static struct {int yet_, mask;} luk = {0, 0x20};

    if (!luk.yet_) {
	_outp((char) ptr.ctrl, 0);
	_outp((char) ptr.buff, 0xff);
	luk.yet_ = 1;
    }
    return (_inp((char) ptr.stat) & luk.mask);
#endif /* NEVER */

    return (0);
}
