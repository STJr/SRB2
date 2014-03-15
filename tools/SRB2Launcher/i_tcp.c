// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// MSERV SDK
//
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Adapted by Oogaland.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
//
//
// DESCRIPTION:
//		TCP/IP stuff.
//
//-----------------------------------------------------------------------------




#include <winsock.h>
#include <wsipx.h>


#include "launcher.h"
#include "mserv.h" //Hurdler: support master server




static int init_tcp_driver = 0;


void I_InitTcpDriver(void)
{
    if (!init_tcp_driver)
    {
#ifdef __WIN32__
        WSADATA winsockdata;
        if( WSAStartup(MAKEWORD(1,1),&winsockdata) )
            I_Error("No Tcp/Ip driver detected");
#endif
#ifdef __DJGPP_
        if( !__lsck_init() )
            I_Error("No Tcp/Ip driver detected");
#endif
        init_tcp_driver = 1;
    }
}
