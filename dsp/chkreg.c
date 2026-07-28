/* chkreg.c - check registry for card information */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <windows.h>
#include "cardinfo.h"

void
check_registry(CARDINFO *ct)
{
    char key_name[80], card_name[MAX_CT_NAME];
    int i, j;
    int32_t n, r, t, temp;
    HKEY hKey;

    for (i = 0; i < NCT; i++) {
        sprintf(key_name,"SOFTWARE\\BTNRH\\SysRes\\CardTypes\\CardType%d", i);
	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, key_name, 0, KEY_QUERY_VALUE, &hKey)
	    == ERROR_SUCCESS) {
	    n = MAX_CT_NAME;
	    t = REG_SZ;
	    r = RegQueryValueEx(hKey, "Name", NULL, &t, card_name, &n);
	    if (r == 0) {
		strncpy(ct[i].name, card_name, MAX_CT_NAME);
	    }
	    n = sizeof(int32_t);
	    t = REG_DWORD;
	    r = RegQueryValueEx(hKey, "bits", NULL, &t, (LPBYTE) &temp, &n);
	    if (r == 0) {
		ct[i].bits = (int) temp;
	    }
	    r = RegQueryValueEx(hKey, "left", NULL, &t, (LPBYTE) &temp, &n);
	    if (r == 0) {
		ct[i].left = (int) temp;
	    }
	    r = RegQueryValueEx(hKey, "nbps", NULL, &t, (LPBYTE) &temp, &n);
	    if (r == 0) {
		ct[i].nbps = (int) temp;
	    }
	    r = RegQueryValueEx(hKey, "ncio", NULL, &t, (LPBYTE) &temp, &n);
	    if (r == 0) {
		ct[i].ncad = ct[i].ncda = (int) temp;
	    }
	    r = RegQueryValueEx(hKey, "ncad", NULL, &t, (LPBYTE) &temp, &n);
	    if (r == 0) {
		ct[i].ncad = (int) temp;
	    }
	    r = RegQueryValueEx(hKey, "ncda", NULL, &t, (LPBYTE) &temp, &n);
	    if (r == 0) {
		ct[i].ncda = (int) temp;
	    }
	    r = RegQueryValueEx(hKey, "ad_mv_fs", NULL, &t, (LPBYTE) &temp, &n);
	    if (r == 0) {
		for (j = 0; j < MAXNCH; j++)
		    ct[i].ad_vfs[j] = temp * 0.001;
	    }
	    r = RegQueryValueEx(hKey, "da_mv_fs", NULL, &t, (LPBYTE) &temp, &n);
	    if (r == 0) {
		for (j = 0; j < MAXNCH; j++)
		    ct[i].da_vfs[j] = temp * 0.001;
	    }
	    for (j = 0; j < MAXNCH; j++) {
		sprintf(key_name, "ad%d_mv_fs", j + 1);
		r = RegQueryValueEx(hKey, key_name, NULL, &t, (LPBYTE) &temp, &n);
		if (r == 0) {
		    ct[i].ad_vfs[j] = temp * 0.001;
		}
		sprintf(key_name, "da%d_mv_fs", j + 1);
		r = RegQueryValueEx(hKey, key_name, NULL, &t, (LPBYTE) &temp, &n);
		if (r == 0) {
		    ct[i].da_vfs[j] = temp * 0.001;
		}
	    }
            RegCloseKey(hKey);
	}
    }
}
