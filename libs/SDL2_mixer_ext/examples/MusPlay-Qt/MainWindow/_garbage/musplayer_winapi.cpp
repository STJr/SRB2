#ifdef MUSPLAY_USE_WINAPI
#include <assert.h>
#include <windows.h>
#include <commctrl.h>
#include <math.h>

#include "../Effects/reverb.h"
#include "../Player/mus_player.h"
#include "musplayer_winapi.h"
#include "../version.h"

MusPlayer_WinAPI* MusPlayer_WinAPI::m_self = nullptr;

LRESULT MusPlayer_WinAPI::MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_HSCROLL:
        {
            if(m_self->m_volume == (HWND)lParam)
            {
                switch(LOWORD(wParam))
                {
                case TB_ENDTRACK:
                case TB_THUMBPOSITION:
                case TB_THUMBTRACK:
                case SB_LEFT:
                case SB_RIGHT:
                    DWORD dwPos;// current position of slider
                    dwPos = SendMessageW(m_self->m_volume, TBM_GETPOS, 0, 0);
                    SendMessageW(m_self->m_volume, TBM_SETPOS,
                                (WPARAM)TRUE,               //redraw flag
                                (LPARAM)dwPos);
                    m_self->on_volume_valueChanged(dwPos);
                    break;
                default:
                    break;
                }
            }
            break;
        }
        case WM_COMMAND:
        {
            switch(HIWORD(wParam))
            {
                case BN_CLICKED:
                {
                    switch( LOWORD(wParam) )
                    {
                    case CMD_Open:
                        m_self->on_open_clicked();
                        break;
                    case CMD_Play:
                        m_self->on_play_clicked();
                        break;
                    case CMD_Stop:
                        m_self->on_stop_clicked();
                        break;
                    case CMD_SetDefault:
                        m_self->on_resetDefaultADLMIDI_clicked();
                        break;
                    case CMD_RecordWave:
                    {
                        BOOL checked = IsDlgButtonChecked(hWnd, CMD_RecordWave);
                        if (checked)
                        {
                            CheckDlgButton(hWnd, CMD_RecordWave, BST_UNCHECKED);
                            m_self->on_recordWav_clicked(false);
                        } else {
                            CheckDlgButton(hWnd, CMD_RecordWave, BST_CHECKED);
                            m_self->on_recordWav_clicked(true);
                        }
                        break;
                    }
                    case CMD_Reverb:
                    {
                        if (PGE_MusicPlayer::reverbEnabled)
                        {
                            CheckMenuItem(m_self->m_contextMenu, CMD_Reverb, MF_UNCHECKED);
                            Mix_UnregisterEffect(MIX_CHANNEL_POST, reverbEffect);
                            PGE_MusicPlayer::reverbEnabled = false;
                        } else {
                            CheckMenuItem(m_self->m_contextMenu, CMD_Reverb, MF_CHECKED);
                            Mix_RegisterEffect(MIX_CHANNEL_POST, reverbEffect, reverbEffectDone, NULL);
                            PGE_MusicPlayer::reverbEnabled = true;
                        }
                        break;
                    }
                    case CMD_ShowLicense:
                    {
                        ShellExecuteW(0, 0, L"http://www.gnu.org/licenses/gpl", 0, 0 , SW_SHOW);
                        break;
                    }
                    case CMD_ShowSource:
                    {
                        ShellExecuteW(0, 0, L"https://github.com/WohlSoft/PGE-Project", 0, 0 , SW_SHOW);
                        break;
                    }
                    default:
                        break;
                    }
                    break;
                }
            }
            break;
        }
        case WM_DROPFILES:
        {
            HDROP hDropInfo = (HDROP)wParam;
            wchar_t sItem[MAX_PATH];
            memset(sItem, 0, MAX_PATH*sizeof(wchar_t));
            if(DragQueryFileW(hDropInfo, 0, (LPWSTR)sItem, sizeof(sItem)))
            {
                m_self->openMusicByArg(Wstr2Str(sItem));
            }
            DragFinish(hDropInfo);
            break;
        }
        case WM_CONTEXTMENU:
        {
            SetForegroundWindow(hWnd);
            TrackPopupMenu(m_self->m_contextMenu,TPM_RIGHTBUTTON|TPM_TOPALIGN|TPM_LEFTALIGN,
                           LOWORD( lParam ),
                           HIWORD( lParam ), 0, hWnd, NULL);
            break;
        }
        //Инфо о минимальном и максимальном размере окна
        case WM_GETMINMAXINFO:
        {
            MINMAXINFO *minMax = (MINMAXINFO*)lParam;
            minMax->ptMinTrackSize.x = 350;
            minMax->ptMinTrackSize.y = m_self->m_height;
            break;
        }
        case WM_CREATE:
        {
            m_self->initUI(hWnd);
            break;
        }
        //Окно было закрыто
        case WM_DESTROY:
        {
            PostQuitMessage(0);
            return 0;
        }
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

