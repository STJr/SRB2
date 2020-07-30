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
#include <sys/types.h>
#endif
#include <typeinfo>
#include <string.h>
#include <stdarg.h>
#include "ipcs.h"
#include "common.h"
#include "mysql.h"
#include "md5.h"
#include <time.h>
//#include <netdb.h>

//=============================================================================

#ifdef __GNUC__
#define ATTRPACK __attribute__ ((packed))
#else
#define ATTRPACK
#endif

#define PT_ASKINFOVIAMS 15

static CServerSocket server_socket;

FILE *logfile;
FILE *errorfile;
FILE *mysqlfile;
FILE *sockfile;
FILE *pidfile;

#if defined(_MSC_VER)
#pragma pack(1)
#endif

typedef struct
{
	char ip[16];			// Big enough to hold a full address.
	UINT16 port;
	UINT8 padding1[2];
	UINT32 time;
} ATTRPACK ms_holepunch_packet_t;

typedef struct
{
	char clientaddr[22];
	UINT8 padding1[2];
	UINT32 time;
} ATTRPACK msaskinfo_pak;

//
// SRB2 network packet data.
//
typedef struct
{
	UINT32 checksum;
	UINT8 ack; // if not null the node asks for acknowledgement, the receiver must resend the ack
	UINT8 ackreturn; // the return of the ack number

	UINT8 packettype;
	UINT8 reserved; // padding

	msaskinfo_pak msaskinfo;
} ATTRPACK doomdata_t;

#if defined(_MSC_VER)
#pragma pack()
#endif

//=============================================================================

#define HOSTNAME "localhost"
#define USER "srb2_ms"
#define PASSWORD "gLRDRb7WgLRDRb7W"
#define DATABASE "srb2_ms"

// MySQL Stuff :D

   const char *server = HOSTNAME;
   const char *user = USER;
   const char *password = PASSWORD;
   const char *database = DATABASE;
   time_t lastupdate;

   MYSQL *conn;
   MYSQL_RES *res;
   MYSQL_ROW row;
   int mysqlconnected = 0;
/*
char *str_replace(char * t1, char * t2, char * t6)
{
	char*t4;
	char*t5=new(0);

	while(strstr(t6,t1)){
        t4=strstr(t6,t1);
        strncpy(t5+strlen(t5),t6,t4-t6);
		strcat(t5,t2);
		t4+=strlen(t1);
		t6=t4;
	}
	return strcat(t5,t4);
}
*/
/*
// Cue was here
char *str_quakeformat(char *msg)
{
    char *quakemsg , *quakemsg1, *quakemsg2;
    char *quakemsg3, *quakemsg4, *quakemsg5;
    char *quakemsg6, *quakemsg7, *quakemsg8;

    quakemsg1 = new(sizeof(char) * (strlen(msg) + 1));
    strcpy(quakemsg1, msg);
    quakemsg2 = str_replace("^1", "\x81",quakemsg1);
    quakemsg3 = str_replace("^2", "\x82",quakemsg2);
    quakemsg4 = str_replace("^3", "\x83",quakemsg3);
    quakemsg5 = str_replace("^4", "\x84",quakemsg4);
    quakemsg6 = str_replace("^5", "\x85",quakemsg5);
    quakemsg7 = str_replace("^6", "\x86",quakemsg6);
    quakemsg8 = str_replace("^7", "\x87",quakemsg7);
    quakemsg = str_replace("^0", "\x80",quakemsg8);
    delete(quakemsg1);
    delete(quakemsg2);
    delete(quakemsg3);
    delete(quakemsg4);
    delete(quakemsg5);
    delete(quakemsg6);
    delete(quakemsg7);
    delete(quakemsg8);

    return quakemsg;
}
  */
void MySQL_Conn(bool force) {
	//if(mysql_ping(conn))
		//mysqlconnected = 0;
	logPrintf(mysqlfile, "Trying to connect to MySQL...\n");
	if(mysqlconnected != 1 || force) {
		if(force)
			mysql_close(conn);
		const char *server = HOSTNAME;
		const char *user = USER;
		const char *password = PASSWORD;
		const char *database = DATABASE;

		conn = mysql_init(NULL);

		/* Connect to database */
		if (!mysql_real_connect(conn, server,
			user, password, database, 8219, NULL, 0)) {
			logPrintf(errorfile, "%s\n", mysql_error(conn));
		}
		conn = mysql_init(NULL);

		/* Connect to database */
		if (!mysql_real_connect(conn, server, user, password, database, 0, NULL, 0)) {
			logPrintf(errorfile, "%s\n", mysql_error(conn));
			mysqlconnected = 0;
		} else {
			logPrintf(mysqlfile, "Connection to MySQL Database successful!\n");
			mysqlconnected = 1;
		}
	}
}

