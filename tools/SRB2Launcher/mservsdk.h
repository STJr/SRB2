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
//      Header file for the master server SDK.
//
//-----------------------------------------------------------------------------

#ifndef _MSERVSDK_H_
#define _MSERVSDK_H_



#ifndef IMPORT
#ifdef __cplusplus
#define IMPORT extern "C" __declspec (dllimport)
#else
#define IMPORT __declspec (dllimport)
#endif
#endif




struct SERVERLIST
{
	char ip[16];
	char port[6];
	char name[32];
	char version[16];
	char perm[4];
};



IMPORT int __stdcall GetServerListEx(char *host, char *str_port, struct SERVERLIST serverlist[], short max_servers);
IMPORT int __stdcall GetServerList(char *host, char *str_port,
						 struct SERVERLIST *serverlist1,struct SERVERLIST *serverlist2,struct SERVERLIST *serverlist3,
						 struct SERVERLIST *serverlist4,struct SERVERLIST *serverlist5,struct SERVERLIST *serverlist6,
						 struct SERVERLIST *serverlist7,struct SERVERLIST *serverlist8,struct SERVERLIST *serverlist9,
						 struct SERVERLIST *serverlist10,struct SERVERLIST *serverlist11,struct SERVERLIST *serverlist12,
						 struct SERVERLIST *serverlist13,struct SERVERLIST *serverlist14,struct SERVERLIST *serverlist15,
						 struct SERVERLIST *serverlist16);




#endif	// !defined(_MSERVSDK_H_)
