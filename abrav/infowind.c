/* infowind.c */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <token.h>
#include <menu.h>
#include "savage.h"
#include "color.h"

int     dis_patient(void);
int     nop();

TOGGLETYPE ears = {
    0,
    { "Left", "Right", NULL },
};

TOGGLETYPE epa_pol = {
    1,
    { "Reversed", "Normal", NULL },
};

PATIENT patient = {"", "", "", 'L', "", ""};

int
reset_cali()
{
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
    {"Electrode &Polarity =", (char *) &epa_pol, TOGGLE, 0, 0, 1, NULL},
    {NULL, NULL, NONE, 0, 0, 0, NULL}
};

int
Info_wind()
{
    int     i;

    i = simple_submenu(menu_x(), menu_y(), info);
    patient.Ear = *ears.strs[ears.at];
    (void) dis_patient();
    return (i);
}