static int MySQL_CheckBan(const char *ip, UINT32 id, bool sendinfo, bool type) {
	msg_t msg;
	int writecode;
	MySQL_Conn(false);
	msg.id = id;
    char banqueryp1[500] = "SELECT bid,INET_NTOA(ipstart),INET_NTOA(ipend),DATE_FORMAT(FROM_UNIXTIME(full_endtime),'%%a %%b %%e %%Y at %%k:%%i (CDT)'),reason,hostonly,permanent FROM ms_bans WHERE INET_ATON('%s') BETWEEN `ipstart` AND `ipend` AND (`full_endtime` > %ld OR `permanent` = '1') LIMIT 1";
//    char exqueryp1[500] = "SELECT * FROM ms_exceptions WHERE `ip` = '%s' AND `bid` = '%s' LIMIT 1";
//    char exquery[500];
    char banquery[500];
//    char bid[5];

    time_t current_time = time (NULL);
    sprintf(banquery, banqueryp1, ip, current_time);
    logPrintf(mysqlfile, "Executing MySQL Query: %s\n", banquery);
    if(mysql_query(conn, banquery)) {
      logPrintf(errorfile, "MYSQL ERROR: %s\n", mysql_error(conn));
	  MySQL_Conn(true);
      return false;
    } else {
		res = mysql_store_result(conn);
		if(mysql_num_rows(res) >= 1) {
			logPrintf(logfile, "We got a ban coming on!\n");
			row = mysql_fetch_row(res);
			if(!type && strcmp(row[5],"1") == 0)
				return false;
			if(sendinfo)
			{
				logPrintf(logfile, "And we're meant to tell them some more!\n");
				msg_ban_t *info = (msg_ban_t *) msg.buffer;
				info->header[0] = '\0';

				logPrintf(logfile, "Got Ban: %s - %s, ends %s, for %s\n", row[1], row[2], row[3], row[4]);
				logPrintf(logfile, "COPY: ipstart\n");
				strcpy(info->ipstart, row[1]);
				logPrintf(logfile, "COPY: ipend\n");
				strcpy(info->ipend, row[2]);
				logPrintf(logfile, "COPY: endtime\n");
				if(strcmp(row[6], "0") == 0)
				{
					logPrintf(logfile, "COPY: It's not permanent.\n");
					strcpy(info->endstamp, row[3]);
				}
				else
				{
					logPrintf(logfile, "COPY: It's permanent.\n");
					strcpy(info->endstamp, "Never");
				}
				logPrintf(logfile, "COPY: reason\n");
				strcpy(info->reason, row[4]);
				logPrintf(logfile, "COPY: hostonly\n");
				if(strcmp(row[5],"0") == 0)
					info->hostonly = false;
				else
					info->hostonly = true;

				if(info->hostonly)
					logPrintf(logfile, "BAN HAMMER: %s - %s || Reason: %s || Endstamp: %s || HOSTONLY\n", info->ipstart, info->ipend, info->reason, info->endstamp);
				else
					logPrintf(logfile, "BAN HAMMER: %s - %s || Reason: %s || Endstamp: %s || NOTHOSTONLY\n", info->ipstart, info->ipend, info->reason, info->endstamp);
				mysql_free_result(res);
				msg.type = GET_BANNED_MSG;
				msg.length = sizeof (msg_ban_t);
				msg.room = 0;
				logPrintf(logfile, "Sending message?\n");
				writecode = server_socket.write(&msg);
				logPrintf(logfile, "Message sent! :D\n");
				if (writecode < 0)
				{
					dbgPrintf(LIGHTRED, "Write error... %d client %d "
										"deleted\n", writecode, id);
					return true;
				}
			}
			return true;
		} else {
			mysql_free_result(res);
			return false;
		}
	}
}

int MySQL_CheckRoom(UINT32 room)
{
	MySQL_Conn(false);
	char checkqueryp1[500] = "SELECT private FROM `ms_rooms` WHERE `room_id` = '%d' AND `private` = '1' LIMIT 1";
	char checkquery[500];
	sprintf(checkquery, checkqueryp1, room);
    logPrintf(mysqlfile, "Executing MySQL Query: %s\n", checkquery);
    if(mysql_query(conn, checkquery)) {
      logPrintf(errorfile, "MYSQL ERROR: %s\n", mysql_error(conn));
	  MySQL_Conn(true);
	  return false;
    } else {
      res = mysql_store_result(conn);
	  if(mysql_num_rows(res) < 1)
		return true;
	  else
		return false;
	}
}

