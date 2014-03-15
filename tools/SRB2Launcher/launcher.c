#include <windows.h>

#include "launcher.h"

int WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpvReserved)
{
	return TRUE;
}


void CONS_Printf(char *fmt, ...)
{
	MessageBox(NULL, fmt, "Master Server", 0);
}

void I_Error (char *error, ...)
{
	MessageBox(NULL, error, "Master Server", MB_ICONERROR);
}
