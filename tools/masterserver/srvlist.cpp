// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 2000 by DooM Legacy Team.
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
//-----------------------------------------------------------------------------

#ifdef __GNUC__
#include <unistd.h>
#endif
#include <typeinfo>
#include "common.h"
#include "srvlist.h"

//=============================================================================

/*
**
*/
CList::CList()
{
	list = NULL;
	current = NULL;
}

/*
**
*/
CList::~CList()
{
	CItem *p;

	while (list)
	{
		p = list;
		list = list->next;
		delete(p);
	}
}

/*
**
*/
int CList::insert(CItem *item)
{
	item->next = list;
	list = item;
	return 0;
}

/*
**
*/
int CList::remove(CItem *item)
{
	CItem *position, *q;

	q = NULL;
	position = list;

	while (position && (position != item))
	{
		q = position;
		position = position->next;
	}
	if (position)
	{
		if (q)
			q->next = position->next;
		else
			list = position->next;
		delete position;

		return 1;
	}

	return 0;
}

/*
**
*/
CItem *CList::getFirst()
{
	current = list;
	return current;
}

/*
**
*/
CItem *CList::getNext()
{
	if (current)
		current = current->next;
	return current;
}

/*
**
*/
void CList::show()
{
	CItem *p;

	p = list;
	while (p)
	{
		p->print();
		p = p->next;
	}
}

//=============================================================================

/*
**
*/
CItem::CItem()
{
	next = NULL;
}

//=============================================================================

/*
**
*/
CInetAddr::CInetAddr(const char *ip, const char *port)
{
	strcpy(this->ip, ip);
	strcpy(this->port, port);
	PortNotChanged = true;
}

/*
**
*/
const char *CInetAddr::getIP()
{
	return ip;
}

/*
**
*/
const char *CInetAddr::getPort()
{
	return port;
}

/*
**
*/
bool CInetAddr::setPort(const char *port)
{
	if (PortNotChanged)
	{
		strcpy(this->port, port);
		PortNotChanged = false;
	}
	return !PortNotChanged;
}

//=============================================================================

/*
**
*/
CPlayerItem::CPlayerItem(const char *ip, const char *port,
	const char *nickname) : CInetAddr(ip, port)
{
	strcpy(this->nickname, nickname);
}

/*
**
*/
void CPlayerItem::print()
{
	dbgPrintf(GREEN, "\tIP\t\t: %s\n\tPort\t\t: %s\n\tNickname\t: %s\n",
		ip, port, nickname);
}

/*
**
*/
char *CPlayerItem::getString()
{
	static char tmpbuf[1024];

	snprintf(tmpbuf, sizeof tmpbuf,
		"\tIP\t\t: %s\n\tPort\t\t: %s\n\tNickname\t: %s\n",
		ip, port, nickname);
	return tmpbuf;
}

//=============================================================================

/*
**
*/
CServerItem::CServerItem(const char *ip, const char *port, const char *hostname, const char *version, ServerType type) : CInetAddr(ip, port)
{
	time_t timenow = time(NULL);
	const tm *timeGMT = gmtime(&timenow);
	// check name of server here
	strcpy(this->hostname, hostname);
	strcpy(this->version, version);
	this->type = type;
	strftime(reg_time, REG_TIME_SIZE+1, "%Y-%m-%dT%H:%MZ",timeGMT);
	{
		int i;
		memset(guid,'\0',GUID_SIZE);
		strcpy(&guid[0], ip);
		strcpy(&guid[15], port); // GenUID
		for (i = 0; i <= GUID_SIZE-1; i++)
		{
			if (guid[i] == '\0' || guid[i] == '.')
				guid[i] = '0' + (rand()/(RAND_MAX/15));
			if (guid[i] > '9')
				guid[i] += 'A'-'9';
		}
		guid[GUID_SIZE] = '\0';
	}

	HeartBeat = time(NULL);
}

/*
**
*/
void CServerItem::print()
{
	dbgPrintf(GREEN, "IP\t\t: %s\nPort\t\t: %s\nHostname\t: %s\nVersion\t: %s\nPermanent\t: %s\n",
		ip, port, hostname, version, (type == ST_PERMANENT) ? "Yes" : "No");
}

/*
**
*/
const char *CServerItem::getString()
{
	static char tmpbuf[1024];

	snprintf(tmpbuf, sizeof tmpbuf,
		"IP\t\t: %s\nPort\t\t: %s\nHostname\t: %s\nVersion\t\t: %s\nPermanent\t: %s\n",
		ip, port, hostname, version, (type==ST_PERMANENT) ? "Yes" : "No");
	return tmpbuf;
}

/*
**
*/
const char *CServerItem::getName()
{
	return hostname;
}

/*
**
*/
const char *CServerItem::getVersion()
{
	return version;
}

/*
**
*/
const char *CServerItem::getGuid()
{
	return guid;
}

/*
**
*/
const char *CServerItem::getRegtime()
{
	return reg_time;
}

/*
**
*/
//=============================================================================

/*
**
*/
void CServerList::insertPlayer(CServerItem *server, CPlayerItem *player)
{
	server->players_list.insert(player);
}

/*
**
*/
void CServerList::removePlayer(CServerItem *server, CPlayerItem *player)
{
	server->players_list.remove(player);
}

/*
**
*/
int CServerList::insert(CServerItem *server)
{
	CList::insert((CItem *)server);
	return 0;
}

/*
**
*/
int CServerList::insert(const char *ip, const char *port,
	const char *hostname, const char *version, ServerType type)
{
	CServerItem *server;

	server = new CServerItem(ip, port, hostname, version, type);
	CList::insert(server);
	return 0;
}

/*
**
*/
int CServerList::remove(CServerItem *server)
{
	return CList::remove((CItem *)server);
}

/*
**
*/
int CServerList::remove(const char *ip, const char *port,
	const char *hostname, const char *version, ServerType type)
{
	// TODO
	CServerItem *position, *q;
	bool match;

	(void)hostname;
	(void)port;

	match = false;
	position = (CServerItem *)list;
	q = NULL;

	while (position && !match)
	{
		if (strcmp(position->ip, ip) == 0
			&& strcmp(position->version, version) == 0
			&& strcmp(position->port, port) == 0
			&& position->type == type)
		{
			match = true;
		}
		else
		{
			q = position;
			position = (CServerItem *)position->next;
		}
	}
	if (position && match)
	{
		if (q)
			q->next = position->next;
		else
			list = position->next;
		delete position;

		return 1;
	}

	return 0;
}

/*
**
*/
void CServerList::show()
{
	CServerItem *p;

	p = (CServerItem *)list;
	while (p)
	{
		p->print();
		p->players_list.show();
		p = (CServerItem *)p->next;
	}
}