void MySQL_AddServer(const char *ip, const char *port, const char *name, const char *version, UINT32 room, bool firstadd, const char *key) {
        char escapedName[255];
        char escapedPort[10];
        char escapedVersion[10];
		char escapedKey[32];
        char insertquery[5000];
        char checkquery[500];
        char updatequery[5000];
        char queryp1[5000] = "INSERT INTO `ms_servers` (`name`,`ip`,`port`,`version`,`timestamp`,`room`,`key`) VALUES ('%s','%s','%s','%s','%ld','%d','%s')";
        char checkqueryp1[500] = "SELECT room_override FROM `ms_servers` WHERE `ip` = '%s' AND `port` = '%s'";
		char updatequeryp1[5000];
		if(firstadd)
		{
			logPrintf(logfile, "First add.\n");
			strcpy(updatequeryp1, "UPDATE `ms_servers` SET `name` = '%s', `port` = '%s', `version` = '%s', timestamp = '%ld', upnow = '1', `room` = '%d', `delisted` = '0', `key` = '%s' WHERE `ip` = '%s' AND `port` = '%s'");
		}
		else
		{
			logPrintf(logfile, "Update ping.\n");
			strcpy(updatequeryp1, "UPDATE `ms_servers` SET `name` = '%s', `port` = '%s', `version` = '%s', timestamp = '%ld', upnow = '1', `room` = '%d', `key` = '%s' WHERE `ip` = '%s' AND `port` = '%s' AND `delisted` = '0'");
		}
        MySQL_Conn(false);
        mysql_real_escape_string(conn, escapedName, name, (unsigned long)strlen(name));
        mysql_real_escape_string(conn, escapedPort, port, (unsigned long)strlen(port));
        mysql_real_escape_string(conn, escapedVersion, version, (unsigned long)strlen(version));
		mysql_real_escape_string(conn, escapedKey, key, (unsigned long)strlen(key));
     if(!MySQL_CheckBan(ip,0,false,1)) {
		if(!MySQL_CheckRoom(room))
		{
			logPrintf(errorfile, "IP %s tried to use the private room %d! THIS SHOULD NOT HAPPEN\n", ip, room);
			return;
		}
        sprintf(checkquery, checkqueryp1, ip, port);
        time_t timestamp;
        timestamp = time (NULL);
        logPrintf(logfile, "Checking for existing servers in table with the same IP and port...\n");
        logPrintf(mysqlfile, "Executing MySQL Query: %s\n", checkquery);
        if(mysql_query(conn, checkquery)) {
          logPrintf(errorfile, "MYSQL ERROR: %s\n", mysql_error(conn));
		  MySQL_Conn(true);
        } else {
          res = mysql_store_result(conn);
          logPrintf(logfile, "Found %d rows...\n", mysql_num_rows(res));
          if(mysql_num_rows(res) < 1) {
             mysql_free_result(res);
			 logPrintf(logfile, "Adding the temporary server: %s:%s || Name: %s || Version: %s || Time: %ld || Room: %d || Key: %s\n", ip, port, name, version, timestamp, room, key);
             sprintf(insertquery, queryp1, escapedName, ip, escapedPort, escapedVersion, timestamp, room, escapedKey);
             logPrintf(mysqlfile, "Executing MySQL Query: %s\n", insertquery);
	     if(mysql_query(conn, insertquery)) {
                logPrintf(errorfile, "MYSQL ERROR: %s\n", mysql_error(conn));
				MySQL_Conn(true);
             } else {
                logPrintf(logfile, "Server added successfully!\n");
             }
          } else {
			row = mysql_fetch_row(res);
			if(atoi(row[0]) != 0)
				room = atoi(row[0]);
            mysql_free_result(res);
            logPrintf(logfile, "Server's IP and port already exists, so let's just update it instead...\n");
            logPrintf(logfile, "Updating Server Data for %s\n", ip);
            sprintf(updatequery, updatequeryp1, escapedName, escapedPort, escapedVersion, timestamp, room, escapedKey, ip, port);
            logPrintf(mysqlfile, "Executing MySQL Query: %s\n", updatequery);
            if(mysql_query(conn, updatequery)) {
               logPrintf(errorfile, "MYSQL ERROR: %s\n", mysql_error(conn));
			   MySQL_Conn(true);
            } else {
				logPrintf(logfile, "Server status for Server: %s:%s || Name: %s || Version: %s || Time: %ld || Room: %d || Key: %s set successfully.\n", ip, port, name, version, timestamp, room, key);
            }
          }
        }
     } else {
            logPrintf(logfile, "IP %s is banned so do nothing.\n", ip);
     }
}

