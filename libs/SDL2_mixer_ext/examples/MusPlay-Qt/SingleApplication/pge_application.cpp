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

#include <QFileOpenEvent>
#include <QDebug>
#include "pge_application.h"

PGE_OSXApplication::PGE_OSXApplication(int &argc, char **argv)
    : QApplication(argc, argv)
{
#ifdef Q_OS_MACX
    m_connected = false;
#endif
}

PGE_OSXApplication::~PGE_OSXApplication()
{}

#ifdef Q_OS_MACX
void PGE_OSXApplication::setConnected()
{
    m_connected = true;
}

bool PGE_OSXApplication::event(QEvent *event)
{
    if(event->type() == QEvent::FileOpen)
    {
        QFileOpenEvent *openEvent = static_cast<QFileOpenEvent *>(event);
        if(openEvent)
        {
            if(m_connected)
            {
                QString file = openEvent->file();
                qDebug()<< ("Opened file "+file + " (signal)");
                emit openFileRequested( file );
            }
            else
            {
                QString file = openEvent->file();
                qDebug()<< ("Opened file "+file + " (queue)");
                m_openFileRequests.enqueue(file);
            }
        } else {
            qWarning()<< ("Failed to process openEvent: pointer is null!");
        }
    }
    return QApplication::event(event);
}

QStringList PGE_OSXApplication::getOpenFileChain()
{
    QStringList chain;
    while(!m_openFileRequests.isEmpty())
    {
        QString file = m_openFileRequests.dequeue();
        chain.push_back(file);
    }
    return chain;
}
#endif //Q_OS_MACX
