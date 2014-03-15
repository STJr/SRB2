/***********************************************************************
 *  i386-supp.c
 *
 *  Description:  Support functions for the i386 GDB target stub.
 *
 *  Credits:      Created by Jonathan Brogdon
 *
 *  Terms of use:  This software is provided for use under the terms
 *                 and conditions of the GNU General Public License.
 *                 You should have received a copy of the GNU General
 *                 Public License along with this program; if not, write
 *                 to the Free Software Foundation, Inc., 59 Temple Place
 *                 Suite 330, Boston, MA 02111-1307, USA.
 *
 *  Global Data:  None.
 *  Global Functions:
 *    gdb_serial_init
 *    gdb_target_init
 *    gdb_target_close
 *    putDebugChar
 *    getDebugChar
 *
 *  History
 *  Engineer:           Date:              Notes:
 *  ---------           -----              ------
 *  Jonathan Brogdon    20000617           Genesis
 *  Gordon Schumacher   20020212           Updated for modularity 
 *
 ***********************************************************************/


#ifdef DJGPP
#include <bios.h>
#endif

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock.h>
#include <errno.h>
#define DEBUGBUFFERSIZE 1024
HANDLE ser_port = (HANDLE)(-1);

//#include "utility/utility.h"
#ifdef _MSC_VER
#ifndef DEBUG_SERIAL
#pragma comment(lib, "wsock32")
#endif
#endif
#endif

#include <stdlib.h>
#include <i386-stub.h>

#ifdef DJGPP
//#include "bios_layer.h"			// Include this for BIOS calls - NOT RECOMMENDED!
//#include "sva_layer.h"			// Include this for SVAsync usage
#include "dzc_layer.h"			// Include this for DZComm usage


#endif
#define SER_TIMEOUT	1000000

#ifdef _WIN32
static LPTOP_LEVEL_EXCEPTION_FILTER s_prev_exc_handler = 0;


#define I386_EXCEPTION_CNT		17

LONG WINAPI exc_protection_handler(EXCEPTION_POINTERS* exc_info)
{
		int exc_nr = exc_info->ExceptionRecord->ExceptionCode & 0xFFFF;

		if (exc_nr < I386_EXCEPTION_CNT) {
				//LOG(FmtString(TEXT("exc_protection_handler: Exception %x"), exc_nr));

#if 0
				if (exc_nr==11 || exc_nr==13 || exc_nr==14) {
						if (mem_fault_routine)
								mem_fault_routine();
				}
#endif

				++exc_info->ContextRecord->Eip;
		}

		return EXCEPTION_CONTINUE_EXECUTION;
}

LONG WINAPI exc_handler(EXCEPTION_POINTERS* exc_info)
{
		int exc_nr = exc_info->ExceptionRecord->ExceptionCode & 0xFFFF;

		if (exc_nr < I386_EXCEPTION_CNT) {
				//LOG(FmtString("Exception %x", exc_nr));
				//LOG(FmtString("EIP=%08X EFLAGS=%08X", exc_info->ContextRecord->Eip, exc_info->ContextRecord->EFlags));

				 // step over initial breakpoint
				if (s_initial_breakpoint) {
						s_initial_breakpoint = 0;
						++exc_info->ContextRecord->Eip;
				}

				SetUnhandledExceptionFilter(exc_protection_handler);

				win32_exception_handler(exc_info);
				//LOG(FmtString("EIP=%08X EFLAGS=%08X", exc_info->ContextRecord->Eip, exc_info->ContextRecord->EFlags));

				SetUnhandledExceptionFilter(exc_handler);

				return EXCEPTION_CONTINUE_EXECUTION;
		}

		return EXCEPTION_CONTINUE_SEARCH;
}

void disable_debugging()
{
		if (s_prev_exc_handler) {
				SetUnhandledExceptionFilter(s_prev_exc_handler);
				s_prev_exc_handler = 0;
		}
}
#endif

