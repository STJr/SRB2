/*
 * Platformer Game Engine by Wohlstand, a free platform for game making
 * Copyright (c) 2014-2023 Vitaly Novichkov <admin@wohlnet.ru>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once
#ifndef PGE_EDITORAPPLICATION_H
#define PGE_EDITORAPPLICATION_H

#include <QApplication>
#include <QQueue>
#include <QStringList>

/*
Note: Class wasn't disabled completely to avoid "Note: No relevant classes found. No output generated." warning.
*/

#if defined(DEFINE_Q_OS_MACX) && !defined(Q_OS_MACX)
#define Q_OS_MACX // Workaround for MOC
#endif

/**
 * @brief Inhereted application which provides open file event, usually needed for OS X
 */
class PGE_OSXApplication : public QApplication
{
    Q_OBJECT
#ifdef Q_OS_MACX
    //! Queue used before slot will be connected to collect file paths received via QFileOpenEvent
    QQueue<QString> m_openFileRequests;
    //! Mark means to don't collect file paths and send them via signals
    bool            m_connected;
#endif
public:
    PGE_OSXApplication(int &argc, char **argv);
    virtual ~PGE_OSXApplication();
#ifdef Q_OS_MACX
    /**
     * @brief Disable collecting of the file paths via queue and send any new-received paths via signal
     */
    void    setConnected();
    /**
     * @brief Input event
     * @param event Event descriptor
     * @return is event successfully processed
     */
    bool    event(QEvent *event);
    /**
     * @brief Get all collected file paths and clear internal queue
     * @return String list of all collected file paths
     */
    QStringList getOpenFileChain();
signals:
    /**
     * @brief Signal emiting on receiving a file path via QFileOpenEvent
     * @param filePath full path to open
     */
    void openFileRequested(QString filePath);
#endif
};

#ifdef Q_OS_MACX
//! Allows processing file open and some other events on OS X. On non-Mac operating systems works as stub.
typedef PGE_OSXApplication PGE_Application;
#else
//! Allows processing file open and some other events on OS X. On non-Mac operating systems works as stub.
typedef QApplication PGE_Application;
#endif //Q_OS_MACX

#endif // PGE_EDITORAPPLICATION_H
