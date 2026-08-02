import sys

with open('evnt_lnx.c', 'r') as f:
    content = f.read()

target = """int
getevent()
{
    int     c;

    while (check_event() == 0)
        if (pgm_terminate)
            event_q = Ctrl_C;
    	else if (execute_it != NULL)
	    (*execute_it) ();

    c = event_q;
    event_q = 0;
    return (c);
}"""

replacement = """#include <unistd.h>
int
getevent()
{
    int     c;

    while (check_event() == 0) {
        if (pgm_terminate) {
            event_q = Ctrl_C;
        } else if (execute_it != NULL) {
            (*execute_it) ();
        } else {
            usleep(1000);
        }
    }

    c = event_q;
    event_q = 0;
    return (c);
}"""

if target in content:
    content = content.replace(target, replacement)
    with open('evnt_lnx.c', 'w') as f:
        f.write(content)
    print("Replaced successfully.")
else:
    print("Target not found.")