#if !(defined(DJGPP) || defined(_WIN32))
void exceptionHandler(int exc_nr, void* exc_addr)
{
		if (exc_nr>=0 && exc_nr<I386_EXCEPTION_CNT)
				exc_handlers[exc_nr] = exc_addr;
}
#endif

/***********************************************************************
 *  gdb_serial_init
 *
 *  Description:  Initializes the serial port for remote debugging.
 *
 *  Inputs:
 *    port        - the PC COM port to use.
 *    speed       - the COM port speed.
 *  Outputs:  None.
 *  Returns:  0 for success
 *
 ***********************************************************************/
int gdb_serial_init(unsigned int port, unsigned int speed)
{
#ifdef DJGPP
	int ret;

	ret = GDBStub_SerInit(port);
	if (ret == 0)
		ret = GDBStub_SerSpeed(speed);

	return ret;
#elif defined(_WIN32)
		DCB        dcb ;

	port = 0; //TODO

	s_prev_exc_handler = SetUnhandledExceptionFilter(exc_handler);

	ser_port = CreateFile( "COM1", GENERIC_READ | GENERIC_WRITE,
	                               0,                     // exclusive access
	                               NULL,                  // no security attrs
	                               OPEN_EXISTING,
	                               FILE_ATTRIBUTE_NORMAL, 
	                               NULL );
	if( ser_port == (HANDLE)(-1) )
	{
		return 0;
	}
	// buffers
	SetupComm( ser_port, DEBUGBUFFERSIZE, DEBUGBUFFERSIZE ) ;

	// purge buffers
	PurgeComm( ser_port, PURGE_TXABORT | PURGE_RXABORT |
	          PURGE_TXCLEAR | PURGE_RXCLEAR ) ;

	// setup port to 9600 8N1
	dcb.DCBlength = sizeof( DCB ) ;

	GetCommState( ser_port, &dcb ) ;

	dcb.BaudRate = speed;
	dcb.ByteSize = 8;
	dcb.Parity = NOPARITY ;
	dcb.StopBits = ONESTOPBIT ;

	dcb.fDtrControl = DTR_CONTROL_ENABLE ;
	dcb.fRtsControl = RTS_CONTROL_ENABLE ;

	dcb.fBinary = TRUE ;
	dcb.fParity = FALSE ;

	SetCommState( ser_port, &dcb ) ;

	return 1;
#endif
}
/***********************************************************************
 *  gdb_target_init
 *
 *  Description:  This function inializes the GDB target.
 *
 *  Inputs:   None.
 *  Outputs:  None.
 *  Returns:  None.
 *
 ***********************************************************************/
void gdb_target_init(void)
{
	set_debug_traps();
	atexit(restore_traps);
}

/***********************************************************************
 *  gdb_target_close
 *
 *  Description:  This function closes the GDB target.
 *
 *  Inputs:   None.
 *  Outputs:  None.
 *  Returns:  None.
 *
 ***********************************************************************/
void gdb_target_close(void)
{
	restore_traps();
}

/***********************************************************************
 *  putDebugChar
 *
 *  Description:  sends a character to the debug COM port.
 *
 *  Inputs:
 *    c           - the data character to be sent
 *  Outputs:  None.
 *  Returns:  0 for success
 *
 ***********************************************************************/
int putDebugChar(char c)
{
	register int timeout = 0;
#ifdef DJGPP

	while ((GDBStub_SerSendOk() == 0) && (timeout < SER_TIMEOUT))
		timeout++;
	return GDBStub_SerSend(c);
#elif defined(_WIN32)
	DWORD buffer[DEBUGBUFFERSIZE];
	COMSTAT    ComStat ;
	DWORD      dwErrorFlags;
	DWORD      dwLength;

	if(ser_port == (HANDLE)-1)
		return 0;

	buffer[0] = c;

	retrywrite:
	ClearCommError( ser_port, &dwErrorFlags, &ComStat ) ;
	dwLength = ComStat.cbOutQue;

	if (dwLength < DEBUGBUFFERSIZE || timeout > SER_TIMEOUT)
	{
		if(WriteFile( ser_port, buffer, 1, &dwLength, NULL ))
			return 1;
		else if(timeout > SER_TIMEOUT)
			return 0;
	}
	else timeout++;
	goto retrywrite;
#endif
}

