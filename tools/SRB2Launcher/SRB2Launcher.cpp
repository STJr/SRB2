/////////////////////////////
//                         //
//    Sonic Robo Blast 2   //
// Official Win32 Launcher //
//                         //
//           By            //
//        SSNTails         //
//    ah518@tcnet.org      //
//  (Sonic Team Junior)    //
//  http://www.srb2.org    //
//                         //
/////////////////////////////
//
// This source code is released under
// Public Domain. I hope it helps you
// learn how to write exciting Win32
// applications in C!
//
// However, you may not alter this
// program and continue to call it
// the "Official Sonic Robo Blast 2
// Launcher".
//
// NOTE: Not all files in this project
// are released under this license.
// Any license mentioned in accompanying
// source files overrides the license
// mentioned here, sorry!
//
// SRB2Launcher.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include <stdlib.h>
#include <stdio.h>
#include "SRB2Launcher.h"

char TempString[256];

char Arguments[16384];

HWND mainHWND;
HWND hostHWND;
HWND joinHWND;
HINSTANCE g_hInst;

HANDLE ServerlistThread = 0;

typedef struct
{
	char nospecialrings;
	char suddendeath;
	char scoringtype[16];
	char matchboxes[16];
	int respawnitemtime;
	int timelimit;
	int pointlimit;
} matchsettings_t;

typedef struct
{
	char raceitemboxes[16];
	int numlaps;
} racesettings_t;

typedef struct
{
	char nospecialrings;
	char matchboxes[16];
	int respawnitemtime;
	int timelimit;
	int pointlimit;
} tagsettings_t;

typedef struct
{
	char nospecialrings;
	char matchboxes[16];
	int respawnitemtime;
	int timelimit;
	int flagtime;
	int pointlimit;
} ctfsettings_t;

typedef struct
{
	char nofile;
	char nodownload;
} joinsettings_t;

typedef struct
{
	matchsettings_t match;
	racesettings_t race;
	tagsettings_t tag;
	ctfsettings_t ctf;
	char gametype[16];
	char startmap[9];
	int maxplayers;
	char advancestage[16];
	int inttime;
	char forceskin;
	char noautoaim;
	char nosend;
	char noadvertise;

	// Monitor Toggles...
	char teleporters[8];
	char superring[8];
	char silverring[8];
	char supersneakers[8];
	char invincibility[8];
	char jumpshield[8];
	char watershield[8];
	char ringshield[8];
	char fireshield[8];
	char bombshield[8];
	char oneup[8];
	char eggmanbox[8];
} hostsettings_t;

typedef struct
{
	hostsettings_t host;
	joinsettings_t join;
	char EXEName[1024];
	char ConfigFile[1024];
	char ManualParameters[8192];
	char PlayerName[24];
	char PlayerColor[16];
	char PlayerSkin[24];
} settings_t;

// Whole structure is just dumped to a binary file when settings are saved.
settings_t launchersettings;

#define APPTITLE "Official Sonic Robo Blast 2 Launcher"
#define APPVERSION "v0.1"
#define APPAUTHOR "SSNTails"
#define APPCOMPANY "Sonic Team Junior"

LRESULT CALLBACK MainProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

//
// RunSRB2
//
// Runs SRB2
// returns true if successful
//
BOOL RunSRB2(void)
{
	SHELLEXECUTEINFO lpExecInfo;
	BOOL result;
	char EXEName[1024];

	memset(&lpExecInfo, 0, sizeof(SHELLEXECUTEINFO));

	lpExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);

	SendMessage(GetDlgItem(mainHWND, IDC_EXENAME), WM_GETTEXT, sizeof(EXEName), (LPARAM)(LPCSTR)EXEName); 
	lpExecInfo.lpFile = EXEName;
	lpExecInfo.lpParameters = Arguments;
	lpExecInfo.nShow = SW_SHOWNORMAL;
	lpExecInfo.hwnd = mainHWND;
	lpExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
	lpExecInfo.lpVerb = "open";

	result = ShellExecuteEx(&lpExecInfo);

	if(!result)
	{
		MessageBox(mainHWND, "Error starting the game!", "Error", MB_OK|MB_APPLMODAL|MB_ICONERROR);
		return false;
	}

	return true;
}