LRESULT MusPlayer_WinAPI::SubCtrlProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static WNDPROC OrigBtnProc = reinterpret_cast<WNDPROC>(static_cast<LONG_PTR>(GetWindowLongPtr(hWnd, GWLP_USERDATA)));
    switch (msg)
    {
        case WM_COMMAND:
        {
            switch( LOWORD(wParam) )
            {
            case CMD_TrackID:
            {
                switch(HIWORD(wParam))
                {
                case  EN_UPDATE:
                    {
                        char trackIDtext[1024];
                        memset(trackIDtext, 0, 1024);
                        GetWindowTextA(m_self->m_gme.m_trackNum, trackIDtext, 1024);
                        //int trackID = atoi(trackIDtext);
                        SetWindowTextA(m_self->m_gme.m_labelTrack, trackIDtext);
                        m_self->on_trackID_editingFinished();
                        goto defaultWndProc;
                    }
                }
                goto defaultWndProc;
            }
            case CMD_MidiDevice:
            {
                switch(HIWORD(wParam))
                {
                    case CBN_SELCHANGE:
                    {
                        LRESULT index = SendMessageW(m_self->m_midi.m_midiDevice, CB_GETCURSEL, 0, 0);
                        if(index != CB_ERR)
                            m_self->on_mididevice_currentIndexChanged(index);
                        goto defaultWndProc;
                    }
                    break;
                    default:
                    break;
                }
                goto defaultWndProc;
            }
            case CMD_Bank:
            {
                switch(HIWORD(wParam))
                {
                    case CBN_SELCHANGE:
                    {
                        LRESULT index = SendMessageW(m_self->m_adlmidi.m_bankID, CB_GETCURSEL, 0, 0);
                        if(index != CB_ERR)
                            m_self->on_fmbank_currentIndexChanged(index);
                        goto defaultWndProc;
                    }
                    break;
                    default:
                    break;
                }
                goto defaultWndProc;
            }
            case CMD_Tremolo:
            {
                BOOL checked = IsDlgButtonChecked(hWnd, CMD_Tremolo);
                if (checked)
                {
                    CheckDlgButton(hWnd, CMD_Tremolo, BST_UNCHECKED);
                    m_self->on_tremolo_clicked(false);
                } else {
                    CheckDlgButton(hWnd, CMD_Tremolo, BST_CHECKED);
                    m_self->on_tremolo_clicked(true);
                }
                goto defaultWndProc;
            }
            case CMD_Vibrato:
            {
                BOOL checked = IsDlgButtonChecked(hWnd, CMD_Vibrato);
                if (checked)
                {
                    CheckDlgButton(hWnd, CMD_Vibrato, BST_UNCHECKED);
                    m_self->on_vibrato_clicked(false);
                } else {
                    CheckDlgButton(hWnd, CMD_Vibrato, BST_CHECKED);
                    m_self->on_vibrato_clicked(true);
                }
                goto defaultWndProc;
            }
            case CMD_ScalableMod:
            {
                BOOL checked = IsDlgButtonChecked(hWnd, CMD_ScalableMod);
                if (checked)
                {
                    CheckDlgButton(hWnd, CMD_ScalableMod, BST_UNCHECKED);
                    m_self->on_modulation_clicked(false);
                } else {
                    CheckDlgButton(hWnd, CMD_ScalableMod, BST_CHECKED);
                    m_self->on_modulation_clicked(true);
                }
                goto defaultWndProc;
            }
            case CMD_AdLibDrums:
            {
                BOOL checked = IsDlgButtonChecked(hWnd, CMD_AdLibDrums);
                if (checked)
                {
                    CheckDlgButton(hWnd, CMD_AdLibDrums, BST_UNCHECKED);
                    m_self->on_adlibMode_clicked(false);
                } else {
                    CheckDlgButton(hWnd, CMD_AdLibDrums, BST_CHECKED);
                    m_self->on_adlibMode_clicked(true);
                }
                goto defaultWndProc;
            }
            default:
                break;
            }
        }
    }

defaultWndProc:
    return CallWindowProc(OrigBtnProc, hWnd, msg, wParam, lParam);
}


