#pragma once
#ifndef MUSPLAYERBASE_H
#define MUSPLAYERBASE_H

#include <QMainWindow>
#include <QUrl>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QTimer>

class MusPlayerBase
{
public:
    MusPlayerBase();
    virtual ~MusPlayerBase();

    /*!
     * \brief Open music file
     * \param musPath full or relative path to music file to open
     */
    virtual void openMusicByArg(QString musPath);

    /*!
     * \brief Open button click event
     */
    virtual void on_open_clicked();

    /*!
     * \brief Stop button click event
     */
    virtual void on_stop_clicked();

    /*!
     * \brief Play button click event
     */
    virtual void on_play_clicked();

    /*!
     * \brief Volume slider event
     * \param value Volume level
     */
    virtual void on_volume_valueChanged(int value);

    /*!
     * \brief Track-ID editing finished event
     */
    virtual void on_trackID_editingFinished();

    /*!
     * \brief Triggers when 'record wav' checkbox was toggled
     * \param checked
     */
    virtual void on_recordWav_clicked(bool checked);

    /*!
     * \brief Changes color of "Recording WAV" label between black and red
     */
    virtual void _blink_red();

    /*!
     * \brief Resets ADLMIDI properties into initial state
     */
    virtual void on_resetDefaultADLMIDI_clicked();

protected:
    //! Full path to currently opened music
    QString m_currentMusic;

    bool   m_blink_state;
    int    m_prevTrackID;
};

#endif // MUSPLAYERBASE_H