void MySQL_ListServers(UINT32 id, UINT32 type, const char *ip, UINT32 room) {
       msg_t msg;
       int writecode;
       MySQL_Conn(false);
       msg.id = id;
       msg.type = type;
	   char servquery[1000];
	   if(room == 0)
		sprintf(servquery, "SELECT ip, port, name, version FROM ( SELECT * FROM `ms_servers` WHERE `upnow` = '1' ORDER BY `sid` ASC) as t2 ORDER BY `sticky` DESC");
	   else
	   {
		char servqueryp1[1000] = "SELECT ip, port, name, version FROM ( SELECT * FROM `ms_servers` WHERE `upnow` = '1' AND `room` = '%d' ORDER BY `sid` ASC) as t2 ORDER BY `sticky` DESC";
		sprintf(servquery, servqueryp1, room);
	   }
     if(!MySQL_CheckBan(ip,0,false,0)) {
       logPrintf(mysqlfile, "Executing MySQL Query: %s\n", servquery);
       if(mysql_query(conn, servquery)) {
          logPrintf(errorfile, "MYSQL ERROR: %s\n", mysql_error(conn));
		  MySQL_Conn(true);
       } else {
          res = mysql_store_result(conn);
          logPrintf(logfile, "Found %d servers...\n", mysql_num_rows(res));
          while ((row = mysql_fetch_row(res)) != NULL)
          {
		msg_server_t *info = (msg_server_t *) msg.buffer;

		info->header[0] = '\0'; // nothing interresting in it (for now)
		strcpy(info->ip,      row[0]);
		strcpy(info->port,    row[1]);
		strcpy(info->name,    row[2]);
		strcpy(info->version, row[3]);

            msg.length = sizeof (msg_server_t);
			msg.room = 0;
            logPrintf(logfile, "Sending message?\n");
            writecode = server_socket.write(&msg);
            logPrintf(logfile, "Message sent! :D\n");
            if (writecode < 0)
            {
		dbgPrintf(LIGHTRED, "Write error... %d client %d "
		"deleted\n", writecode, id);
		return;
            }
       }
         mysql_free_result(res);
       }
     } else {
            logPrintf(logfile, "IP %s is banned so do nothing.\n", ip);
     }
}

void MySQL_ListRooms(UINT32 id, UINT32 type, const char *ip, bool sendtype) {
	msg_t msg;
	int writecode;
	//(void)ip;
	MySQL_Conn(false);
	msg.id = id;
	msg.type = type;
	char roomquery[1000];
	logPrintf(logfile, "Check their ban status first... (ID %d)\n",id);
	if(MySQL_CheckBan(ip,id,true,sendtype))
		return;

	//Hmph!  I suppose it'll be up to me to handle this. ~Inuyasha
	if (sendtype)
	{
		char overridequery[500];

		sprintf(overridequery, "SELECT room_override FROM `ms_servers` WHERE `ip` = '%s' AND `room_override` > 0", ip);
		logPrintf(mysqlfile, "Checking for room overrides! Executing MySQL Query: %s\n", overridequery);
		if (mysql_query(conn, overridequery))
		{
			logPrintf(errorfile, "MYSQL ERROR: %s\n", mysql_error(conn));
			MySQL_Conn(true);
			return;
		}

		res = mysql_store_result(conn);
		if (mysql_num_rows(res) >= 1)
		{
			int overriddenRoom;

			row = mysql_fetch_row(res);
			overriddenRoom = atoi(row[0]);
			mysql_free_result(res);

			sprintf(roomquery, "SELECT room_id, title, motd FROM `ms_rooms` WHERE `room_id` = '%d' LIMIT 1", overriddenRoom);
			if (mysql_query(conn, roomquery))
			{
				logPrintf(errorfile, "MYSQL ERROR: %s\n", mysql_error(conn));
				MySQL_Conn(true);
				return;
			}
			res = mysql_store_result(conn);
			if (mysql_num_rows(res) >= 1)
			{
				msg_rooms_t *info = (msg_rooms_t *) msg.buffer;
				char roommotd[300];

				row = mysql_fetch_row(res);

				info->header[0] = '\0'; // nothing interesting in it (for now)
				strcpy(info->name, row[1]);

				// Just in case, let's make sure we don't overflow.
				sprintf(roommotd, "\x85Your server has had a room override applied to it, so this is the only room you may host in.\x80\n\n%s", row[2]);
				strncpy(info->motd, roommotd, 255);
				info->motd[255] = '\0';

				info->id = atoi(row[0]);

				logPrintf(logfile, "Sending Room %s with ID %d and MOTD %s\n", info->name, info->id, info->motd);

				msg.length = sizeof (msg_rooms_t);
				msg.room = 0;
				logPrintf(logfile, "Sending message?\n");
				writecode = server_socket.write(&msg);
				logPrintf(logfile, "Message sent! :D\n");
				if (writecode < 0)
				{
					dbgPrintf(LIGHTRED, "Write error... %d client %d "
					                    "deleted\n", writecode, id);
				}
				mysql_free_result(res);
				return;
			}
			else
			{
				logPrintf(errorfile, "Someone's room override isn't set correctly! Room %d doesn't exist!", overriddenRoom);
				mysql_free_result(res);
			}
		}
	}

	if(sendtype)
		sprintf(roomquery, "SELECT room_id, title, motd FROM `ms_rooms` WHERE `private` = '0' AND `visible` = '1' ORDER BY `order` ASC");
	 else
		sprintf(roomquery, "SELECT room_id, title, motd FROM `ms_rooms` WHERE `visible` = '1' ORDER BY `order` ASC");
	if(mysql_query(conn, roomquery)) {
		logPrintf(errorfile, "MYSQL ERROR: %s\n", mysql_error(conn));
		MySQL_Conn(true);
	} else
	{
		res = mysql_store_result(conn);
		logPrintf(logfile, "Found %d rooms...\n", mysql_num_rows(res));
		while ((row = mysql_fetch_row(res)) != NULL)
		{
			msg_rooms_t *info = (msg_rooms_t *) msg.buffer;

			info->header[0] = '\0'; // nothing interresting in it (for now)
			strcpy(info->name,	row[1]);
			strcpy(info->motd,	row[2]);
			info->id = atoi(row[0]);

			logPrintf(logfile, "Sending Room %s with ID %d and MOTD %s\n", info->name, info->id, info->motd);

			msg.length = sizeof (msg_rooms_t);
			msg.room = 0;
			logPrintf(logfile, "Sending message?\n");
			writecode = server_socket.write(&msg);
			logPrintf(logfile, "Message sent! :D\n");
			if (writecode < 0)
			{
				dbgPrintf(LIGHTRED, "Write error... %d client %d "
				                    "deleted\n", writecode, id);
				return;
			}
		   }
		 mysql_free_result(res);
	}
}

