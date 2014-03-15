// SRB2MP.cpp : Defines the entry point for the application.
//

#include "StdAfx.h"
#include "lump.h"

#define APPTITLE "SRB2 Music Player"
#define APPVERSION "v0.1"
#define APPAUTHOR "SSNTails"
#define APPCOMPANY "Sonic Team Junior"

static FSOUND_STREAM *fmus = NULL;
static int fsoundchannel = -1;
static FMUSIC_MODULE  *mod = NULL;
static struct wadfile* wfptr = NULL;

static inline VOID M_SetVolume(int volume)
{
	if (mod && FMUSIC_GetType(mod) != FMUSIC_TYPE_NONE)
		FMUSIC_SetMasterVolume(mod, volume);
	if (fsoundchannel  != -1)
		FSOUND_SetVolume(fsoundchannel, volume);
}

static inline BOOL M_InitMusic(VOID)
{
	if (FSOUND_GetVersion() < FMOD_VERSION)
	{
		printf("Error : You are using the wrong DLL version!\nYou should be using FMOD %.02f\n", FMOD_VERSION);
		return FALSE;
	}

#ifdef _DEBUG
	FSOUND_SetOutput(FSOUND_OUTPUT_WINMM);
#endif

	if (!FSOUND_Init(44100, 1, FSOUND_INIT_GLOBALFOCUS))
	{
		printf("%s\n", FMOD_ErrorString(FSOUND_GetError()));
		return FALSE;
	}
	return TRUE;
}

static inline VOID M_FreeMusic(VOID)
{
	if (mod)
	{
		FMUSIC_StopSong(mod);
		FMUSIC_FreeSong(mod);
		mod = NULL;
	}
	if (fmus)
	{
		FSOUND_Stream_Stop(fmus);
		FSOUND_Stream_Close(fmus);
		fmus = NULL;
		fsoundchannel = -1;
	}
}

static inline VOID M_ShutdownMusic(VOID)
{
	M_FreeMusic();
	FSOUND_Close();
	//remove(musicfile); // Delete the temp file
}

static inline VOID M_PauseMusic(VOID)
{
	if (mod && !FMUSIC_GetPaused(mod))
		FMUSIC_SetPaused(mod, TRUE);
	if (fsoundchannel != -1 && FSOUND_IsPlaying(fsoundchannel))
		FSOUND_SetPaused(fsoundchannel, TRUE);
}

static inline VOID M_ResumeMusic(VOID)
{
	if (mod && FMUSIC_GetPaused(mod))
		FMUSIC_SetPaused(mod, FALSE);
	if (fsoundchannel != -1 && FSOUND_GetPaused(fsoundchannel))
		FSOUND_SetPaused(fsoundchannel, FALSE);
}

static inline VOID M_StopMusic(VOID)
{
	if (mod)
		FMUSIC_StopSong(mod);
	if (fsoundchannel != -1 && fmus && FSOUND_IsPlaying(fsoundchannel))
		FSOUND_Stream_Stop(fmus);
}

