#include "../command.h"
#include "../s_sound.h"
#include "../i_sound.h"

//
// CD MUSIC I/O
//

UINT8 cdaudio_started = 0;

consvar_t cd_volume = {"cd_volume","31",CV_SAVE,soundvolume_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cdUpdate  = {"cd_update","1",CV_SAVE, NULL, NULL, 0, NULL, NULL, 0, 0, NULL};


FUNCMATH void I_InitCD(void){}

FUNCMATH void I_StopCD(void){}

FUNCMATH void I_PauseCD(void){}

FUNCMATH void I_ResumeCD(void){}

FUNCMATH void I_ShutdownCD(void){}

FUNCMATH void I_UpdateCD(void){}

FUNCMATH void I_PlayCD(UINT8 track, UINT8 looping)
{
	(void)track;
	(void)looping;
}

FUNCMATH boolean I_SetVolumeCD(int volume)
{
	(void)volume;
	return false;
}
