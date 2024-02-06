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

#include <QtDebug>

#include "singleapplication.h"

/**
 * @brief SingleApplication::SingleApplication
 *  Constructor. Checks and fires up LocalServer or closes the program
 *  if another instance already exists
 * @param argc
 * @param argv
 */
SingleApplication::SingleApplication(QStringList &args) :
    m_sema("PGE_MusPlaySemaphore_pehq395mh03hu320vu3n0u", 1),
    m_shmem("PGE_MusPlaySharedMemory_vy24h$@62j6@^jWyh3c6@46j@$^v24J42j6")
{
    m_shouldContinue = false; // By default this is not the main process

    m_server = nullptr;
    QString isServerRuns;

    //! is another copy of MusPlay running?
    bool isRunning=false;
    //! Don't create semaphore listening because another copy of Editor runs
    bool forceRun = false;
    m_sema.acquire();//Avoid races

    if(!m_shmem.create(1))//Detect shared memory copy
    {
        m_shmem.attach();
        m_shmem.detach();
        if(!m_shmem.create(1))
        {
            isRunning = true;
            if(!m_shmem.attach())
                qWarning() << "Can't re-attach existing shared memory!";
        }
    }

    //Force run second copy of application
    if(args.contains("--force-run", Qt::CaseInsensitive))
    {
        isServerRuns.clear();
        isRunning = false;
        forceRun = true;
        args.removeAll("--force-run");
    }

    if(isRunning)
    {
        QUdpSocket acceptor;
        acceptor.bind(QHostAddress::LocalHost, 58235, QUdpSocket::ReuseAddressHint|QUdpSocket::ShareAddress);

        // Attempt to connect to the LocalServer
        m_socket.connectToHost(QHostAddress::LocalHost, 58234);
        if(m_socket.waitForConnected(100))
        {
            m_socket.write(QString("CMD:Is SDL2 Mixer X running?").toUtf8());
            m_socket.flush();
            if(acceptor.waitForReadyRead(100))
            {
                //QByteArray dataGram;//Yes, I'm runs!
                QByteArray datagram;
                datagram.resize(static_cast<int>(acceptor.pendingDatagramSize()));
                QHostAddress sender;
                quint16 senderPort;
                acceptor.readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);
                if(QString::fromUtf8(datagram) == "Yes, I'm runs!")
                {
                    isServerRuns = "Yes!";
                    qDebug() <<"Found running player!";
                }
                else
                    qDebug() << "I'v got: "<<QString::fromUtf8(datagram);
            }
        }
    }

    if(args.contains("--force-run", Qt::CaseInsensitive))
    {
        isServerRuns.clear();
        args.removeAll("--force-run");
    }

    m_arguments = args;

    if(isRunning)
    {
        QString str = QString("CMD:showUp");
        QByteArray bytes;
        for(int i = 1; i < m_arguments.size(); i++)
            str.append(QString("\n%1").arg(m_arguments[i]));
        bytes = str.toUtf8();
        m_socket.write(bytes);
        m_socket.flush();
        QThread::msleep(100);
        m_socket.close();
    }
    else
    {
        // The attempt was insuccessful, so we continue the program
        m_shouldContinue = true;
        if(!forceRun)
        {
            m_server = new LocalServer();
            m_server->start();
            QObject::connect(m_server, SIGNAL(showUp()), this, SLOT(slotShowUp()));
            QObject::connect(m_server, SIGNAL(dataReceived(QString)), this, SLOT(slotOpenFile(QString)));
            QObject::connect(m_server, SIGNAL(acceptedCommand(QString)), this, SLOT(slotAcceptedCommand(QString)));
            QObject::connect(this, SIGNAL(stopServer()), m_server, SLOT(stopServer()));
        }
    }

    m_sema.release();//Free semaphore
}

/**
 * @brief SingleApplication::~SingleApplication
 *  Destructor
 */
SingleApplication::~SingleApplication()
{
    if(m_shouldContinue)
    {
        emit stopServer();
        if(m_server && (!m_server->wait(5000)))
        {
            qDebug() << "TERMINATOR RETURNS BACK single application! 8-)";
            m_server->terminate();
            qDebug() << "Wait for nothing";
            m_server->wait();
            qDebug() << "Terminated!";
        }
    }

    if(m_server)
        delete m_server;
}

/**
 * @brief SingleApplication::shouldContinue
 *  Weather the program should be terminated
 * @return bool
 */
bool SingleApplication::shouldContinue()
{
    return m_shouldContinue;
}

QStringList SingleApplication::arguments()
{
    return m_arguments;
}

/**
 * @brief SingleApplication::slotShowUp
 *  Executed when the showUp command is sent to LocalServer
 */
void SingleApplication::slotShowUp()
{
    emit showUp();
}

void SingleApplication::slotOpenFile(QString path)
{
    emit openFile(path);
}

void SingleApplication::slotAcceptedCommand(QString cmd)
{
    emit acceptedCommand(cmd);
}