void MySQL_CheckVersion(UINT32 id, UINT32 type, const char *ip, UINT32 modid, const char *modversion) {
	msg_t msg;
	int writecode;
	(void)ip;
	MySQL_Conn(false);
	msg.id = id;
	msg.type = type;
	char versionquery[1000];
	sprintf(versionquery, "SELECT * FROM `ms_versions` WHERE `mod_id` = '%d' AND `mod_version` > %s LIMIT 1",modid, modversion);
	logPrintf(mysqlfile, "Executing MySQL Query: %s\n", versionquery);
	if(mysql_query(conn, versionquery)) {
		logPrintf(errorfile, "MYSQL ERROR: %s\n", mysql_error(conn));
		MySQL_Conn(true);
	} else
	{
		res = mysql_store_result(conn);
		if(mysql_num_rows(res) < 1)
		{
			logPrintf(logfile, "Buffer to NULL\n");
			strcpy(msg.buffer,"NULL");
		}
		else
		{
			row = mysql_fetch_row(res);
			logPrintf(logfile, "Version found: %s || Codebase: %s || Version: %s || Version String: %s || Mod ID: %s\n", row[4], row[3], row[1], row[2], row[0]);
			strcpy(msg.buffer,row[2]);
		}
		msg.length = sizeof msg.buffer;
		msg.room = 0;
		logPrintf(logfile, "Sending message?\n");
		writecode = server_socket.write(&msg);
		logPrintf(logfile, "Message sent! :D\n");
		if (writecode < 0)
		{
			dbgPrintf(LIGHTRED, "Write error... %d client %d "
			                    "deleted\n", writecode, id);
			return;
		}
		mysql_free_result(res);
	}
}

void MySQL_ListServServers(UINT32 id, UINT32 type, const char *ip) {
       msg_t msg;
       int writecode;
       static char str[1024];
       //char banqueryp1[500] = "SELECT reason,name,FROM_UNIXTIME(endtime) FROM ms_bans WHERE INET_ATON('%s') BETWEEN `ipstart` AND `ipend` AND `endtime` < %ld LIMIT 1";
       //char banquery[500];
       //time_t current_time = time (NULL);
       char servquery[1000] = "SELECT ip, port, name, version, permanent FROM ( SELECT * FROM `ms_servers` WHERE `upnow` = '1' OR `permanent` = '1' ORDER BY `sid` ASC) as ms_servers ORDER BY `sticky` DESC";
       MySQL_Conn(false);
     if(!MySQL_CheckBan(ip,0,false,0)) {
       msg.id = id;
       msg.type = type;
	   msg.room = 0;
       logPrintf(mysqlfile, "Executing MySQL Query: %s\n", servquery);
       if(mysql_query(conn, servquery)) {
          logPrintf(errorfile, "MYSQL ERROR: %s\n", mysql_error(conn));
		  MySQL_Conn(true);
       } else {
          res = mysql_store_result(conn);
          logPrintf(logfile, "Found %d servers...\n", mysql_num_rows(res));
          while ((row = mysql_fetch_row(res)) != NULL)
	  {
                if(strcmp(row[4], "0") == 1)
                snprintf(str, sizeof str, "IP\t\t: %s\nPort\t\t: %s\nHostname\t: %s\nVersion\t\t: %s\nPermanent\t: %s\n", row[0], row[1], row[2], row[3], "Yes");
		else
                snprintf(str, sizeof str, "IP\t\t: %s\nPort\t\t: %s\nHostname\t: %s\nVersion\t\t: %s\nPermanent\t: %s\n", row[0], row[1], row[2], row[3], "No");

        msg.length = (INT32)(strlen(str)+1); // send also the '\0'
		strcpy(msg.buffer, str);
		dbgPrintf(CYAN, "Writing: (%d)\n%s\n", msg.length, msg.buffer);
		writecode = server_socket.write(&msg);
		if (writecode < 0)
		{
			dbgPrintf(LIGHTRED, "Write error... %d client %d deleted\n", writecode, id);
			return;
		}
	 }
         mysql_free_result(res);
       }
     } else {
            logPrintf(logfile, "IP %s is banned so do nothing! Let them find out for themselves.\n", ip);
     }
}