//
// ChooseEXEName
//
// Provides a common dialog box
// for selecting the desired executable.
//
void ChooseEXEName(void)
{
	OPENFILENAME ofn;
	char FileBuffer[256];

	ZeroMemory(&ofn, sizeof(ofn));
	ofn.hwndOwner = NULL;
	FileBuffer[0] = '\0';
	ofn.lpstrFilter = "Executable Files\0*.exe\0All Files\0*.*\0\0";
	ofn.lpstrInitialDir = NULL;
	ofn.lpstrFile = FileBuffer;
	ofn.lStructSize = sizeof(ofn);
	ofn.nMaxFile = sizeof(FileBuffer);
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

	if(GetOpenFileName(&ofn))
	{
		SendMessage(GetDlgItem(mainHWND, IDC_EXENAME), WM_SETTEXT, 0, (LPARAM)(LPCSTR)FileBuffer);
		strcpy(launchersettings.EXEName, FileBuffer);
	}
}

//
// ChooseConfigName
//
// Provides a common dialog box
// for selecting the desired cfg file.
//
void ChooseConfigName(void)
{
	OPENFILENAME ofn;
	char FileBuffer[256];

	ZeroMemory(&ofn, sizeof(ofn));
	ofn.hwndOwner = NULL;
	FileBuffer[0] = '\0';
	ofn.lpstrFilter = "Config Files\0*.cfg\0All Files\0*.*\0\0";
	ofn.lpstrInitialDir = NULL;
	ofn.lpstrFile = FileBuffer;
	ofn.lStructSize = sizeof(ofn);
	ofn.nMaxFile = sizeof(FileBuffer);
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

	if(GetOpenFileName(&ofn))
	{
		SendMessage(GetDlgItem(mainHWND, IDC_CONFIGFILE), WM_SETTEXT, 0, (LPARAM)(LPCSTR)FileBuffer);
		strcpy(launchersettings.ConfigFile, FileBuffer);
	}
}

//
// Add External File
//
// Provides a common dialog box
// for adding an external file.
//
void AddExternalFile(void)
{
	OPENFILENAME ofn;
	char FileBuffer[256];

	ZeroMemory(&ofn, sizeof(ofn));
	ofn.hwndOwner = NULL;
	FileBuffer[0] = '\0';
	ofn.lpstrFilter = "WAD/SOC Files\0*.soc;*.wad\0All Files\0*.*\0\0";
	ofn.lpstrInitialDir = NULL;
	ofn.lpstrFile = FileBuffer;
	ofn.lStructSize = sizeof(ofn);
	ofn.nMaxFile = sizeof(FileBuffer);
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

	if(GetOpenFileName(&ofn) && SendMessage(GetDlgItem(mainHWND, IDC_EXTFILECOMBO), CB_FINDSTRING, -1, (LPARAM)(LPCSTR)FileBuffer) == CB_ERR)
		SendMessage(GetDlgItem(mainHWND, IDC_EXTFILECOMBO), CB_ADDSTRING, 0, (LPARAM)(LPCSTR)FileBuffer);
}

