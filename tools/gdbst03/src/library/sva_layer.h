//=======================================================================================================
// sva_layer.h - Serial command layer for SVAsync
// 
//=======================================================================================================

//=======================================================================================================
//=======================================================================================================

#ifndef _SVA_LAYER_H
#define _SVA_LAYER_H


//===============================================================================
// Include files
//===============================================================================

#include "svasync.h"

//===============================================================================
// Inline function definitions
//===============================================================================

// Initialize the serial library
// Should return 0 if no error occurred
__inline int GDBStub_SerInit(int port)
{
	int ret, init;

	ret = init = SVAsyncInit(port - 1);
	if (ret == 0)
		ret = SVAsyncFifoInit();
	if (init == 0)
		atexit(SVAsyncStop);
	return ret;
}


// Set the serial port speed (and other configurables)
// Should return 0 if the speed is set properly
__inline int GDBStub_SerSpeed(int speed)
{
	SVAsyncSet(speed, BITS_8 | NO_PARITY | STOP_1);
	SVAsyncHand(DTR | RTS);
	return 0;
}


// Check to see if there's room in the buffer to send data
// Should return 0 if it is okay to send
__inline int GDBStub_SerSendOk(void)
{
	return !SVAsyncOutStat();
}


// Send a character to the serial port
// Should return 0 if the send succeeds
__inline int GDBStub_SerSend(int c)
{
	SVAsyncOut((char) c);
	return 0;
}


// Check to see if there are characters waiting in the buffer
// Should return 0 if there's data waiting
__inline int GDBStub_SerRecvOk(void)
{
	return SVAsyncInStat();
}


// Read a character from the serial port
// Should return the character read
__inline int GDBStub_SerRecv(void)
{
	return SVAsyncIn();
}




#endif
