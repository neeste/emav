#include <stdlib.h>
/* test.c */

#include <stdio.h>
#include "menu.h"

#define  ESCAPE 27

int     sub_menu(char *);
int     quit_pgm(char *);
int     prn_screen_select(char *);
int     prn_setup(char *);
int     File_wind();
int     Info_wind();

typedef struct {
    char Comment[82];
    char Lastname[13];
    char Firstname[12];
    char Ear;
    char ID[12];
    char Dob[12];
} PATIENT;

char w[40];
float temperature = 37.5;
short years = 1;

MAIN_MENU mainmenu[] = {
    {"&File", File_wind},
    {"&Info", Info_wind},
    {NULL, NULL}
};
MENUITEM file_item[] = {
    {"&Print Screen  (^P)", NULL, NONE, 0, 0, 1, prn_screen_select},
    {"P&rinter Setup...", NULL, NONE, 0, 0, 1, prn_setup},
    {"E&xit", NULL, NONE, 0, 0, 1, quit_pgm},
    {NULL, NULL, NONE, 0, 0, 0, NULL}
};
PATIENT patient = {"Demo program", "Liu", "Zhiqiang", 'L', "", ""};
TOGGLETYPE haircolors = {
    0,
    {
	"Black",
	"Blond",
	"Gray",
	"Red",
	NULL,
    }
};
TOGGLETYPE togglear = {
    0,
    {
	"Left",
	"Right",
	NULL,
    }
};
MENUITEM info[] = {
    {"&Last Name :", patient.Lastname, STRING, 12, 0, 1, NULL},
    {"&First Name:", patient.Firstname, STRING, 11, 0, 1, NULL},
    {"&Ear [R/L] =", (char *) &togglear, TOGGLE, 0, 0, 1, NULL},
    {"&ID :", patient.ID, STRING, 11, 0, 1, NULL},
    {"&Date of Birth :", patient.Dob, STRING, 11, 0, 1, NULL},
    {"&Temperature (Celsus) :", (char *) &temperature, FLOAT, 5, 1, 1, NULL},
    {"&Hair color =", (char *) &haircolors, TOGGLE, 0, 0, 1, NULL},
    {"&Years Worked at BTNRH :", (char *) &years, SHORT, 3, 0, 1, NULL},
    {"&Comment :", patient.Comment, STRING, 81, 0, 1, NULL},
    {"&Options...", NULL, NONE, 0, 0, 1, sub_menu},
    {NULL, NULL, NONE, 0, 0, 0, NULL}
};
MENUITEM sub_opt[] = {
    {"&Where?? :", w, STRING, 40, 0, 1, NULL},
    {"&Where?? :", w, STRING, 40, 0, 1, NULL},
    {"&Where?? :", w, STRING, 40, 0, 1, NULL},
    {"&Where?? :", w, STRING, 40, 0, 1, NULL},
    {NULL, NULL, NONE, 0, 0, 0, NULL}
};

int
sub_menu(char *no_use)
{
    int ret;

    ret = simple_submenu(menu_x() + 40, menu_y() - 40, sub_opt);
    return (ret);
}

int
quit_pgm(char *no_use)
{
    if (decide(1, 1, "Do you really want to exit?"))
        mainmenu_exit();
    return(ESCAPE);
}

int
File_wind()
{
    return(simple_submenu(menu_x(), menu_y(), file_item));
}


int
Info_wind()
{
    return(simple_submenu(menu_x(), menu_y(), info));
}

int
MAIN_PGM(int ac, char **av)
{
    int i, d;

    set_title("Test");
    init_gr();
    clr_scr();
    for (i = 0; i < 8; i++) {
	d = i * 10;
	gr_recto(100 + d, 100 + d, 200 + d, 200 + d,  8 + i);
    }
    mouse_init();
    mainmenu_init(mainmenu);
    mainmenu_show(NULL);
    mainmenu_driver();
    end_gr();

    return (0);
}