//
// CompileArguments
//
// Go through ALL of the settings
// and put them into a parameter
// string. Yikes!
//
void CompileArguments(void)
{
	HWND temp;
	int numitems;
	int i;

	// Clear out the arguments, if any existed.
	memset(Arguments, 0, sizeof(Arguments));


	// WAD/SOC Files Added
	temp = GetDlgItem(mainHWND, IDC_EXTFILECOMBO);

	if ((numitems = SendMessage(temp, CB_GETCOUNT, 0, 0)) > 0)
	{
		char tempbuffer[1024];

		strcpy(Arguments, "-file ");

		for (i = 0; i < numitems; i++)
		{
			SendMessage(temp, CB_GETLBTEXT, i, (LPARAM)(LPCSTR)tempbuffer);
			strcat(Arguments, tempbuffer);
			strcat(Arguments, " ");
		}
	}

	// Manual Parameters
	temp = GetDlgItem(mainHWND, IDC_PARAMETERS);

	if (SendMessage(temp, WM_GETTEXTLENGTH, 0, 0) > 0)
	{
		char tempbuffer[8192];

		SendMessage(temp, WM_GETTEXT, 8192, (LPARAM)(LPCSTR)tempbuffer);

		strcat(Arguments, tempbuffer);
	}
}

//
// GetConfigVariable
//
// Pulls a value out of the chosen
// config.cfg and places it into the
// string supplied in 'dest'
//
BOOL GetConfigVariable(char* varname, char* dest)
{
	FILE* f;
	int size = 0;
	char* buffer;
	char* posWeWant;
	char* stringstart;
	char varnamecpy[256];

	varnamecpy[0] = '\n';

	strncpy(varnamecpy+1, varname, 255);

	if(!(f = fopen(launchersettings.ConfigFile, "rb")))
		return false; // Oops!

	// Get file size
	while(!feof(f))
	{
		size++;
		fgetc(f);
	}

	fseek(f, 0, SEEK_SET);

	buffer = (char*)malloc(size);

	fread(buffer, size, 1, f);
	fclose(f);

	posWeWant = strstr(buffer, varname);

	if(posWeWant == NULL)
	{
		free(buffer);
		return false;
	}

	posWeWant++;

	// We are now at the line we want.
	// Get the variable from it

	while(*posWeWant != '\"') posWeWant++;

	posWeWant++;

	stringstart = posWeWant;

	while(*posWeWant != '\"') posWeWant++;

	*posWeWant = '\0';

	strcpy(dest, stringstart);

	free(buffer);

	// Phew!
	return true;
}

SOCKET ConnectSocket(char* IPAddress, int portnumber)
{
	DWORD dwDestAddr;
	SOCKADDR_IN sockAddrDest;
	SOCKET sockDest;

	// Create socket
	sockDest = socket(AF_INET, SOCK_STREAM, 0);

	if(sockDest == SOCKET_ERROR)
	{
//		printf("Could not create socket: %d\n", WSAGetLastError());
		return INVALID_SOCKET;
	}

	// Convert address to in_addr (binary) format
	dwDestAddr = inet_addr(IPAddress);

	if(dwDestAddr == INADDR_NONE)
	{
		// It's not a xxx.xxx.xxx.xxx IP, so resolve through DNS
		struct hostent* pHE = gethostbyname(IPAddress);
		if(pHE == 0)
		{
//			printf("Unable to resolve address.\n");
			return INVALID_SOCKET;
		}

		dwDestAddr = *((u_long*)pHE->h_addr_list[0]);
	}

	// Initialize SOCKADDR_IN with IP address, port number and address family
	memcpy(&sockAddrDest.sin_addr, &dwDestAddr, sizeof(DWORD));
	
	sockAddrDest.sin_port = htons(portnumber);
	sockAddrDest.sin_family = AF_INET;

	// Attempt to connect to server
	if(connect(sockDest, (LPSOCKADDR)&sockAddrDest, sizeof(sockAddrDest)) == SOCKET_ERROR)
	{
//		printf("Could not connect to server socket: %d\n", WSAGetLastError());
		closesocket(sockDest);
		return INVALID_SOCKET;
	}
	
	return sockDest;
}