MusPlayer_WinAPI::MusPlayer_WinAPI(HINSTANCE hInstance, int nCmdShow) : MusPlayerBase()
{
    assert(!MusPlayer_WinAPI::m_self && "Multiple instances of MainWindow are Forbidden! It's singleton!");
    MusPlayer_WinAPI::m_self = this;
    m_MainWndClass = L"PGEMusPlayer";
    m_hInstance = hInstance;

    INITCOMMONCONTROLSEX icc;
    // Initialise common controls.
    icc.dwSize  = sizeof(icc);
    icc.dwICC   = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&icc);

    // Class for our main window.
    memset(&m_wc, 0, sizeof(m_wc));
    m_wc.cbSize        = sizeof(m_wc);
    m_wc.style         = 0;
    m_wc.lpfnWndProc   = &MusPlayer_WinAPI::MainWndProc;
    m_wc.cbClsExtra    = 0;
    m_wc.cbWndExtra    = 0;
    m_wc.hInstance     = hInstance;
    m_wc.hIcon         = (HICON) LoadImage(hInstance, MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, 0, 0,
                                         LR_DEFAULTSIZE | LR_DEFAULTCOLOR | LR_SHARED);
    m_wc.hCursor       = (HCURSOR) LoadImage(NULL, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_SHARED);
    m_wc.hbrBackground = (HBRUSH) (COLOR_BTNFACE+1);
    m_wc.lpszMenuName  = NULL;
    m_wc.lpszClassName = m_MainWndClass;
    m_wc.hIconSm       = (HICON) LoadImage(hInstance, MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON,
                                            GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON),
                                            LR_DEFAULTCOLOR | LR_SHARED);

    // Register our window classes, or error.
    if(!RegisterClassExW(&m_wc))
    {
        MessageBoxA(NULL, "Error registering window class.", "Error", MB_ICONERROR | MB_OK);
        return;
    }


    m_height = 170;
    // Create instance of main window.
    m_hWnd = CreateWindowExW(WS_EX_ACCEPTFILES,
                             m_MainWndClass,
                             m_MainWndClass,
                             WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,
                             CW_USEDEFAULT,
                             CW_USEDEFAULT,
                             320, m_height, NULL, NULL, hInstance, NULL);

    // Show window and force a paint.
    ShowWindow(m_hWnd, nCmdShow);
    UpdateWindow(m_hWnd);
}

MusPlayer_WinAPI::~MusPlayer_WinAPI()
{
    on_stop_clicked();
    Mix_CloseAudio();
    MusPlayer_WinAPI::m_self = nullptr;
}

