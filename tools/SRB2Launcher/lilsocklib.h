// Unlike lilsocklib.c, since this takes code from SRB2,
// it is under the GPL, rather than public domain. =(
//
#ifndef __LILSOCKLIB_H__
#define __LILSOCKLIB_H__

#define SD_BOTH         0x02

#define PACKET_SIZE 1024

#define  MS_NO_ERROR               0
#define  MS_SOCKET_ERROR        -201
#define  MS_CONNECT_ERROR       -203
#define  MS_WRITE_ERROR         -210
#define  MS_READ_ERROR          -211
#define  MS_CLOSE_ERROR         -212
#define  MS_GETHOSTBYNAME_ERROR -220
#define  MS_GETHOSTNAME_ERROR   -221
#define  MS_TIMEOUT_ERROR       -231

// see master server code for the values
#define ADD_SERVER_MSG           101
#define REMOVE_SERVER_MSG        103
#ifdef MASTERSERVERS12
#define ADD_SERVERv2_MSG         104
#endif
#define GET_SERVER_MSG           200
#define GET_SHORT_SERVER_MSG     205
#ifdef MASTERSERVERS12
#define ASK_SERVER_MSG           206
#define ANSWER_ASK_SERVER_MSG    207
#endif

#define HEADER_SIZE ((long)sizeof (long)*3)

#define HEADER_MSG_POS    0
#define IP_MSG_POS       16
#define PORT_MSG_POS     32
#define HOSTNAME_MSG_POS 40

/** A message to be exchanged with the master server.
  */
typedef struct
{
	long id;                  ///< Unused?
	long type;                ///< Type of message.
	long length;              ///< Length of the message.
	char buffer[PACKET_SIZE]; ///< Actual contents of the message.
} msg_t;

SOCKET ConnectSocket(char* IPAddress);
#endif
