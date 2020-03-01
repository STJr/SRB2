// SONIC ROBO BLAST 2 JART
//-----------------------------------------------------------------------------
// Copyright (C) 2020 by James R.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
// \brief HTTP based master server

#include <curl/curl.h>

#include "doomdef.h"
#include "d_clisrv.h"
#include "command.h"
#include "mserv.h"
#include "i_tcp.h"/* for current_port */

consvar_t cv_http_masterserver = {
	"http_masterserver",
	"http://www.jameds.org/MS/0",
	CV_SAVE,

	NULL, NULL, 0, NULL, NULL, 0, 0, NULL/* C90 moment */
};

consvar_t cv_masterserver_token = {
	"masterserver_token",
	"",
	CV_SAVE,

	NULL, NULL, 0, NULL, NULL, 0, 0, NULL/* C90 moment */
};

static int hms_started;

static char hms_server_token[sizeof "xxx.xxx.xxx.xxx/xxxxx"];

struct HMS_buffer
{
	CURL *curl;
	char *buffer;
	int   needle;
	int    end;
};

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
		curl_global_init(CURL_GLOBAL_ALL);
		hms_started = 1;
	}

	curl = curl_easy_init();

	seek = strlen(cv_http_masterserver.string) + 1;/* + '/' */

	va_start (ap, format);
	url = malloc(seek + vsnprintf(0, 0, format, ap) + 1);
	va_end (ap);

	sprintf(url, "%s/", cv_http_masterserver.string);

	va_start (ap, format);
	vsprintf(&url[seek], format, ap);
	va_end (ap);

	CONS_Printf("HMS: connecting '%s'...\n", url);

	buffer = malloc(sizeof *buffer);
	buffer->curl = curl;
	/* I just allocated 4k and fuck it! */
	buffer->end = 4096;
	buffer->buffer = malloc(buffer->end);
	buffer->needle = 0;

	curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, HMS_on_read);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, buffer);

	free(url);

	return buffer;
}

static int
HMS_do (struct HMS_buffer *buffer)
{
	long status;
	curl_easy_perform(buffer->curl);
	curl_easy_getinfo(buffer->curl, CURLINFO_RESPONSE_CODE, &status);
	buffer->buffer[buffer->needle] = '\0';
	if (status != 200)
	{
		CONS_Alert(CONS_ERROR,
				"Master server error %ld: %s",
				status,
				buffer->buffer
		);
		return 0;
	}
	else
		return 1;
}

static void
HMS_end (struct HMS_buffer *buffer)
{
	free(buffer->buffer);
	curl_easy_cleanup(buffer->curl);
	free(buffer);
}

int
HMS_in_use (void)
{
	return cv_http_masterserver.string[0];
}

void
HMS_fetch_rooms (void)
{
	struct HMS_buffer *hms;
	char *p;
	char *end;

	char *id;
	char *title;
	char *motd;

	int i;

	hms = HMS_connect("rooms?token=%s&verbose", cv_masterserver_token.string);
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

				p = ( end + 3 );
			}
			else
				break;
		}

		room_list[i].header.buffer[0] = 1;
		room_list[i].id = 0;
		strcpy(room_list[i].name, "All");
		strcpy(room_list[i].motd, "Wildcard.");

		room_list[i + 1].header.buffer[0] = 0;
	}
	HMS_end(hms);
}

int
HMS_register (void)
{
	char post[256];
	struct HMS_buffer *hms;
	char *title;
	int ok;

	hms = HMS_connect("rooms/%d/register?token=%s",
			ms_RoomId,
			cv_masterserver_token.string
	);
	title = curl_easy_escape(hms->curl, cv_servername.string, 0);
	sprintf(post,
			"port=%d&title=%s",
			current_port,
			title
	);
	curl_easy_setopt(hms->curl, CURLOPT_POSTFIELDS, post);

	ok = HMS_do(hms);

	if (ok)
	{
		strlcpy(hms_server_token, strtok(hms->buffer, "\n"),
				sizeof hms_server_token);
	}

	curl_free(title);
	HMS_end(hms);

	return ok;
}

void
HMS_unlist (void)
{
	struct HMS_buffer *hms;
	hms = HMS_connect("servers/%s/unlist?token=%s",
			hms_server_token,
			cv_masterserver_token.string
	);
	curl_easy_setopt(hms->curl, CURLOPT_CUSTOMREQUEST, "POST");
	/*curl_easy_setopt(hms->curl, CURLOPT_POSTFIELDS,
			cv_masterserver_token.string);*/
	HMS_do(hms);
	HMS_end(hms);
}

void
HMS_update (void)
{
	char post[256];
	struct HMS_buffer *hms;
	char *title;

	hms = HMS_connect("servers/%s/update?token=%s",
			hms_server_token,
			cv_masterserver_token.string
	);

	title = curl_easy_escape(hms->curl, cv_servername.string, 0);
	sprintf(post, "title=%s", title);
	curl_free(title);

	curl_easy_setopt(hms->curl, CURLOPT_POSTFIELDS, post);

	HMS_do(hms);
	HMS_end(hms);
}

void
HMS_list_servers (void)
{
	struct HMS_buffer *hms;
	hms = HMS_connect("servers?token=%s", cv_masterserver_token.string);
	if (HMS_do(hms))
		CONS_Printf("%s", hms->buffer);
	HMS_end(hms);
}

void
HMS_fetch_servers (msg_server_t *list, int room_number)
{
	struct HMS_buffer *hms;
	char *end;
	char *section_end;
	char *p;

	char *room;
	char *address;
	char *port;
	char *title;

	int i;

	if (room_number > 0)
	{
		hms = HMS_connect("rooms/%d/servers?token=%s",
				room_number,
				cv_masterserver_token.string
		);
	}
	else
		hms = HMS_connect("servers?token=%s", cv_masterserver_token.string);

	if (HMS_do(hms))
	{
		p = hms->buffer;
		i = 0;
		do
		{
			section_end = strstr(p, "\n\n");
			room = strtok(p, "\n");
			p = strtok(0, "");
			for (; i < MAXSERVERLIST && ( end = strchr(p, '\n') ); ++i)
			{
				*end = '\0';

				address = strtok(p, " ");
				port    = strtok(0, " ");
				title   = strtok(0, "");

				if (address && port && title)
				{
					strlcpy(list[i].ip, address, sizeof list[i].ip);
					strlcpy(list[i].port, port, sizeof list[i].port);
					strlcpy(list[i].name, title, sizeof list[i].name);
					list[i].room = atoi(room);

					list[i].header.buffer[0] = 1;

					if (end == section_end)
					{
						i++;/* lol */
						break;
					}

					p = ( end + 1 );
				}
				else
				{
					section_end = 0;
					break;
				}
			}
			p = ( section_end + 2 );
		}
		while (section_end) ;

		list[i].header.buffer[0] = 0;
	}

	HMS_end(hms);
}
