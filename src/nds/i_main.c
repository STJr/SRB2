#include "../doomdef.h"
#include "../d_main.h"
#include "../m_argv.h"
#include "../i_system.h"

int main(int argc, char **argv)
{
	myargc = argc;
	myargv = argv; /// \todo pull out path to exe from this string

	CONS_Printf("I_StartupSystem...");
	I_StartupSystem();

	// startup SRB2
	CONS_Printf("Setting up SRB2...\n");
	D_SRB2Main();
	CONS_Printf("Entering main game loop...\n");
	// never return
	D_SRB2Loop();

	// return to OS
#ifndef __GNUC__
	return 0;
#endif
}
