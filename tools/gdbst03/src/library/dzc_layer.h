//=======================================================================================================
// dzc_layer.h - Serial command layer for DZcomm
// 
//=======================================================================================================

//=======================================================================================================
//=======================================================================================================

#ifndef _DZC_LAYER_H
#define _DZC_LAYER_H


//===============================================================================
// Include files
//===============================================================================

#include <dzcomm.h>

//===============================================================================
// Static variable definitions
//===============================================================================

comm_port *comport;

//===============================================================================
// Inline function definitions
//===============================================================================

// Initialize the serial library
// Should return 0 if no error occurred
__inline int GDBStub_SerInit(int port)
{
	int ret;
	comm com;

	ret = dzcomm_init();
	if (ret != 0)
	{
		switch (port)
		{
			case 4:
				com = _com4;
				break;

			case 3:
				com = _com3;
				break;

			case 2:
				com = _com2;
				break;

			case 1:
			default:
				com = _com1;
				break;
		}
		comport = comm_port_init(com);
	}
	return (ret == 0);
}

// Set the serial port speed (and other configurables)
// Should return 0 if the speed is set properly
__inline int GDBStub_SerSpeed(int speed)
{
	baud_bits bps;

	switch (speed)
	{
		case 110:
			bps = _110;
			break;

		case 150:
			bps = _150;
			break;

		case 300:
			bps = _300;
			break;

		case 600:
			bps = _600;
			break;

		case 1200:
			bps = _1200;
			break;

		case 2400:
			bps = _2400;
			break;

		case 4800:
			bps = _4800;
			break;

		case 9600:
			bps = _9600;
			break;

		case 19200:
			bps = _19200;
			break;

		case 38400:
			bps = _38400;
			break;

		case 57600:
			bps = _57600;
			break;

		case 115200:
		default:
			bps = _115200;
			break;
	}

	comm_port_set_baud_rate(comport, bps);
	comm_port_set_parity(comport, NO_PARITY);
	comm_port_set_data_bits(comport, BITS_8);
	comm_port_set_stop_bits(comport, STOP_1);
	comm_port_set_flow_control(comport, RTS_CTS);
	comm_port_install_handler(comport);

	return 0;
}

// Check to see if there's room in the buffer to send data
// Should return 0 if it is okay to send
__inline int GDBStub_SerSendOk(void)
{
	return (comm_port_out_full(comport) == 0);
}

// Send a character to the serial port
// Should return 0 if the send succeeds
__inline int GDBStub_SerSend(int c)
{
	return comm_port_out(comport, (unsigned char) c);
}

// Check to see if there are characters waiting in the buffer
// Should return 0 if there's data waiting
__inline int GDBStub_SerRecvOk(void)
{
	return (comm_port_in_empty(comport) == 0);
}

// Read a character from the serial port
// Should return the character read
__inline int GDBStub_SerRecv(void)
{
	return comm_port_test(comport);
}




#endif

/*==================================================================

   $Log:     $


===============================================================*/