//
// MS_Write():
//
static int MS_Write(msg_t *msg, SOCKET socket)
{
#ifdef NONET
	msg = NULL;
	return MS_WRITE_ERROR;
#else
	int len;

	if (msg->length < 0)
		msg->length = (long)strlen(msg->buffer);
	len = msg->length + HEADER_SIZE;

	msg->type = htonl(msg->type);
	msg->length = htonl(msg->length);

	if (send(socket, (char *)msg, len, 0) != len)
		return MS_WRITE_ERROR;
	return 0;
#endif
}

//
// MS_Read():
//
static int MS_Read(msg_t *msg, SOCKET socket)
{
#ifdef NONET
	msg = NULL;
	return MS_READ_ERROR;
#else
	if (recv(socket, (char *)msg, HEADER_SIZE, 0) != HEADER_SIZE)
		return MS_READ_ERROR;

	msg->type = ntohl(msg->type);
	msg->length = ntohl(msg->length);

	if (!msg->length) // fix a bug in Windows 2000
		return 0;

	if (recv(socket, (char *)msg->buffer, msg->length, 0) != msg->length)
		return MS_READ_ERROR;
	return 0;
#endif
}

/** Gets a list of game servers from the master server.
  */
static inline int GetServersList(SOCKET socket)
{
	msg_t msg;
	int count = 0;

	msg.type = GET_SERVER_MSG;
	msg.length = 0;
	if (MS_Write(&msg, socket) < 0)
		return MS_WRITE_ERROR;

	while (MS_Read(&msg, socket) >= 0)
	{
		if (!msg.length)
		{
//			if (!count)
//				printf("No server currently running.\n");
			return MS_NO_ERROR;
		}
		count++;
		printf(msg.buffer);
	}

	return MS_READ_ERROR;
}

//
// RunSocketStuff
//
// Thread that checks the masterserver for new games.
void RunSocketStuff(HWND hListView)
{
	WSADATA wsaData;
	SOCKET sock;
	int i = 0;
	char ServerAddressAndPort[256];
	char Address[256];
	char Port[8];

	if(!GetConfigVariable("masterserver", ServerAddressAndPort)) // srb2.servegame.org:28900
	{
		ServerlistThread = NULL;
		return;
	}

	strcpy(Address, ServerAddressAndPort);

	for(i = 0; i < (signed)strlen(Address); i++)
	{
		if(Address[i] == ':')
		{
			Address[i] = '\0';
			break;
		}
	}

	for(i = 0; i < (signed)strlen(ServerAddressAndPort); i++)
	{
		if(ServerAddressAndPort[i] == ':')
		{
			strcpy(Port, &ServerAddressAndPort[i+1]);
			break;
		}
	}

	// Address now contains the hostname or IP
	// Port now contains the port number


	// Initialize WinSock
	if(WSAStartup(MAKEWORD(1, 1), &wsaData) != 0)
	{
//		printf("Could not initialize sockets.\n");
		ServerlistThread = NULL;
		return;
	}

	// Create socket and connect to server
	sock = ConnectSocket(/*IPAddress*/0, /*PortNumber*/0);
	if(sock == INVALID_SOCKET)
	{
//		printf("Socket Error: %d\n", WSAGetLastError());
		ServerlistThread = NULL;
		return;
	}

	// We're connected!
	// Now get information from server


	// Add games to listview box.
	ListView_DeleteAllItems(hListView);

/*
	while (MoreServersStillExist)
	{
		GetTheNextServerInformation();
		AddItemToList(hListView, servername, ping, players, gametype, level);
	}
*/

	// close socket
	closesocket(sock);
		
	// Clean up WinSock
	if(WSACleanup() == SOCKET_ERROR)
	{
//		printf("Could not cleanup sockets: %d\n", WSAGetLastError());
		ServerlistThread = NULL;
		return;
	}

	printf("Winsock thread terminated successfully.\n");
	ServerlistThread = NULL;

	return;
}