void MySQL_RemoveServer(char *ip, char *port, char *name, char *version) {
        char escapedName[255];
        char updatequery[5000];
        char updatequeryp1[5000] = "UPDATE `ms_servers` SET upnow = '0' WHERE `ip` = '%s' AND `port` = '%s' AND `permanent` = '0'";
        MySQL_Conn(false);
        mysql_real_escape_string(conn, escapedName, name, (unsigned long)strlen(name));
        sprintf(updatequery, updatequeryp1, ip, port);
        logPrintf(mysqlfile, "Executing MySQL Query: %s\n", updatequery);
        if(mysql_query(conn, updatequery)) {
           logPrintf(errorfile, "MYSQL ERROR: %s\n", mysql_error(conn));
		   MySQL_Conn(true);
        } else {
           logPrintf(logfile, "Server: %s:%s || Name: %s || Version: %s removed successfully.\n", ip, port, name, version);
        }
}

/* LIVE FUNCTIONS */
void LIVE_AuthUser(UINT32 id, char *buffer)
{
	msg_live_auth_t *auth;
	char escapedName[255];
	char saltedPassword[62];
	char hashedPassword[34];
	char namequery[1000];
	char namequeryp1[1000] = "SELECT username,userid,salt,password FROM `user` WHERE `username` = '%s' LIMIT 1";
	char userquery[1000];
	char userqueryp1[1000] = "SELECT userid,username,live_authkey,live_publickey FROM `user` WHERE `userid` = '%s' LIMIT 1";
	msg_t msg2;
	char ip[32];
	int writecode;
	msg2.type = LIVE_INVALID_USER;

	auth = (msg_live_auth_t *)buffer;

	logPrintf(logfile, "Got User %s Pass %s\n", auth->username, auth->password);
	// retrieve the true ip of the server
	strcpy(ip, server_socket.getClientIP(id));
	MySQL_Conn(false);
	msg2.id = id;
	logPrintf(logfile, "Check their ban status first... (ID %d)\n",id);
	if(MySQL_CheckBan(ip,0,false,0))
		return;
	mysql_real_escape_string(conn, escapedName, auth->username, (unsigned long)strlen(auth->username));
	sprintf(namequery, namequeryp1, escapedName);
	logPrintf(mysqlfile, "Executing MySQL Query: %s\n", namequery);
    if(mysql_query(conn, namequery)) {
		logPrintf(errorfile, "MYSQL ERROR: %s\n", mysql_error(conn));
		MySQL_Conn(true);
	} else {
		res = mysql_store_result(conn);
		if(mysql_num_rows(res) < 1)
		{
			logPrintf(logfile, "Invalid user: %s\n", auth->username);
			msg2.type = LIVE_INVALID_USER;
		}
		else
		{
			row = mysql_fetch_row(res);
			sprintf(saltedPassword, "%s%s", auth->password, row[2]);
			logPrintf(logfile, "Pre-hash: %s (Salt is %s)\n", saltedPassword, row[2]);
			sprintf(hashedPassword, md5(saltedPassword).c_str());
			logPrintf(logfile, "Got Hashed Password: %s\n", hashedPassword);
			if(strcmp(hashedPassword, row[3]) != 0)
			{
				msg2.type = LIVE_INVALID_USER;
				logPrintf(logfile, "INVALID USER!\n");
			} else {
				msg2.type = LIVE_SEND_USER;
				sprintf(userquery, userqueryp1, row[1]);
				mysql_free_result(res);
				logPrintf(mysqlfile, "Executing MySQL Query: %s\n", userquery);
			    if(mysql_query(conn, userquery)) {
					logPrintf(errorfile, "MYSQL ERROR: %s\n", mysql_error(conn));
					MySQL_Conn(true);
				} else {
					res = mysql_store_result(conn);
					if(mysql_num_rows(res) < 1)
					{
						logPrintf(errorfile, "User Information Error, invalid USER ID!\n");
						msg2.type = LIVE_INVALID_USER;
					} else {
						row = mysql_fetch_row(res);
						msg_live_user_t *user = (msg_live_user_t *) msg2.buffer;

						user->header[0] = '\0'; // nothing interresting in it (for now)
						user->uid = atoi(row[0]);
						strcpy(user->username,	row[1]);
						logPrintf(logfile, "Found user: %s, id %s\n", row[1], row[0]);
						logPrintf(logfile, "Sending User %s with ID %d\n", user->username, user->uid);

						msg2.length = sizeof (msg_live_user_t);
					}
				}
			}
		}
	}
	msg2.length = sizeof msg2.buffer;
	msg2.room = 0;
	logPrintf(logfile, "Sending message?\n");
	writecode = server_socket.write(&msg2);
	logPrintf(logfile, "Message sent! :D\n");
	if (writecode < 0)
	{
		dbgPrintf(LIGHTRED, "Write error... %d client %d "
		                    "deleted\n", writecode, id);
		return;
	}
	mysql_free_result(res);
}