/***********************************************************************
 *  getDebugChar
 *
 *  Description:  gets a character from the debug COM port.
 *
 *  Inputs:   None.
 *  Outputs:  None.
 *  Returns:  character data from the serial support.
 *
 ***********************************************************************/
int getDebugChar(void)
{
	register int timeout = 0;
#ifdef DJGPP
	register int ret = -1;

	while ((GDBStub_SerRecvOk() == 0) && (timeout < SER_TIMEOUT))
		timeout++;
	if (timeout < SER_TIMEOUT)
		ret = GDBStub_SerRecv();

	return ret;
#elif defined(_WIN32)
	DWORD buffer[DEBUGBUFFERSIZE];
	COMSTAT    ComStat ;
	DWORD      dwErrorFlags;
	DWORD      dwLength;

	if(ser_port == (HANDLE)-1)
		return -1;

	retryread:
	ClearCommError( ser_port, &dwErrorFlags, &ComStat ) ;
	dwLength = min( DEBUGBUFFERSIZE, ComStat.cbInQue ) ;

	if (dwLength > 0 || timeout > SER_TIMEOUT)
	{
		if(ReadFile( ser_port, buffer, 1, &dwLength, NULL ))
			return  buffer[0];
		else if(timeout > SER_TIMEOUT)
			return -1;
	}
	else timeout++;
	goto retryread;
#endif
}

#if 0
static SOCKET s_rem_fd = INVALID_SOCKET;

int init_gdb_connect()
{
#ifdef _WIN32
		WORD wVersionRequested;
		WSADATA wsa_data;
#endif
		SOCKADDR_IN srv_addr;
		SOCKADDR_IN rem_addr;
		SOCKET srv_socket;
		int rem_len;

		memset(&srv_addr,0,sizeof(srv_addr));
		s_prev_exc_handler = SetUnhandledExceptionFilter(exc_handler);

#ifdef _WIN32
		wVersionRequested= MAKEWORD( 2, 2 );
		if (WSAStartup(wVersionRequested, &wsa_data)) {
				fprintf(stderr, "WSAStartup() failed");
				return 0;
		}
#endif

		srv_addr.sin_family = AF_INET;
		srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		srv_addr.sin_port = htons(9999);

		srv_socket = socket(PF_INET, SOCK_STREAM, 0);
		if (srv_socket == INVALID_SOCKET) {
				perror("socket()");
				return 0;
		}

		if (bind(srv_socket, (struct sockaddr*) &srv_addr, sizeof(srv_addr)) == -1) {
				perror("bind()");
				return 0;
		}

		if (listen(srv_socket, 4) == -1) {
				perror("listen()");
				return 0;
		}

		rem_len = sizeof(rem_addr);

		for(;;) {
				s_rem_fd = accept(srv_socket, (struct sockaddr*)&rem_addr, &rem_len);

				if (s_rem_fd == INVALID_SOCKET) {
						if (errno == EINTR)
								continue;

						perror("accept()");
						return 0;
				}

				break;
		}

		return 1;
}


int getDebugChar()
{
		char buffer[DEBUGBUFFERSIZE];
		int r;

		if (s_rem_fd == INVALID_SOCKET)
				return EOF;

		r = recv(s_rem_fd, buffer, 1, 0);
		if (r == -1) {
				perror("recv()");
				//LOG(TEXT("debugger connection broken"));
				s_rem_fd = INVALID_SOCKET;
				return EOF;
		}

		if (!r)
				return EOF;

		return buffer[0];
}

void putDebugChar(int c)
{
		if (s_rem_fd == INVALID_SOCKET) {
				const char buffer[] = {c};

				if (!send(s_rem_fd, buffer, 1, 0)) {
						perror("send()");
						//LOG(TEXT("debugger connection broken"));
						exit(-1);
				}
		}
}
#endif
