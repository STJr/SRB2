// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  i_tcp.h
/// \brief TCP driver, sockets.

#ifndef __I_TCP__
#define __I_TCP__

extern UINT16 current_port;

/**	\brief	The I_InitTcpNetwork function

	\return	true if going or in a netgame, else it's false
*/
boolean I_InitTcpNetwork(void);

/**	\brief	The I_InitTcpNetwork function

	\return	true if TCP/IP stack was initilized, else it's false
*/
boolean I_InitTcpDriver(void);
void I_ShutdownTcpDriver(void);

#endif