/*
** sendServersInformations()
*/
static void sendServersInformations(UINT32 id, UINT32 room)
{
	msg_t msg;
	int writecode;
	char ip[16];
        strcpy(ip, server_socket.getClientIP(id));
	(void)room;
	logPrintf(logfile, "Sending servers informations\n");
	msg.id = id;
	msg.type = SEND_SERVER_MSG;
	msg.room = 0;
	MySQL_ListServServers(id, SEND_SERVER_MSG, ip); // Awesome new MySQL Code!
	msg.length = 0;
	//dbgPrintf(CYAN, "Writing: (%d) %s\n", msg.length, "");
	writecode = server_socket.write(&msg);
	if (writecode < 0)
	{
		dbgPrintf(LIGHTRED, "Write error... %d client %d deleted\n",
			writecode, id);
	}
}

/*
** sendShortServersInformations()
*/
static void sendShortServersInformations(UINT32 id, UINT32 room)
{
	msg_t msg;
	int writecode;
	char ip[16];
        strcpy(ip, server_socket.getClientIP(id));

	logPrintf(logfile, "Sending short servers informations\n");
	msg.id = id;
	msg.type = SEND_SHORT_SERVER_MSG;
	MySQL_ListServers(id, SEND_SHORT_SERVER_MSG, ip, room); // New code that's full of win!
	logPrintf(logfile, "Packet Room: %d\n", room);
	msg.length = 0;
	writecode = server_socket.write(&msg);
	if (writecode < 0)
	{
		dbgPrintf(LIGHTRED, "Write error... %d client %d deleted\n",
			writecode, id);
	}
}

/*
** sendRoomInformations()
*/
static void sendRoomInformations(UINT32 id, UINT32 room, bool type)
{
	msg_t msg;
	int writecode;
	char ip[16];
        strcpy(ip, server_socket.getClientIP(id));
	(void)room;
	logPrintf(logfile, "Sending room information\n");
	msg.id = id;
	msg.type = SEND_ROOMS_MSG;
	msg.room = 0;
	MySQL_ListRooms(id, SEND_ROOMS_MSG, ip, type); // New code that's full of win!
	msg.length = 0;
	writecode = server_socket.write(&msg);
	if (writecode < 0)
	{
		dbgPrintf(LIGHTRED, "Write error... %d client %d deleted\n",
			writecode, id);
	}
}

/*
** sendVersionInformations()
*/
static void sendVersionInformations(UINT32 id, UINT32 modid, char *modversion)
{
	msg_t msg;
	int writecode;
	char ip[16];
        strcpy(ip, server_socket.getClientIP(id));

	logPrintf(logfile, "Sending version information\n");
	msg.id = id;
	msg.type = SEND_VERSION_MSG;
	msg.room = 0;
	MySQL_CheckVersion(id, SEND_VERSION_MSG, ip, modid, modversion); // New code that's full of win!
	msg.length = sizeof msg.buffer;
	writecode = server_socket.write(&msg);
	if (writecode < 0)
	{
		dbgPrintf(LIGHTRED, "Write error... %d client %d deleted\n",
			writecode, id);
	}
}

/*
** addServer()
*/
static void addServer(int id, char *buffer, bool firstadd)
{
	msg_server_t *info;
	char oldversion = 0;

	//TODO: Be sure there is no flood from a given IP:
	//      If a host need more than 2 servers, then it should be registrated
	//      manually

	info = (msg_server_t *)buffer;

	// I want to be sure the informations are correct, of course!
	info->port[sizeof (info->port)-1] = '\0';
	info->name[sizeof (info->name)-1] = '\0';
	info->version[sizeof (info->version)-1] = '\0';

	logPrintf(logfile, "addServer(): Version = \"%s\"\n", info->version);
	logPrintf(logfile, "addServer(): Key = \"%s\"\n", info->key);

	// retrieve the true ip of the server
	strcpy(info->ip, server_socket.getClientIP(id));
	//strcpy(info->port, server_socket.getClientPort(id));

	if (info->version[0] == '1' &&
		info->version[1] == '.' &&
		info->version[2] == '0' &&
		info->version[3] == '9' &&
		info->version[4] == '.')
		{
			if ((info->version[5] == '2') || (info->version[5] == '3'))
			{
					oldversion = 1;
			}
		}

	if (info->version[0] == '1' &&
		info->version[1] == '.' &&
		info->version[2] == '6' &&
		info->version[3] == '9' &&
		info->version[4] == '.' &&
		info->version[5] == '6')
		{
					oldversion = 1;
		}

	if (!oldversion)
	{
		MySQL_AddServer(info->ip, info->port, info->name, info->version, info->room, firstadd, info->key);
	}
	else
	{
		logPrintf(logfile, "Not adding the temporary server: %s %s %s %s\n", info->ip, info->port, info->name, info->version);
	}
}