BOOL GetGameList(HWND hListView)
{
	DWORD dwThreadID;
	ServerlistThread = CreateThread(  NULL, // default security
										 0, // default stack
										 (LPTHREAD_START_ROUTINE)(void*)RunSocketStuff, // thread function 
										 hListView, // arguments
										 0, // default flags 
										 &dwThreadID); // don't need this, but it makes it happy (and compatible with old Win32 OSes)

	if(ServerlistThread == NULL)
		return false;

	return true;
}

void RegisterDialogClass(char* name, WNDPROC callback)
{
	WNDCLASS wnd;

	wnd.style			= CS_HREDRAW | CS_VREDRAW;
	wnd.cbWndExtra		= DLGWINDOWEXTRA;
	wnd.cbClsExtra		= 0;
	wnd.hCursor			= LoadCursor(NULL,MAKEINTRESOURCE(IDC_ARROW));
	wnd.hIcon			= LoadIcon(NULL,MAKEINTRESOURCE(IDI_ICON1));
	wnd.hInstance		= g_hInst;
	wnd.lpfnWndProc		= callback;
	wnd.lpszClassName	= name;
	wnd.lpszMenuName	= NULL;
	wnd.hbrBackground	= (HBRUSH)(COLOR_WINDOW);

	if(!RegisterClass(&wnd))
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
	CreateMutex(NULL, true, APPTITLE);

	if(GetLastError() == ERROR_ALREADY_EXISTS)
		return 0;

	g_hInst = hInstance;

	RegisterDialogClass("SRB2Launcher", MainProc);

	DialogBox(g_hInst, (LPCSTR)IDD_MAIN, NULL, (DLGPROC)MainProc);

	return 0;
}

//
// InitHostOptionsComboBoxes
//
// Does what it says.
//
void InitHostOptionsComboBoxes(HWND hwnd)
{
	HWND ctrl;

	ctrl = GetDlgItem(hwnd, IDC_MATCHBOXES);

	if(ctrl)
	{
		SendMessage(ctrl, CB_ADDSTRING, 0, (LPARAM)(LPCSTR)"Normal (Default)");
		SendMessage(ctrl, CB_ADDSTRING, 0, (LPARAM)(LPCSTR)"Random");
		SendMessage(ctrl, CB_ADDSTRING, 0, (LPARAM)(LPCSTR)"Non-Random");
		SendMessage(ctrl, CB_ADDSTRING, 0, (LPARAM)(LPCSTR)"None");
	}

	ctrl = GetDlgItem(hwnd, IDC_RACEITEMBOXES);

	if(ctrl)
	{
		SendMessage(ctrl, CB_ADDSTRING, 0, (LPARAM)(LPCSTR)"Normal (Default)");
		SendMessage(ctrl, CB_ADDSTRING, 0, (LPARAM)(LPCSTR)"Random");
		SendMessage(ctrl, CB_ADDSTRING, 0, (LPARAM)(LPCSTR)"Teleports");
		SendMessage(ctrl, CB_ADDSTRING, 0, (LPARAM)(LPCSTR)"None");
	}

	ctrl = GetDlgItem(hwnd, IDC_MATCH_SCORING);

	if(ctrl)
	{
		SendMessage(ctrl, CB_ADDSTRING, 0, (LPARAM)(LPCSTR)"Normal (Default)");
		SendMessage(ctrl, CB_ADDSTRING, 0, (LPARAM)(LPCSTR)"Classic");
	}
}

LRESULT CALLBACK MatchOptionsDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
		case WM_INITDIALOG:
			InitHostOptionsComboBoxes(hwnd);
			break;
	
		case WM_DESTROY:
			EndDialog(hwnd, LOWORD(wParam));
			break;
			
		case WM_COMMAND:
		{
			switch(LOWORD(wParam))
			{
				case 2:
					PostMessage(hwnd, WM_DESTROY, 0, 0);
					break;
			    default:
					break;
			}

			break;
		}
	}

	return 0;
}

