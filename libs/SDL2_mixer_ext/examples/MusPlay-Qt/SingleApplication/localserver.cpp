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

#include <QFile>
#include <QStringList>
#include <QtDebug>

#include "localserver.h"


IntProcServer::IntProcServer()
{
    connect(this, SIGNAL(readyRead()), this, SLOT(doReadData()));
}

IntProcServer::~IntProcServer()
{}

void IntProcServer::stateChanged(QAbstractSocket::SocketState stat)
{
    switch(stat)
    {
    case QAbstractSocket::UnconnectedState: qDebug()<<"The socket is not connected.";break;
    case QAbstractSocket::HostLookupState: qDebug()<<"The socket is performing a host name lookup.";break;
    case QAbstractSocket::ConnectingState: qDebug()<<"The socket has started establishing a connection.";break;
    case QAbstractSocket::ConnectedState: qDebug()<<"A connection is established.";break;
    case QAbstractSocket::BoundState: qDebug()<<"The socket is bound to an address and port.";break;
    case QAbstractSocket::ClosingState: qDebug()<<"The socket is about to close (data may still be waiting to be written).";break;
    case QAbstractSocket::ListeningState: qDebug()<<"[For internal]";break;
    }
}

void IntProcServer::doReadData()
{
    while(hasPendingDatagrams())
    {
        QByteArray datagram;
        datagram.resize(static_cast<int>(pendingDatagramSize()));
        QHostAddress sender;
        quint16 senderPort;
        readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);
        emit messageIn(QString::fromUtf8(datagram));
    }
}

void IntProcServer::displayError(QTcpSocket::SocketError socketError)
{
    Q_UNUSED(socketError)
    qDebug() << QString("SDL2 Mixer X SingleAPP: The following error occurred: %1.").arg(errorString());
}



static IntProcServer *ipServer = nullptr;

/**
 * @brief LocalServer::LocalServer
 *  Constructor
 */
LocalServer::LocalServer()
{
    ipServer = new IntProcServer();
    connect(ipServer, SIGNAL(messageIn(QString)), this, SLOT(slotOnData(QString)));
    connect(this, SIGNAL(privateDataReceived(QString)), this, SLOT(slotOnData(QString)));
    if(!ipServer->bind(QHostAddress::LocalHost, 58234,  QUdpSocket::ReuseAddressHint|QUdpSocket::ShareAddress))
        qWarning() << ipServer->errorString();
}


/**
 * @brief LocalServer::~LocalServer
 *  Destructor
 */
LocalServer::~LocalServer()
{
    ipServer->close();
    delete ipServer;
}




/**
 * -----------------------
 * QThread requred methods
 * -----------------------
 */
void LocalServer::run()
{
    exec();
}


void LocalServer::exec()
{
    while(ipServer->isOpen())
        msleep(100);
}


/**
 * -------
 * SLOTS
 * -------
 */
void LocalServer::stopServer()
{
    if(ipServer) ipServer->close();
}

void LocalServer::slotOnData(QString data)
{
    qDebug() << data;
    QStringList args = data.split('\n');
    foreach(QString c, args)
    {
        if(c.startsWith("CMD:", Qt::CaseInsensitive))
            onCMD(c);
        else
            emit dataReceived(c);
    }
}


void LocalServer::onCMD(QString data)
{
    //  Trim the leading part from the command
    if(data.startsWith("CMD:"))
    {
        data.remove("CMD:");

        qDebug()<<"Accepted data: "+data;

        QStringList commands;
        commands << "showUp";
        commands << "Is SDL2 Mixer X running?";

        int cmdID = commands.indexOf(data);
        switch(cmdID)
        {
        case 0:
        {
            emit showUp();
            break;
        }
        case 1:
        {
            QUdpSocket answer;
            answer.connectToHost(QHostAddress::LocalHost, 58235);
            answer.waitForConnected(100);
            answer.write(QString("Yes, I'm runs!").toUtf8());
            answer.waitForBytesWritten(100);
            answer.flush();
            break;
        }
        default:
            emit acceptedCommand(data);
        }
    }
    else
        emit acceptedCommand(data);
}