/*
** removeServer()
*/
static void removeServer(int id, char *buffer)
{
	msg_server_t *info;

	info = (msg_server_t *)buffer;

	// I want to be sure the informations are correct, of course!
	info->port[sizeof (info->port)-1] = '\0';
	info->name[sizeof (info->name)-1] = '\0';
	info->version[sizeof (info->version)-1] = '\0';

	// retrieve the true ip of the server
	strcpy(info->ip, server_socket.getClientIP(id));

	logPrintf(logfile, "Removing the temporary server: %s %s %s %s\n", info->ip, info->port, info->name, info->version);
	MySQL_RemoveServer(info->ip, info->port, info->name, info->version); // New and win.
}

/*
** analyseUDP()
*/
static int analyseUDP(size_t size, const char *buffer)
{
	// this would be the part of reading the PT_SERVERINFO packet,
	// but i'm not about to backport that sloppy code
	(void)size;
	(void)buffer;
	return INVALID_MSG;
}

/*
** UDPMessage()
*/
static int UDPMessage(size_t size, const char *buffer)
{
	if (size == 4)
	{
		return 0;
	}
	else if (size == sizeof(ms_holepunch_packet_t))
	{
		return 0;
	}
	else if (size > 58)
	{
		dbgPrintf(LIGHTGREEN, "Got a UDP %d byte long message\n", size);
		return analyseUDP(size, buffer);
	}
	else
		return INVALID_MSG;
}

/*
** analyseMessage()
*/
static int analyseMessage(msg_t *msg)
{
	switch (msg->type)
	{
	case UDP_RECV_MSG:
		return UDPMessage(msg->length, msg->buffer);
	case ACCEPT_MSG:
		break;
	case ADD_SERVER_MSG:
		addServer(msg->id, msg->buffer, true);
		break;
	case PING_SERVER_MSG:
		addServer(msg->id, msg->buffer, false);
		break;
	case REMOVE_SERVER_MSG:
		removeServer(msg->id, msg->buffer);
		break;
	case GET_SERVER_MSG:
		sendServersInformations(msg->id, msg->room);
		break;
	case GET_SHORT_SERVER_MSG:
		sendShortServersInformations(msg->id, msg->room);
		break;
	case GET_ROOMS_MSG:
		sendRoomInformations(msg->id, msg->room, 0);
		break;
	case GET_ROOMS_HOST_MSG:
		sendRoomInformations(msg->id, msg->room, 1);
		break;
	case GET_VERSION_MSG:
		sendVersionInformations(msg->id, msg->room, msg->buffer);
		break;
	case LIVE_AUTH_USER:
		LIVE_AuthUser(msg->id, msg->buffer);
		break;
	default:
		return INVALID_MSG;
	}
	return 0;
}

/*
** main()
*/
int main(int argc, char *argv[])
{
	msg_t msg;
#ifdef __unix__
	pid_t pid;
	char pidstr[12];
#endif

	if (argc <= 1)
	{
		fprintf(stderr, "usage: %s port\n", argv[0]);
		exit(1);
	}

	if (server_socket.listen(argv[1]) < 0)
	{
		fprintf(stderr, "Error while initializing the server; port being used! Try killing the other Master Server.\n");
		exit(2);
	}

	logfile = openFile("server.log");
	errorfile = openFile("error.log");
	mysqlfile = openFile("mysql.log");
	sockfile = openFile("sockets.log");
	MySQL_Conn(false);
#if !defined (DEBUG) && defined (__unix__)
	pid = fork();
	switch (pid)
	{
	case 0: break;  // child
	case -1: printf("Error while launching the server in background\n"); return -1;
	default:
		pidfile = fopen("msd.pid", "w+");
		sprintf(pidstr, "%d", pid);
		fputs(pidstr, pidfile);
		fclose(pidfile);
		return 0; // parent: keep child in background
	}
#endif
	srand((unsigned)time(NULL)); // Alam: GUIDs
	for (;;)
	{
		memset(&msg, 0, sizeof (msg)); // remove previous message
		if (!server_socket.read(&msg))
		{
			// valid message: header message seems ok
			analyseMessage(&msg);
			//servers_list.show(); // for debug purpose
		}
	}

#ifndef _MSC_VER
	/* NOTREACHED */
	return 0;
#endif
}