LRESULT CALLBACK RaceOptionsDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
		case WM_INITDIALOG:
			InitHostOptionsComboBoxes(hwnd);
			break;
	
		case WM_DESTROY:
			EndDialog(hwnd, LOWORD(wParam));
			break;
			
		case WM_COMMAND:
		{
			switch(LOWORD(wParam))
			{
				case 2:
					PostMessage(hwnd, WM_DESTROY, 0, 0);
					break;
			    default:
					break;
			}

			break;
		}
	}

	return 0;
}

LRESULT CALLBACK TagOptionsDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
		case WM_INITDIALOG:
			InitHostOptionsComboBoxes(hwnd);
			break;
	
		case WM_DESTROY:
			EndDialog(hwnd, LOWORD(wParam));
			break;
			
		case WM_COMMAND:
		{
			switch(LOWORD(wParam))
			{
				case 2:
					PostMessage(hwnd, WM_DESTROY, 0, 0);
					break;
			    default:
					break;
			}

			break;
		}
	}

	return 0;
}

LRESULT CALLBACK CTFOptionsDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
		case WM_INITDIALOG:
			InitHostOptionsComboBoxes(hwnd);
			break;
	
		case WM_DESTROY:
			EndDialog(hwnd, LOWORD(wParam));
			break;
			
		case WM_COMMAND:
		{
			switch(LOWORD(wParam))
			{
				case 2:
					PostMessage(hwnd, WM_DESTROY, 0, 0);
					break;
			    default:
					break;
			}

			break;
		}
	}

	return 0;
}

LRESULT CALLBACK HostProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	HWND temp;

	switch(message)
	{
		case WM_INITDIALOG:
			hostHWND = hwnd;

			temp = GetDlgItem(hwnd, IDC_ADVANCEMAP);
			SendMessage(temp, CB_ADDSTRING, 0, (LPARAM)(LPCSTR)"Off");
			SendMessage(temp, CB_ADDSTRING, 0, (LPARAM)(LPCSTR)"Next (Default)");
			SendMessage(temp, CB_ADDSTRING, 0, (LPARAM)(LPCSTR)"Random");

			temp = GetDlgItem(hwnd, IDC_GAMETYPE);
			SendMessage(temp, CB_ADDSTRING, 0, (LPARAM)(LPCSTR)"Coop");
			SendMessage(temp, CB_ADDSTRING, 0, (LPARAM)(LPCSTR)"Match");
			SendMessage(temp, CB_ADDSTRING, 0, (LPARAM)(LPCSTR)"Team Match");
			SendMessage(temp, CB_ADDSTRING, 0, (LPARAM)(LPCSTR)"Race");
			SendMessage(temp, CB_ADDSTRING, 0, (LPARAM)(LPCSTR)"Time-Only Race");
			SendMessage(temp, CB_ADDSTRING, 0, (LPARAM)(LPCSTR)"Tag");
			SendMessage(temp, CB_ADDSTRING, 0, (LPARAM)(LPCSTR)"CTF");

			break;
	
	    case WM_CREATE:
	    {
	            
	        break;
	    }

		case WM_DESTROY:
		{
			hostHWND = NULL;
			EndDialog(hwnd, LOWORD(wParam));
			break;
		}
			
		case WM_COMMAND:
		{
			switch(LOWORD(wParam))
			{
				case 2:
					PostMessage(hwnd, WM_DESTROY, 0, 0);
					break;
				case IDC_OPTIONS:
					{
						int gametype;
						int dialognum;
						DLGPROC callbackfunc;

						// Grab the current gametype from the IDC_GAMETYPE
						// combo box and then display the appropriate
						// options dialog.
						temp = GetDlgItem(hostHWND, IDC_GAMETYPE);
						switch(SendMessage(temp, CB_GETCURSEL, 0, 0))
						{
							case 0:
								gametype = 0;
								break;
							case 1:
								gametype = 1;
								break;
							case 2:
								gametype = 1;
								break;
							case 3:
								gametype = 2;
								break;
							case 4:
								gametype = 2;
								break;
							case 5:
								gametype = 3;
								break;
							case 6:
								gametype = 4;
								break;
						}

						switch(gametype)
						{
							case 1:
								dialognum = IDD_MATCHOPTIONS;
								callbackfunc = (DLGPROC)MatchOptionsDlgProc;
								break;
							case 2:
								dialognum = IDD_RACEOPTIONS;
								callbackfunc = (DLGPROC)RaceOptionsDlgProc;
								break;
							case 3:
								dialognum = IDD_TAGOPTIONS;
								callbackfunc = (DLGPROC)TagOptionsDlgProc;
								break;
							case 4:
								dialognum = IDD_CTFOPTIONS;
								callbackfunc = (DLGPROC)CTFOptionsDlgProc;
								break;
							case 0:
							default: // ???
								dialognum = 0;
								callbackfunc = NULL;
								MessageBox(hostHWND, "This gametype does not have any additional options.", "Not Available", MB_OK|MB_APPLMODAL);
								break;
						}

						if(dialognum)
							DialogBox(g_hInst, (LPCSTR)dialognum, hostHWND, callbackfunc);
					}
					break;
			    default:
					break;
			}

			break;
		}

		case WM_PAINT:
		{
			break;
		}
	}

	return 0;
}

