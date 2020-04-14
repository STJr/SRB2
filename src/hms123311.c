// SONIC ROBO BLAST 2 KART
//-----------------------------------------------------------------------------
// Copyright (C) 2020 by James R.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
// \brief HTTP based master server

/*
Documentation available here.

                     <http://mb.srb2.org/MS/tools/api/v1/>
*/

#include <curl/curl.h>

#include "doomdef.h"
#include "d_clisrv.h"
#include "command.h"
#include "mserv.h"
#include "i_tcp.h"/* for current_port */
#include "z_zone.h"

/* I just stop myself from making macros anymore. */
#define Blame( ... ) \
	CONS_Printf("\x85" __VA_ARGS__)

consvar_t cv_masterserver_debug = {
	"masterserver_debug", "Off", CV_SAVE, CV_OnOff,
	NULL, 0, NULL, NULL, 0, 0, NULL/* C90 moment */
};

static int hms_started;

static char *hms_server_token;

struct HMS_buffer
{
	CURL *curl;
	char *buffer;
	int   needle;
	int    end;
};

static void
Contact_error (void)
{
	CONS_Alert(CONS_ERROR,
			"There was a problem contacting the master server...\n"
	);
}

static size_t
HMS_on_read (char *s, size_t _1, size_t n, void *userdata)
{
	struct HMS_buffer *buffer;
	buffer = userdata;
	if (n < ( buffer->end - buffer->needle ))
	{
		memcpy(&buffer->buffer[buffer->needle], s, n);
		buffer->needle += n;
		return n;
	}
	else
		return 0;
}

static struct HMS_buffer *
HMS_connect (const char *format, ...)
{
	va_list ap;
	CURL *curl;
	char *url;
	size_t seek;
	struct HMS_buffer *buffer;

	if (! hms_started)
	{
		if (curl_global_init(CURL_GLOBAL_ALL) != 0)
		{
			Contact_error();
			Blame("From curl_global_init.\n");
			return NULL;
		}
		else
		{
			atexit(curl_global_cleanup);
			hms_started = 1;
		}
	}

	curl = curl_easy_init();

	if (! curl)
	{
		Contact_error();
		Blame("From curl_easy_init.\n");
		return NULL;
	}

	seek = strlen(ms_API) + 1;/* + '/' */

	va_start (ap, format);
	url = ZZ_Alloc(seek + vsnprintf(0, 0, format, ap) + 1);
	va_end (ap);

	sprintf(url, "%s/", ms_API);

	va_start (ap, format);
	vsprintf(&url[seek], format, ap);
	va_end (ap);

	CONS_Printf("HMS: connecting '%s'...\n", url);

	buffer = ZZ_Alloc(sizeof *buffer);
	buffer->curl = curl;
	/* I just allocated 4k and fuck it! */
	buffer->end = 4096;
	buffer->buffer = ZZ_Alloc(buffer->end);
	buffer->needle = 0;

	if (cv_masterserver_debug.value)
	{
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
		curl_easy_setopt(curl, CURLOPT_STDERR, logstream);
	}

	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, HMS_on_read);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, buffer);

	Z_Free(url);

	return buffer;
}

static int
HMS_do (struct HMS_buffer *buffer)
{
	CURLcode cc;
	long status;

	char *p;

	cc = curl_easy_perform(buffer->curl);

	if (cc != CURLE_OK)
	{
		Contact_error();
		Blame(
				"From curl_easy_perform: %s\n",
				curl_easy_strerror(cc)
		);
		return 0;
	}

	buffer->buffer[buffer->needle] = '\0';

	curl_easy_getinfo(buffer->curl, CURLINFO_RESPONSE_CODE, &status);

	if (status != 200)
	{
		p = strchr(buffer->buffer, '\n');

		if (p)
			*p = '\0';

		Contact_error();
		Blame(
				"Master server error %ld: %s%s\n",
				status,
				buffer->buffer,
				( (p) ? "" : " (malformed)" )
		);

		return 0;
	}
	else
		return 1;
}

static void
HMS_end (struct HMS_buffer *buffer)
{
	curl_easy_cleanup(buffer->curl);
	Z_Free(buffer->buffer);
	Z_Free(buffer);
}

int
HMS_fetch_rooms (int joining)
{
	struct HMS_buffer *hms;
	int ok;

	char *id;
	char *title;
	char *motd;

	char *p;
	char *end;

	int i;

	hms = HMS_connect("rooms");

	if (HMS_do(hms))
	{
		p = hms->buffer;

		for (i = 0; i < NUM_LIST_ROOMS && ( end = strstr(p, "\n\n\n") ); ++i)
		{
			*end = '\0';

			id    = strtok(p, "\n");
			title = strtok(0, "\n");
			motd  = strtok(0, "");

			if (id && title && motd)
			{
				room_list[i].header.buffer[0] = 1;

				room_list[i].id = atoi(id);
				strlcpy(room_list[i].name, title, sizeof room_list[i].name);
				strlcpy(room_list[i].motd, motd, sizeof room_list[i].motd);

				p = ( end + 3 );/* skip the three linefeeds */
			}
			else
				break;
		}

		room_list[i].header.buffer[0] = 0;

		ok = 1;
	}
	else
		ok = 0;

	HMS_end(hms);

	return ok;
}

