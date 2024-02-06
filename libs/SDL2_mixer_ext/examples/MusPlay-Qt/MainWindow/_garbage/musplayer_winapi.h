#ifndef MUSPLAYER_WINAPI_H
#define MUSPLAYER_WINAPI_H

#ifdef MUSPLAY_USE_WINAPI
#include "musplayer_base.h"
#include "defines.h"

class MusPlayer_WinAPI: public MusPlayerBase
{
public:
    enum Commands
    {
        CMD_Open=30000,
        CMD_Play,
        CMD_Stop,
        CMD_Volume,
        CMD_TrackID,
        CMD_TrackIDspin,
        CMD_MidiDevice,
        CMD_RecordWave,
        CMD_Bank,
        CMD_Tremolo,
        CMD_Vibrato,
        CMD_AdLibDrums,
        CMD_ScalableMod,
        CMD_SetDefault,
        GRP_GME,
        GRP_MIDI,
        GRP_ADLMIDI,
        CMD_Reverb,
        CMD_AssocFiles,
        CMD_Version,
        CMD_ShowLicense,
        CMD_ShowSource
    };
    static   MusPlayer_WinAPI* m_self;
    static   LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static   LRESULT CALLBACK SubCtrlProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    explicit MusPlayer_WinAPI(HINSTANCE hInstance = 0, int nCmdShow = 0);
    virtual ~MusPlayer_WinAPI();

    LONG        m_height;

    WNDCLASSEXW m_wc;
    HINSTANCE   m_hInstance;

    LPCWSTR     m_MainWndClass;

    HWND        m_hWnd;

    HWND        m_buttonOpen;
    HWND        m_buttonPlay;
    HWND        m_buttonStop;

    HWND        m_volume;
    HWND        m_formatInfo;

    HWND        m_recordWave;

    HWND        m_labelTitle;
    HWND        m_labelArtist;
    HWND        m_labelAlboom;
    HWND        m_labelCopyright;

    HWND        m_groupGME;
    struct GroupGME
    {
        HWND    m_labelTrack;
        HWND    m_trackNum;
        HWND    m_trackNumUpDown;
    } m_gme;

    HWND        m_groupMIDI;
    struct GroupMIDI
    {
        HWND    m_labelDevice;
        HWND    m_midiDevice;
    } m_midi;

    HWND        m_groupADLMIDI;
    struct GroupADLMIDI
    {
        HWND    m_labelBank;
        HWND    m_bankID;
        HWND    m_tremolo;
        HWND    m_vibrato;
        HWND    m_adlibDrums;
        HWND    m_scalableMod;
        HWND    m_resetToDefault;
    } m_adlmidi;

    HMENU m_contextMenu;

    void        initUI(HWND hWnd);
    void        exec();

    virtual void openMusicByArg(QString musPath);
    virtual void on_open_clicked();
    virtual void on_stop_clicked();
    virtual void on_play_clicked();
    virtual void on_mididevice_currentIndexChanged(int index);
    virtual void on_trackID_editingFinished();
    virtual void on_recordWav_clicked(bool checked);
    virtual void _blink_red();
    virtual void on_resetDefaultADLMIDI_clicked();
};

#endif

#endif // MUSPLAYER_WINAPI_H