//
// AddItemToList
//
// Adds a game to the list view.
//
void AddItemToList(HWND hListView, char* servername,
				   char* ping, char* players,
				   char* gametype, char* level)
{
	LVITEM			lvTest;

	lvTest.mask = LVIF_TEXT | LVIF_STATE;

	lvTest.pszText = servername;
	lvTest.iIndent = 0;
	lvTest.stateMask = 0;
	lvTest.state = 0;

	lvTest.iSubItem = 0;
	lvTest.iItem = ListView_InsertItem(hListView, &lvTest);

	ListView_SetItemText(hListView,lvTest.iItem,1,ping);

	ListView_SetItemText(hListView,lvTest.iItem,2,players);

	ListView_SetItemText(hListView,lvTest.iItem,3,gametype);

	ListView_SetItemText(hListView,lvTest.iItem,4,level);
}

//
// InitJoinGameColumns
//
// Initializes the column headers on the listview control
// on the Join Game page.
//
void InitJoinGameColumns(HWND hDlg)
{
	HWND hItemList;
	LVCOLUMN		columns[10];
	int i = 0;

	//Create the columns in the list control
	hItemList = GetDlgItem(hDlg, IDC_GAMELIST);

	columns[i].mask = LVCF_TEXT | LVCF_WIDTH;
	columns[i].pszText = "Server Name";
	columns[i].cchTextMax = 256;
	columns[i].cx = 120;
	columns[i].fmt = LVCFMT_LEFT;
	ListView_InsertColumn(hItemList, i, &columns[i]);

	i++;

	columns[i].mask = LVCF_TEXT | LVCF_WIDTH;
	columns[i].pszText = "Ping";
	columns[i].cchTextMax = 256;
	columns[i].cx = 80;
	columns[i].fmt = LVCFMT_LEFT;
	ListView_InsertColumn(hItemList, i, &columns[i]);

	i++;

	columns[i].mask = LVCF_TEXT | LVCF_WIDTH;
	columns[i].pszText = "Players";
	columns[i].cchTextMax = 256;
	columns[i].cx = 80;
	columns[i].fmt = LVCFMT_LEFT;
	ListView_InsertColumn(hItemList, i, &columns[i]);

	i++;

	columns[i].mask = LVCF_TEXT | LVCF_WIDTH;
	columns[i].pszText = "Game Type";
	columns[i].cchTextMax = 256;
	columns[i].cx = 80;
	columns[i].fmt = LVCFMT_LEFT;
	ListView_InsertColumn(hItemList, i, &columns[i]);

	i++;

	columns[i].mask = LVCF_TEXT | LVCF_WIDTH;
	columns[i].pszText = "Level";
	columns[i].cchTextMax = 256;
	columns[i].cx = 120;
	columns[i].fmt = LVCFMT_LEFT;
	ListView_InsertColumn(hItemList, i, &columns[i]);
}