int
HMS_register (void)
{
	struct HMS_buffer *hms;
	int ok;

	char post[256];

	char *title;

	hms = HMS_connect("rooms/%d/register", ms_RoomId);

	title = curl_easy_escape(hms->curl, cv_servername.string, 0);

	snprintf(post, sizeof post,
			"port=%d&"
			"title=%s&"
			"version=%d.%d.%d",

			current_port,

			title,

			VERSION/100,
			VERSION%100,
			SUBVERSION
	);

	curl_free(title);

	curl_easy_setopt(hms->curl, CURLOPT_POSTFIELDS, post);

	ok = HMS_do(hms);

	if (ok)
	{
		hms_server_token = Z_StrDup(strtok(hms->buffer, "\n"));
	}

	HMS_end(hms);

	return ok;
}

void
HMS_unlist (void)
{
	struct HMS_buffer *hms;

	hms = HMS_connect("servers/%s/unlist", hms_server_token);

	curl_easy_setopt(hms->curl, CURLOPT_CUSTOMREQUEST, "POST");

	HMS_do(hms);
	HMS_end(hms);

	Z_Free(hms_server_token);
}

int
HMS_update (void)
{
	struct HMS_buffer *hms;
	int ok;

	char post[256];

	char *title;

	hms = HMS_connect("servers/%s/update", hms_server_token);

	title = curl_easy_escape(hms->curl, cv_servername.string, 0);

	snprintf(post, sizeof post,
			"title=%s",
			title
	);

	curl_free(title);

	curl_easy_setopt(hms->curl, CURLOPT_POSTFIELDS, post);

	ok = HMS_do(hms);
	HMS_end(hms);

	return ok;
}

void
HMS_list_servers (void)
{
	struct HMS_buffer *hms;

	char *p;

	hms = HMS_connect("servers");

	if (HMS_do(hms))
	{
		p = &hms->buffer[strlen(hms->buffer)];
		while (*--p == '\n')
			;

		CONS_Printf("%s\n", hms->buffer);
	}

	HMS_end(hms);
}

msg_server_t *
HMS_fetch_servers (msg_server_t *list, int room_number)
{
	struct HMS_buffer *hms;

	char local_version[9];

	char *room;

	char *address;
	char *port;
	char *title;
	char *version;

	char *end;
	char *section_end;
	char *p;

	int i;

	if (room_number > 0)
	{
		hms = HMS_connect("rooms/%d/servers", room_number);
	}
	else
		hms = HMS_connect("servers");

	if (HMS_do(hms))
	{
		snprintf(local_version, sizeof local_version,
				"%d.%d.%d",
				VERSION/100,
				VERSION%100,
				SUBVERSION
		);

		p = hms->buffer;
		i = 0;

		do
		{
			section_end = strstr(p, "\n\n");

			room = strtok(p, "\n");

			p = strtok(0, "");

			if (! p)
				break;

			while (i < MAXSERVERLIST && ( end = strchr(p, '\n') ))
			{
				*end = '\0';

				address = strtok(p, " ");
				port    = strtok(0, " ");
				title   = strtok(0, " ");
				version = strtok(0, "");

				if (address && port && title && version)
				{
					if (strcmp(version, local_version) == 0)
					{
						strlcpy(list[i].ip,      address, sizeof list[i].ip);
						strlcpy(list[i].port,    port,    sizeof list[i].port);
						strlcpy(list[i].name,    title,   sizeof list[i].name);
						strlcpy(list[i].version, version, sizeof list[i].version);

						list[i].room = atoi(room);

						list[i].header.buffer[0] = 1;

						i++;
					}

					if (end == section_end)/* end of list for this room */
						break;
					else
						p = ( end + 1 );/* skip server delimiter */
				}
				else
				{
					section_end = 0;/* malformed so quit the parsing */
					break;
				}
			}

			p = ( section_end + 2 );
		}
		while (section_end) ;

		list[i].header.buffer[0] = 0;
	}
	else
		list = NULL;

	HMS_end(hms);

	return list;
}

int
HMS_compare_mod_version (char *buffer, size_t buffer_size)
{
	struct HMS_buffer *hms;
	int ok;

	char *version;
	char *version_name;

	hms = HMS_connect("versions/%d", MODID);

	ok = 0;

	if (HMS_do(hms))
	{
		version      = strtok(hms->buffer, " ");
		version_name = strtok(0, "\n");

		if (version && version_name)
		{
			if (atoi(version) != MODVERSION)
			{
				strlcpy(buffer, version_name, buffer_size);
				ok = 1;
			}
			else
				ok = -1;
		}
	}

	HMS_end(hms);

	return ok;
}