static inline VOID M_StartFMODSong(LPVOID data, int len, int looping, HWND hDlg)
{
	const int loops = FSOUND_LOOP_NORMAL|FSOUND_LOADMEMORY;
	const int nloop = FSOUND_LOADMEMORY;
	M_FreeMusic();

	if (looping)
		mod = FMUSIC_LoadSongEx(data, 0, len, loops, NULL, 0);
	else
		mod = FMUSIC_LoadSongEx(data, 0, len, nloop, NULL, 0);

	if (mod)
	{
		FMUSIC_SetLooping(mod, (signed char)looping);
		FMUSIC_SetPanSeperation(mod, 0.0f);
	}
	else
	{
		if (looping)
			fmus = FSOUND_Stream_Open(data, loops, 0, len);
		else
			fmus = FSOUND_Stream_Open(data, nloop, 0, len);
	}

	if (!fmus && !mod)
	{
		MessageBoxA(hDlg, FMOD_ErrorString(FSOUND_GetError()), "Error", MB_OK|MB_APPLMODAL);
		return;
	}

	// Scan the OGG for the COMMENT= field for a custom loop point
	if (looping && fmus)
	{
		const BYTE *origpos, *dataum = data;
		size_t scan, size = len;

		CHAR buffer[512];
		BOOL titlefound = FALSE, artistfound = FALSE;

		unsigned int loopstart = 0;

		origpos = dataum;

		for(scan = 0; scan < size; scan++)
		{
			if (!titlefound)
			{
				if (*dataum++ == 'T')
				if (*dataum++ == 'I')
				if (*dataum++ == 'T')
				if (*dataum++ == 'L')
				if (*dataum++ == 'E')
				if (*dataum++ == '=')
				{
					size_t titlecount = 0;
					CHAR title[256];
					BYTE length = *(dataum-10) - 6;

					while(titlecount < length)
						title[titlecount++] = *dataum++;

					title[titlecount] = '\0';

					sprintf(buffer, "Title: %s", title);

					SendMessage(GetDlgItem(hDlg, IDC_TITLE), WM_SETTEXT, 0, (LPARAM)(LPCSTR)buffer);

					titlefound = TRUE;
				}
			}
		}

		dataum = origpos;

		for(scan = 0; scan < size; scan++)
		{
			if (!artistfound)
			{
				if (*dataum++ == 'A')
				if (*dataum++ == 'R')
				if (*dataum++ == 'T')
				if (*dataum++ == 'I')
				if (*dataum++ == 'S')
				if (*dataum++ == 'T')
				if (*dataum++ == '=')
				{
					size_t artistcount = 0;
					CHAR artist[256];
					BYTE length = *(dataum-11) - 7;

					while(artistcount < length)
						artist[artistcount++] = *dataum++;

					artist[artistcount] = '\0';

					sprintf(buffer, "By: %s", artist);

					SendMessage(GetDlgItem(hDlg, IDC_ARTIST), WM_SETTEXT, 0, (LPARAM)(LPCSTR)buffer);

					artistfound = TRUE;
				}
			}
		}

		dataum = origpos;

		for(scan = 0; scan < size; scan++)
		{
			if (*dataum++ == 'C'){
			if (*dataum++ == 'O'){
			if (*dataum++ == 'M'){
			if (*dataum++ == 'M'){
			if (*dataum++ == 'E'){
			if (*dataum++ == 'N'){
			if (*dataum++ == 'T'){
			if (*dataum++ == '='){
			if (*dataum++ == 'L'){
			if (*dataum++ == 'O'){
			if (*dataum++ == 'O'){
			if (*dataum++ == 'P'){
			if (*dataum++ == 'P'){
			if (*dataum++ == 'O'){
			if (*dataum++ == 'I'){
			if (*dataum++ == 'N'){
			if (*dataum++ == 'T'){
			if (*dataum++ == '=')
			{
				size_t newcount = 0;
				CHAR looplength[64];
				while (*dataum != 1 && newcount < 63)
				{
					looplength[newcount++] = *dataum++;
				}

				looplength[newcount] = '\n';

				loopstart = atoi(looplength);
			}
			else
				dataum--;}
			else
				dataum--;}
			else
				dataum--;}
			else
				dataum--;}
			else
				dataum--;}
			else
				dataum--;}
			else
				dataum--;}
			else
				dataum--;}
			else
				dataum--;}
			else
				dataum--;}
			else
				dataum--;}
			else
				dataum--;}
			else
				dataum--;}
			else
				dataum--;}
			else
				dataum--;}
			else
				dataum--;}
			else
				dataum--;}
		}

		if (loopstart > 0)
		{
			const int length = FSOUND_Stream_GetLengthMs(fmus);
			const int freq = 44100;
			const unsigned int loopend = (unsigned int)((freq/1000.0f)*length-(freq/1000.0f));
			if (!FSOUND_Stream_SetLoopPoints(fmus, loopstart, loopend))
			{
				printf("FMOD(Start,FSOUND_Stream_SetLoopPoints): %s\n",
					FMOD_ErrorString(FSOUND_GetError()));
			}
		}
	}

	if (mod)
		FMUSIC_PlaySong(mod);
	if (fmus)
		fsoundchannel = FSOUND_Stream_PlayEx(FSOUND_FREE, fmus, NULL, FALSE);

	M_SetVolume(128);
}

static inline VOID FreeWADLumps(VOID)
{
	M_FreeMusic();
	if (wfptr) free_wadfile(wfptr);
	wfptr = NULL;
}

static inline VOID ReadWADLumps(LPCSTR WADfilename, HWND hDlg)
{
	HWND listbox = GetDlgItem(hDlg, IDC_PLAYLIST);
	FILE* f;
	struct lumplist *curlump;

	SendMessage(listbox, LB_RESETCONTENT, 0, 0);
	FreeWADLumps();
	f = fopen(WADfilename, "rb");
	wfptr = read_wadfile(f);
	fclose(f);

	/* start of C_LIST */
	/* Loop through the lump list, printing lump info */
	for(curlump = wfptr->head->next; curlump; curlump = curlump->next)
	{
		LPCSTR lumpname = get_lump_name(curlump->cl);
		if (!strncmp(lumpname, "O_", 2) || !strncmp(lumpname, "D_", 2))
			SendMessage(listbox, LB_ADDSTRING, 0, (LPARAM)(LPCSTR)get_lump_name(curlump->cl));
	}
	/* end of C_LIST */
}

