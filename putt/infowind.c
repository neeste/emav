/* infowind.c */

#include <stdio.h>
#include <string.h>

#include <menu.h>

#include "savage.h"
#include "color.h"

extern char thsfn[];

int     dec_ear(char **);
int     dis_patient(void);

TOGGLETYPE ears = {
    0,
    { "Left", "Right", NULL },
};

PATIENT patient = {"", "", "", 'L', "", ""};

int
reset_cali()
{
    cali_file[0] = 0;
    return(0);
}

MENUITEM info[] =
{
    {"&Last Name :", patient.Lastname, STRING, 12, 0, 1, reset_cali},
    {"&First Name:", patient.Firstname, STRING, 11, 0, 1, reset_cali},
    {"&Ear [R/L] =", (char *) &ears, TOGGLE, 0, 0, 1, NULL},
    {"&ID :", patient.ID, STRING, 11, 0, 1, NULL},
    {"&Date of Birth :", patient.Dob, STRING, 11, 0, 1, NULL},
    {"&Comment   :", patient.Comment, STRING, 81, 0, 1, NULL},
    {"&Probe cal.:", thsfn, STRING, 40, 0, 1, NULL},
    {NULL, NULL, NONE, 0, 0, 0, NULL}
};

int
Info_wind()
{
    int     i;

    i = simple_submenu(menu_x(), menu_y(), info);
    patient.Ear = *ears.strs[ears.at];
    dis_patient();
    return (i);
}
