//=======================================================================================================
// bios_layer.h - Serial command layer for standard BIOS calls
//                It's here if you want it, but I wouldn't suggest it...
//=======================================================================================================

//=======================================================================================================
//=======================================================================================================

#ifndef _BIOS_LAYER_H
#define _BIOS_LAYER_H


//===============================================================================
// Include files
//===============================================================================

#include <pc.h>

//===============================================================================
// Static variable definitions
//===============================================================================

unsigned comport;

//===============================================================================
// Inline function definitions
//===============================================================================

#define BIOS_SER_TIMEOUT	1000000

// Initialize the serial library
// Should return 0 if no error occurred
__inline int GDBStub_SerInit(int port)
{
	comport = (unsigned) port;
	return 0;
}


// Set the serial port speed (and other configurables)
// Should return 0 if the speed is set properly
__inline int GDBStub_SerSpeed(int speed)
{
	unsigned bps;

	switch (speed)
	{
		case 110:
			bps = _COM_110;
			break;

		case 150:
			bps = _COM_150;
			break;

		case 300:
			bps = _COM_300;
			break;

		case 600:
			bps = _COM_600;
			break;

		case 1200:
			bps = _COM_1200;
			break;

		case 2400:
			bps = _COM_2400;
			break;

		case 4800:
			bps = _COM_4800;
			break;

		case 9600:
		default:
			bps = _COM_9600;
			break;
	}

	_bios_serialcom(_COM_INIT, comport,
	                bps | _COM_NOPARITY | _COM_CHR8 | _COM_STOP1);
	return 0;
}


// Check to see if there's room in the buffer to send data
// Should return 0 if it is okay to send
__inline int GDBStub_SerSendOk(void)
{
	return 0;
}


// Send a character to the serial port
// Should return 0 if the send succeeds
__inline int GDBStub_SerSend(int c)
{
	register int ret;
	register int timeout = 0;

	do
	{
		ret = _bios_serialcom(_COM_SEND, comport, (unsigned) c);
	} while((ret != 0) && (timeout++ < BIOS_SER_TIMEOUT));
	return (timeout >= BIOS_SER_TIMEOUT);
}


// Check to see if there are characters waiting in the buffer
// Should return 0 if there's data waiting
__inline int GDBStub_SerRecvOk(void)
{
	return 0;
}


// Read a character from the serial port
// Should return the character read
__inline int GDBStub_SerRecv(void)
{
	register int data;
	register int timeout = 0;

	do
	{
		data = _bios_serialcom(_COM_RECEIVE, comport, 0) & 0xff;
	} while((data > 0xff) && (timeout++ < BIOS_SER_TIMEOUT));

	return data;
}




#endif