LRESULT CALLBACK JoinProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
		case WM_INITDIALOG:
			joinHWND = hwnd;
			InitJoinGameColumns(hwnd);
			// Start server listing thread
			// and grab game list.
			GetGameList(GetDlgItem(hwnd, IDC_GAMELIST));
			break;
	
		case WM_DESTROY:
			joinHWND = NULL;
			// Terminate server listing thread.
			EndDialog(hwnd, LOWORD(wParam));
			break;
			
		case WM_COMMAND:
		{
			switch(LOWORD(wParam))
			{
				case 2:
					PostMessage(hwnd, WM_DESTROY, 0, 0);
					break;
				case IDC_SEARCHGAMES:
					if(ServerlistThread == NULL)
						GetGameList(GetDlgItem(hwnd, IDC_GAMELIST));
					break;
			    default:
					break;
			}

			break;
		}

		case WM_PAINT:
		{
			break;
		}
	}

	return 0;
}

void InitializeDefaults(void)
{
	memset(&launchersettings, 0, sizeof(launchersettings));
	strcpy(launchersettings.EXEName, "srb2win.exe");
	strcpy(launchersettings.ConfigFile, "config.cfg");
}

LRESULT CALLBACK MainProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	HWND temp;

	switch(message)
	{
		case WM_INITDIALOG:
			mainHWND = hwnd;
			InitializeDefaults();
			SendMessage(GetDlgItem(hwnd, IDC_EXENAME), WM_SETTEXT, 0, (LPARAM)(LPCSTR)launchersettings.EXEName);
			SendMessage(GetDlgItem(hwnd, IDC_CONFIGFILE), WM_SETTEXT, 0, (LPARAM)(LPCSTR)launchersettings.ConfigFile);
			break;
	
	    case WM_CREATE:
	    {
	            
	        break;
	    }

		case WM_DESTROY:
		{
			PostQuitMessage(0);
			break;
		}
			
		case WM_COMMAND:
		{
			switch(LOWORD(wParam))
			{
				case 2:
					PostMessage(hwnd, WM_DESTROY, 0, 0);
					break;
				case IDC_ABOUT: // The About button.
					sprintf(TempString, "%s %s by %s - %s", APPTITLE, APPVERSION, APPAUTHOR, APPCOMPANY);
					MessageBox(mainHWND, TempString, "About", MB_OK|MB_APPLMODAL);
					break;
				case IDC_FINDEXENAME:
					ChooseEXEName();
					break;
				case IDC_FINDCONFIGNAME:
					ChooseConfigName();
					break;
				case IDC_ADDFILE:
					AddExternalFile();
					break;
				case IDC_REMOVEFILE:
					temp = GetDlgItem(mainHWND, IDC_EXTFILECOMBO);
					SendMessage(temp, CB_DELETESTRING, SendMessage(temp, CB_GETCURSEL, 0, 0), 0);
					break;
				case IDC_HOSTGAME:
					DialogBox(g_hInst, (LPCSTR)IDD_HOSTGAME, mainHWND, (DLGPROC)HostProc);
					break;
				case IDC_JOINGAME:
					DialogBox(g_hInst, (LPCSTR)IDD_JOINGAME, mainHWND, (DLGPROC)JoinProc);
					break;
				case IDC_GO:
					CompileArguments();
					RunSRB2();
					break;
			    default:
					break;
			}

			break;
		}

		case WM_PAINT:
		{
			break;
		}
	}

	return 0;
}