void MusPlayer_WinAPI::initUI(HWND hWnd)
{
    HGDIOBJ hFont   = GetStockObject(DEFAULT_GUI_FONT);
    HFONT hFontBold = CreateFontW(-11, 0, 0, 0, FW_BOLD, FALSE, FALSE,
                                0, ANSI_CHARSET, OUT_DEFAULT_PRECIS,
                               CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                               DEFAULT_PITCH | FF_SWISS, L"Tahoma");
    int left = 5;
    int top = 10;

    HWND hText = CreateWindowExW(0, L"STATIC", L"Press \"Open\" or drag&&drop music file into this window",
                                WS_CHILD | WS_VISIBLE | SS_LEFT,
                                left, top, 400, 15,
                                hWnd, NULL, m_hInstance, NULL);
    top += 25;

    m_labelTitle = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_LEFT,
                                left, top, 400, 15,
                                hWnd, NULL, m_hInstance, NULL);
    top += 15;
    m_labelArtist = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_LEFT,
                                left, top, 400, 15,
                                hWnd, NULL, m_hInstance, NULL);
    top += 15;
    m_labelAlboom = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_LEFT,
                                left, top, 400, 15,
                                hWnd, NULL, m_hInstance, NULL);
    top += 15;
    m_labelCopyright = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_LEFT,
                                left, top, 400, 15,
                                hWnd, NULL, m_hInstance, NULL);
    top += 15;


    m_buttonOpen = CreateWindowExW(0, L"BUTTON", L"Open", WS_TABSTOP|WS_VISIBLE|WS_CHILD,
                                left, top, 60, 21, hWnd,
                                (HMENU)CMD_Open, // Here is the ID of your button ( You may use your own ID )
                                m_hInstance, NULL);
    left += 60;

    m_buttonPlay = CreateWindowExW(0, L"BUTTON", L"Play/Pause", WS_TABSTOP|WS_VISIBLE|WS_CHILD,
                                left, top, 60, 21, hWnd,
                                (HMENU)CMD_Play, // Here is the ID of your button ( You may use your own ID )
                                m_hInstance, NULL);
    left += 60;

    m_buttonStop = CreateWindowExW(0, L"BUTTON", L"Stop", WS_TABSTOP|WS_VISIBLE|WS_CHILD,
                                left, top, 60, 21, hWnd,
                                (HMENU)CMD_Stop, // Here is the ID of your button ( You may use your own ID )
                                m_hInstance, NULL);

    left += 60;
    m_volume    = CreateWindowExW(0, TRACKBAR_CLASS, L"Volume", WS_TABSTOP|WS_VISIBLE|WS_CHILD,
                                  left, top-2, 80, 25, hWnd,
                                  (HMENU)CMD_Volume, // Here is the ID of your button ( You may use your own ID )
                                  m_hInstance, NULL);
    SendMessageW(m_volume, TBM_SETRANGE,
                (WPARAM)TRUE,               //redraw flag
                (LPARAM)MAKELONG(0, 128));  //min. & max. positions
    SendMessageW(m_volume, TBM_SETPOS,
                (WPARAM)TRUE,               //redraw flag
                (LPARAM)128);
    SendMessageW(m_volume, WM_SETFONT, (WPARAM)hFont, 0);
    left += 80;

    m_formatInfo = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_LEFT,
                                   left, top, 200, 15,
                                   hWnd, NULL, m_hInstance, NULL);
    SendMessageW(m_formatInfo, WM_SETFONT, (WPARAM)hFont, 0);

    left = 5;
    top += 21;

    m_recordWave = CreateWindowExW(0, L"BUTTON", L"Record wave", WS_TABSTOP|WS_VISIBLE|WS_CHILD|SS_LEFT|BS_CHECKBOX,
                                                             left, top, 200, 15,
                                                             hWnd, (HMENU)CMD_RecordWave, m_hInstance, NULL);
    SendMessageW(m_recordWave, WM_SETFONT, (WPARAM)hFont, 0);
    CheckDlgButton(m_groupADLMIDI, CMD_RecordWave, BST_UNCHECKED);

    top += 18;

    WNDPROC OldBtnProc;

    m_groupGME = CreateWindowExW(0, L"BUTTON", L"Game Music Emulators setup", WS_VISIBLE | WS_CHILD | BS_GROUPBOX,
                                left, top, 330, 50, hWnd, (HMENU)GRP_GME, m_hInstance, NULL);
    SendMessageW(m_groupGME, WM_SETFONT, (WPARAM)hFont, 0);
    OldBtnProc = reinterpret_cast<WNDPROC>(static_cast<LONG_PTR>(
                 SetWindowLongPtr(m_groupGME, GWLP_WNDPROC,
                 reinterpret_cast<LONG_PTR>(MusPlayer_WinAPI::SubCtrlProc))) );
    SetWindowLongPtr(m_groupGME, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(OldBtnProc));

    m_gme.m_labelTrack = CreateWindowExW(0, L"STATIC", L"Track number:", WS_CHILD | WS_VISIBLE | SS_LEFT,
                                10, 20, 70, 15,
                                 m_groupGME, NULL, m_hInstance, NULL);
    SendMessageW(m_gme.m_labelTrack, WM_SETFONT, (WPARAM)hFont, 0);

    m_gme.m_trackNum = CreateWindowExW(0, L"EDIT", L"ED_RED",
                                    WS_TABSTOP|WS_CHILD|WS_VISIBLE|ES_LEFT|WS_BORDER,
                                    80, 20, 240, 20,
                                    m_groupGME, (HMENU)(CMD_TrackID),
                                    m_hInstance, NULL);

    SendMessageW(m_gme.m_trackNum, WM_SETFONT, (WPARAM)hFont, 0);
    // with a spin control to its right side
    m_gme.m_trackNumUpDown = CreateWindowExW(0, UPDOWN_CLASS, L"SP_RED",
                                            WS_TABSTOP|WS_CHILD | WS_VISIBLE
                                            | UDS_ARROWKEYS | UDS_ALIGNRIGHT
                                            | UDS_SETBUDDYINT | WS_BORDER,
                                            80, 20, 240, 20,
                                            m_groupGME, (HMENU)(CMD_TrackIDspin), m_hInstance, NULL);
    SendMessageW(m_gme.m_trackNumUpDown, WM_SETFONT, (WPARAM)hFont, 0);

    // Set the buddy control.
    SendMessage(m_gme.m_trackNumUpDown, UDM_SETBUDDY, (LONG)m_gme.m_trackNum, 0L);
    // Set the range.
    SendMessage(m_gme.m_trackNumUpDown, UDM_SETRANGE, 0L, MAKELONG(32000, 0) );
    // Set the initial value
    SendMessage(m_gme.m_trackNumUpDown, UDM_SETPOS, 0L, MAKELONG((int)(0), 0));

    //SendMessage(m_gme.m_trackNumUpDown, UDS_WRAP, 0L, FALSE);




    m_groupMIDI = CreateWindowExW(0, L"BUTTON", L"MIDI Setup", WS_VISIBLE|WS_CHILD|BS_GROUPBOX|WS_GROUP,
                                left, top, 330, 50,
                                hWnd, (HMENU)GRP_MIDI, m_hInstance, NULL);
    SendMessageW(m_groupMIDI, WM_SETFONT, (WPARAM)hFont, 0);

    OldBtnProc = reinterpret_cast<WNDPROC>(static_cast<LONG_PTR>(
                 SetWindowLongPtr(m_groupMIDI, GWLP_WNDPROC,
                 reinterpret_cast<LONG_PTR>(MusPlayer_WinAPI::SubCtrlProc))) );
    SetWindowLongPtr(m_groupMIDI, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(OldBtnProc));

    top += 50;

    m_midi.m_labelDevice = CreateWindowExW(0, L"STATIC", L"MIDI Device:", WS_CHILD | WS_VISIBLE | SS_LEFT,
                                10, 20, 70, 15,
                                m_groupMIDI, NULL, m_hInstance, NULL);
    SendMessageW(m_midi.m_labelDevice, WM_SETFONT, (WPARAM)hFont, 0);

    const wchar_t* midiDevices[] = {
        L"ADL Midi (OPL Synth emulation)",
        L"Timidity (GUS-like wavetable MIDI Synth)",
        L"Native midi (Built-in MIDI of your OS)"
    };
    m_midi.m_midiDevice = CreateWindowExW(0, L"COMBOBOX", L"MidiDevice", WS_TABSTOP|WS_VISIBLE|WS_CHILD|CBS_DROPDOWNLIST|WS_TABSTOP,
                                              80, 17, 240, 210, m_groupMIDI, (HMENU)CMD_MidiDevice, m_hInstance, NULL);
    SendMessageW(m_midi.m_midiDevice, WM_SETFONT, (WPARAM)hFont, 0);
    SendMessageW(m_midi.m_midiDevice, CB_ADDSTRING, 0, (LPARAM)midiDevices[0]);
    SendMessageW(m_midi.m_midiDevice, CB_ADDSTRING, 0, (LPARAM)midiDevices[1]);
    SendMessageW(m_midi.m_midiDevice, CB_ADDSTRING, 0, (LPARAM)midiDevices[2]);
    SendMessageW(m_midi.m_midiDevice, CB_SETCURSEL, 0, 0);

    m_groupADLMIDI = CreateWindowExW(0, L"BUTTON", L"ADLMIDI Extra Setup", WS_VISIBLE|WS_CHILD|BS_GROUPBOX|WS_GROUP,
                                left, top, 330, 125,
                                hWnd, (HMENU)GRP_ADLMIDI, m_hInstance, NULL);
    SendMessageW(m_groupADLMIDI, WM_SETFONT, (WPARAM)hFont, 0);
    //top += 50;

    OldBtnProc=reinterpret_cast<WNDPROC>(static_cast<LONG_PTR>(
                 SetWindowLongPtr(m_groupADLMIDI, GWLP_WNDPROC,
                 reinterpret_cast<LONG_PTR>(MusPlayer_WinAPI::SubCtrlProc))) );
    SetWindowLongPtr(m_groupADLMIDI, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(OldBtnProc));


    m_adlmidi.m_labelBank = CreateWindowExW(0, L"STATIC", L"ADL Midi bank ID:", WS_CHILD|WS_VISIBLE | SS_LEFT,
                                10, 20, 70, 15,
                                m_groupADLMIDI, NULL, m_hInstance, NULL);
    SendMessageW(m_adlmidi.m_labelBank, WM_SETFONT, (WPARAM)hFont, 0);

    m_adlmidi.m_bankID = CreateWindowExW(0, L"COMBOBOX", L"BankId", WS_TABSTOP|WS_VISIBLE|WS_CHILD|CBS_DROPDOWNLIST|CBS_DISABLENOSCROLL|WS_VSCROLL|CBS_NOINTEGRALHEIGHT|WS_TABSTOP,
                                              80, 17, 240, 210, m_groupADLMIDI, (HMENU)CMD_Bank, m_hInstance, NULL);
    SendMessageW(m_adlmidi.m_bankID, WM_SETFONT, (WPARAM)hFont, 0);
    int insCount            = Mix_ADLMIDI_getTotalBanks();
    const char*const* names = Mix_ADLMIDI_getBankNames();
    for(int i=0; i<insCount; i++)
    {
        SendMessageA(m_adlmidi.m_bankID, CB_ADDSTRING, 0, (LPARAM)names[i]);
    }
    SendMessageW(m_adlmidi.m_bankID, CB_SETCURSEL, 58, 0);


    m_adlmidi.m_tremolo = CreateWindowExW(0, L"BUTTON", L"Deep tremolo", WS_TABSTOP|WS_VISIBLE|WS_CHILD|SS_LEFT|BS_CHECKBOX,
                                          10, 40, 200, 15,
                                          m_groupADLMIDI, (HMENU)CMD_Tremolo, m_hInstance, NULL);
    SendMessageW(m_adlmidi.m_tremolo, WM_SETFONT, (WPARAM)hFont, 0);
    CheckDlgButton(m_groupADLMIDI, CMD_Tremolo, BST_CHECKED);

    m_adlmidi.m_vibrato = CreateWindowExW(0, L"BUTTON", L"Deep vibrato", WS_TABSTOP|WS_VISIBLE|WS_CHILD|SS_LEFT|BS_CHECKBOX,
                                          10, 60, 200, 15,
                                          m_groupADLMIDI, (HMENU)CMD_Vibrato, m_hInstance, NULL);
    SendMessageW(m_adlmidi.m_vibrato, WM_SETFONT, (WPARAM)hFont, 0);
    CheckDlgButton(m_groupADLMIDI, CMD_Vibrato, BST_CHECKED);

    m_adlmidi.m_scalableMod = CreateWindowExW(0, L"BUTTON", L"Scalable modulation", WS_TABSTOP|WS_VISIBLE|WS_CHILD|SS_LEFT|BS_CHECKBOX,
                                          10, 80, 200, 15,
                                          m_groupADLMIDI, (HMENU)CMD_ScalableMod, m_hInstance, NULL);
    SendMessageW(m_adlmidi.m_scalableMod, WM_SETFONT, (WPARAM)hFont, 0);
    CheckDlgButton(m_groupADLMIDI, CMD_ScalableMod, BST_UNCHECKED);

    m_adlmidi.m_adlibDrums = CreateWindowExW(0, L"BUTTON", L"AdLib drums mode", WS_TABSTOP|WS_VISIBLE|WS_CHILD|SS_LEFT|BS_CHECKBOX,
                                          10, 100, 200, 15,
                                          m_groupADLMIDI, (HMENU)CMD_AdLibDrums, m_hInstance, NULL);
    SendMessageW(m_adlmidi.m_adlibDrums, WM_SETFONT, (WPARAM)hFont, 0);
    CheckDlgButton(m_groupADLMIDI, CMD_AdLibDrums, BST_UNCHECKED);


    ShowWindow(m_groupGME, SW_HIDE);
    ShowWindow(m_groupMIDI, SW_HIDE);
    ShowWindow(m_groupADLMIDI, SW_HIDE);


    SendMessageW(hWnd, WM_SETFONT, (WPARAM)hFont, 0);

    SendMessageW(m_buttonOpen, WM_SETFONT, (WPARAM)hFont, 0);
    SendMessageW(m_buttonPlay, WM_SETFONT, (WPARAM)hFont, 0);
    SendMessageW(m_buttonStop, WM_SETFONT, (WPARAM)hFont, 0);

    SendMessageW(hText, WM_SETFONT, (WPARAM)hFontBold, 0);

    SendMessageW(m_labelTitle, WM_SETFONT, (WPARAM)hFont, 0);
    SendMessageW(m_labelArtist, WM_SETFONT, (WPARAM)hFont, 0);
    SendMessageW(m_labelAlboom, WM_SETFONT, (WPARAM)hFont, 0);
    SendMessageW(m_labelCopyright, WM_SETFONT, (WPARAM)hFont, 0);

    SetWindowTextW(hWnd, L"Simple SDL2 Mixer X Music player [WinAPI-based]");

    HMENU hSubMenu;
    m_contextMenu = CreatePopupMenu();
    AppendMenuW(m_contextMenu, MF_STRING, CMD_Open, L"Open");
    AppendMenuW(m_contextMenu, MF_STRING, CMD_Play, L"Play/Pause");
    AppendMenuW(m_contextMenu, MF_STRING, CMD_Stop, L"Stop");
    AppendMenuW(m_contextMenu, MF_SEPARATOR, 0, 0);
    AppendMenuW(m_contextMenu, MF_STRING, CMD_Reverb, L"Reverb");
    AppendMenuW(m_contextMenu, MF_STRING, CMD_AssocFiles, L"Associate files");
    EnableMenuItem(m_contextMenu, CMD_AssocFiles, MF_GRAYED);
    AppendMenuW(m_contextMenu, MF_SEPARATOR, 0, 0);

    hSubMenu = CreatePopupMenu();
    AppendMenuA(hSubMenu, MF_STRING, CMD_Version, "SDL Mixer X Music Player [WinAPI], v." V_FILE_VERSION);
    EnableMenuItem(hSubMenu, CMD_Version, MF_GRAYED);
    AppendMenuW(hSubMenu, MF_STRING, CMD_ShowLicense, L"Licensed under GNU GPLv3 license");
    AppendMenuW(hSubMenu, MF_SEPARATOR, 0, 0);
    AppendMenuW(hSubMenu, MF_STRING, CMD_ShowSource, L"Get source code");

    AppendMenuW(m_contextMenu,  MF_STRING | MF_POPUP, (UINT)hSubMenu, L"About");

}