//
// OpenWadfile
//
// Provides a common dialog box
// for selecting the desired wad file.
//
static inline VOID OpenWadfile(HWND hDlg)
{
	OPENFILENAMEA ofn;
	CHAR FileBuffer[256] = "";

	ZeroMemory(&ofn, sizeof(ofn));
	ofn.hwndOwner = hDlg;
	ofn.lpstrFilter = "WAD Files\0*.wad\0All Files\0*.*\0\0";
	ofn.lpstrInitialDir = NULL;
	ofn.lpstrFile = FileBuffer;
	ofn.lStructSize = sizeof(ofn);
	ofn.nMaxFile = sizeof(FileBuffer);
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

	if (GetOpenFileNameA(&ofn))
		ReadWADLumps(FileBuffer, hDlg);
}

static inline VOID PlayLump(HWND hDlg)
{
	HWND listbox = GetDlgItem(hDlg, IDC_PLAYLIST);
	LRESULT cursel = SendMessage(listbox, LB_GETCURSEL, 0, 0);

	/* Start of C_EXTRACT */
	CHAR argv[9];

	SendMessage(listbox, LB_GETTEXT, cursel, (LPARAM)(LPCSTR)argv);

	/* Extract LUMPNAME FILENAME pairs */
	if (wfptr)
	{
			struct lumplist *extracted;

			printf("Extracting lump %s...\n", argv);
			/* Find the lump to extract */
			extracted = find_previous_lump(wfptr->head, NULL, argv);
			if (extracted == NULL || (extracted = extracted->next) == NULL)
				return;

			/* Extract lump */
			M_StartFMODSong(extracted->cl->data, extracted->cl->len, FALSE, hDlg);

	/* end of extracting LUMPNAME FILENAME pairs */

	} /* end of C_EXTRACT */
	return;
}

static LRESULT CALLBACK MainWndproc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return DefWindowProc(hWnd, message, wParam, lParam);
}

static INT_PTR CALLBACK DialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
		case WM_INITDIALOG:
			M_InitMusic();
			break;

		case WM_CLOSE:
			EndDialog(hDlg, message);
			break;

		case WM_DESTROY:
			FreeWADLumps();
			M_ShutdownMusic();
			break;

		case WM_COMMAND:
		{
			switch(LOWORD(wParam))
			{
				case 2:
					EndDialog(hDlg, message);
					return TRUE;
				case IDC_ABOUT: // The About button.
				{
					char TempString[256];
					sprintf(TempString, "%s %s by %s - %s", APPTITLE, APPVERSION, APPAUTHOR, APPCOMPANY);
					MessageBoxA(hDlg, TempString, "About", MB_OK|MB_APPLMODAL);
				}
					return TRUE;
				case IDC_OPEN:
					OpenWadfile(hDlg);
					return TRUE;
				case IDC_PLAY:
					PlayLump(hDlg);
					return TRUE;
				default:
					break;
			}
			break;
		}
	}

	return FALSE;
}

static inline VOID RegisterDialogClass(LPCSTR name, WNDPROC callback, HINSTANCE hInst)
{
	WNDCLASSA wnd;

	wnd.style         = CS_HREDRAW | CS_VREDRAW;
	wnd.cbWndExtra    = DLGWINDOWEXTRA;
	wnd.cbClsExtra    = 0;
	wnd.hCursor       = LoadCursorA(NULL,(LPCSTR)MAKEINTRESOURCE(IDC_ARROW));
	wnd.hIcon         = LoadIcon(NULL,MAKEINTRESOURCE(IDI_ICON1));
	wnd.hInstance     = hInst;
	wnd.lpfnWndProc   = callback;
	wnd.lpszClassName = name;
	wnd.lpszMenuName  = NULL;
	wnd.hbrBackground = (HBRUSH)(COLOR_WINDOW);

	if (!RegisterClassA(&wnd))
	{
		return;
	}
}

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
	// Prevent multiples instances of this app.
	CreateMutexA(NULL, TRUE, APPTITLE);

	if (GetLastError() == ERROR_ALREADY_EXISTS)
		return 0;

	RegisterDialogClass("SRB2MP", MainWndproc, hInstance);

	DialogBoxA(hInstance, (LPCSTR)IDD_MAIN, NULL, DialogProc);

	return 0;
}
