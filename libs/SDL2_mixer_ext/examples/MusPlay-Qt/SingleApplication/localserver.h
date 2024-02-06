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

#ifndef LOCALSERVER_H
#define LOCALSERVER_H

#include <QThread>
#include <QVector>
#include <QTcpServer>
#include <QUdpSocket>
#include <QTcpSocket>
#include <QLocalSocket>

/*!
 * \brief Interprocess connection proxy class
 */
class IntProcServer  : public QUdpSocket
{
    Q_OBJECT
public:
    explicit IntProcServer();
    ~IntProcServer();

public slots:
    /*!
     * \brief UDP Socket state change event
     * \param stat New state of socket
     */
    void stateChanged(QAbstractSocket::SocketState stat);

signals:
    void messageIn(QString msg);

protected slots:
    void doReadData();
    void displayError(QAbstractSocket::SocketError socketError);
};

/*!
 * \brief Local server thread class
 */
class LocalServer : public QThread
{
    Q_OBJECT
public:
    LocalServer();
    ~LocalServer();
    void shut();

protected:
    /**
     * -----------------------
     * QThread requred methods
     * -----------------------
     */

    /**
     * @brief run
     *  Initiate the thread.
     */
    void run();

    /**
     * @brief LocalServer::exec
     *  Keeps the thread alive. Waits for incomming connections
     */
    void exec();

signals:
    /*!
     * \brief Triggering when raw data was received by server
     * \param data received rae data
     */
    void dataReceived(QString data);
    /*!
     * \brief Triggering when raw data was received by server
     * \param data received rae data
     */
    void privateDataReceived(QString data);

    /*!
     * \brief Show up window command signal
     */
    void showUp();

    /*!
     * \brief File open incoming command
     * \param path full file path to open
     */
    void openFile(QString path);

    /*!
     * \brief Accepted raw command signal
     * \param cmd command line
     */
    void acceptedCommand(QString cmd);

private slots:
    /**
     * @brief Initialize stopping of server
     */
    void stopServer();

    /**
    * @brief LocalServer::slotOnData
    *  Executed when data is received
    * @param data
    */
    void slotOnData(QString data);

private:
    /**
     * @brief Parse accepted line and run necessary commands
     * @param data Input data to parse
     */
    void onCMD(QString data);
};

#endif // LOCALSERVER_H