void MusPlayer_WinAPI::exec()
{
    MSG msg;

    // Main message loop.
    while(GetMessage(&msg, NULL, 0, 0) > 0)
    {
        if (!TranslateAccelerator(msg.hwnd, NULL, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
}

void MusPlayer_WinAPI::openMusicByArg(QString musPath)
{
    BOOL waveRecorderInProcess = IsDlgButtonChecked(m_hWnd, CMD_RecordWave);
    if(waveRecorderInProcess) return;
    currentMusic=musPath;
    Mix_HaltMusic();
    on_play_clicked();
}

void MusPlayer_WinAPI::on_open_clicked()
{
    OPENFILENAMEW open;
    memset(&open, 0, sizeof(open));

    wchar_t flnm[MAX_PATH];
    flnm[0]=L'\0';
    open.lStructSize = sizeof(OPENFILENAMEW);
    open.hInstance = m_hInstance;
    open.hwndOwner = m_hWnd;
    open.lpstrFilter = L"All files (*.*)\0*.*";
    open.lpstrDefExt = L"\0";
    open.lpstrFile = flnm;
    open.nMaxFile = MAX_PATH;
    open.lpstrTitle = L"Open music file to play...";
    open.Flags = OFN_FILEMUSTEXIST;
    if(GetOpenFileNameW(&open)==TRUE)
    {
        currentMusic = Wstr2Str(open.lpstrFile);
    } else {
        return;
    }

    Mix_HaltMusic();
    on_play_clicked();
}

void MusPlayer_WinAPI::on_stop_clicked()
{
    PGE_MusicPlayer::MUS_stopMusic();
    SetWindowTextW(m_buttonPlay, L"Play");
    BOOL checked = IsDlgButtonChecked(m_hWnd, CMD_RecordWave);
    if (checked)
    {
        CheckDlgButton(m_hWnd, CMD_RecordWave, BST_UNCHECKED);
        PGE_MusicPlayer::stopWavRecording();
        EnableWindow(m_buttonOpen, TRUE);
        EnableWindow(m_buttonPlay, TRUE);
        EnableWindow(m_gme.m_trackNum, TRUE);
        EnableWindow(m_gme.m_trackNumUpDown, TRUE);
        EnableWindow(m_midi.m_midiDevice, TRUE);
        EnableWindow(m_adlmidi.m_bankID, TRUE);
        EnableWindow(m_adlmidi.m_tremolo, TRUE);
        EnableWindow(m_adlmidi.m_vibrato, TRUE);
        EnableWindow(m_adlmidi.m_scalableMod, TRUE);
        EnableWindow(m_adlmidi.m_adlibDrums, TRUE);
    }
}

void MusPlayer_WinAPI::on_play_clicked()
{
    if(Mix_PlayingMusic())
    {
        if(Mix_PausedMusic())
        {
            Mix_ResumeMusic();
            SetWindowTextW(m_buttonPlay, L"Pause");
            return;
        }
        else
        {
            Mix_PauseMusic();
            SetWindowTextW(m_buttonPlay, L"Resume");
            return;
        }
    }

    char trackIDtext[1024];
    memset(trackIDtext, 0, 1024);
    GetWindowTextA(m_gme.m_trackNum, trackIDtext, 1024);
    int trackID = atoi(trackIDtext);
    m_prevTrackID = trackID;
    SetWindowTextW(m_buttonPlay, L"Play");
    if(currentMusic.empty())
        return;

    if(PGE_MusicPlayer::MUS_openFile(currentMusic +"|"+trackIDtext ) )
    {
        PGE_MusicPlayer::MUS_playMusic();
        SetWindowTextW(m_buttonPlay, L"Pause");
    }

    ShowWindow(m_groupGME, SW_HIDE);
    ShowWindow(m_groupMIDI, SW_HIDE);
    ShowWindow(m_groupADLMIDI, SW_HIDE);
    switch(PGE_MusicPlayer::type)
    {
        case MUS_MID:
            ShowWindow(m_groupMIDI,     SW_SHOW);
            ShowWindow(m_groupADLMIDI,  SW_SHOW);
            m_height = 350;
            break;
        case MUS_SPC:
            ShowWindow(m_groupGME,  SW_SHOW);
            m_height = 220;
            break;
        default:
            m_height = 170;
            break;
    }
    SetWindowPos(m_hWnd, HWND_TOP, 0, 0, 350, m_height, SWP_NOMOVE|SWP_NOZORDER);

    SetWindowTextW(m_labelTitle,        Str2Wstr(PGE_MusicPlayer::MUS_getMusTitle()).c_str());
    SetWindowTextW(m_labelArtist,       Str2Wstr(PGE_MusicPlayer::MUS_getMusArtist()).c_str());
    SetWindowTextW(m_labelAlboom,       Str2Wstr(PGE_MusicPlayer::MUS_getMusAlbum()).c_str());
    SetWindowTextW(m_labelCopyright,    Str2Wstr(PGE_MusicPlayer::MUS_getMusCopy()).c_str());
    SetWindowTextA(m_formatInfo,        PGE_MusicPlayer::musicTypeC());
}

void MusPlayer_WinAPI::on_mididevice_currentIndexChanged(int index)
{
    switch(index)
    {
        case 0: Mix_SetMidiDevice(MIDI_ADLMIDI);
        break;
        case 1: Mix_SetMidiDevice(MIDI_Timidity);
        break;
        case 2: Mix_SetMidiDevice(MIDI_Native);
        break;
        case 3: Mix_SetMidiDevice(MIDI_Fluidsynth);
        break;
        default: Mix_SetMidiDevice(MIDI_ADLMIDI);
        break;
    }
    if(Mix_PlayingMusic())
    {
        if(PGE_MusicPlayer::type==MUS_MID)
        {
            Mix_HaltMusic();
            on_play_clicked();
        }
    }
}

void MusPlayer_WinAPI::on_trackID_editingFinished()
{
    if(Mix_PlayingMusic())
    {
        char buff[1024];
        memset(buff, 0, 1024);
        GetWindowTextA(m_gme.m_trackNum, buff, 1024);
        int trackID = atoi(buff);
        if( (PGE_MusicPlayer::type == MUS_SPC) && (m_prevTrackID != trackID) )
        {
            Mix_HaltMusic();
            on_play_clicked();
        }
    }
}

void MusPlayer_WinAPI::on_recordWav_clicked(bool checked)
{
    if(checked)
    {
        PGE_MusicPlayer::MUS_stopMusic();
        SetWindowTextW(m_buttonPlay, L"Play");
        PGE_MusicPlayer::stopWavRecording();
        PGE_MusicPlayer::startWavRecording(currentMusic + ".wav");
        on_play_clicked();
        EnableWindow(m_buttonOpen, FALSE);
        EnableWindow(m_buttonPlay, FALSE);
        EnableWindow(m_gme.m_trackNum, FALSE);
        EnableWindow(m_gme.m_trackNumUpDown, FALSE);
        EnableWindow(m_midi.m_midiDevice, FALSE);
        EnableWindow(m_adlmidi.m_bankID, FALSE);
        EnableWindow(m_adlmidi.m_tremolo, FALSE);
        EnableWindow(m_adlmidi.m_vibrato, FALSE);
        EnableWindow(m_adlmidi.m_scalableMod, FALSE);
        EnableWindow(m_adlmidi.m_adlibDrums, FALSE);
    } else {
        on_stop_clicked();
        PGE_MusicPlayer::stopWavRecording();
        SetWindowTextW(m_buttonPlay, L"Play");
        EnableWindow(m_buttonOpen, TRUE);
        EnableWindow(m_buttonPlay, TRUE);
        EnableWindow(m_gme.m_trackNum, TRUE);
        EnableWindow(m_gme.m_trackNumUpDown, TRUE);
        EnableWindow(m_midi.m_midiDevice, TRUE);
        EnableWindow(m_adlmidi.m_bankID, TRUE);
        EnableWindow(m_adlmidi.m_tremolo, TRUE);
        EnableWindow(m_adlmidi.m_vibrato, TRUE);
        EnableWindow(m_adlmidi.m_scalableMod, TRUE);
        EnableWindow(m_adlmidi.m_adlibDrums, TRUE);
    }
}

void MusPlayer_WinAPI::_blink_red()
{
    m_blink_state=!m_blink_state;
    //TODO: Timer and background color toggler (blink label by red while WAV recording)
}

void MusPlayer_WinAPI::on_resetDefaultADLMIDI_clicked()
{
    //TODO!
}

#endif
