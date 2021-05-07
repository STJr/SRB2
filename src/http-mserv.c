// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2020-2021 by James R.
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

#ifdef HAVE_CURL
#include <curl/curl.h>
#endif

#include "doomdef.h"
#include "d_clisrv.h"
#include "command.h"
#include "m_argv.h"
#include "m_menu.h"
#include "mserv.h"
#include "i_tcp.h"/* for current_port */
#include "i_threads.h"

/* reasonable default I guess?? */
#define DEFAULT_BUFFER_SIZE (4096)

/* I just stop myself from making macros anymore. */
#define Blame( ... ) \
	CONS_Printf("\x85" __VA_ARGS__)

static void MasterServer_Debug_OnChange (void);

consvar_t cv_masterserver_timeout = CVAR_INIT
(
		"masterserver_timeout", "5", CV_SAVE, CV_Unsigned,
		NULL
);

consvar_t cv_masterserver_debug = CVAR_INIT
(
	"masterserver_debug", "Off", CV_SAVE|CV_CALL, CV_OnOff,
	MasterServer_Debug_OnChange
);

consvar_t cv_masterserver_token = CVAR_INIT
(
		"masterserver_token", "", CV_SAVE, NULL,
		NULL
);

#ifdef MASTERSERVER

static int hms_started;

static char *hms_api;
#ifdef HAVE_THREADS
static I_mutex hms_api_mutex;
#endif

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
	size_t blocks;

	(void)_1;

	buffer = userdata;

	if (n >= (size_t)( buffer->end - buffer->needle ))
	{
		/* resize to next multiple of buffer size */
		blocks = ( n / DEFAULT_BUFFER_SIZE + 1 );
		buffer->end += ( blocks * DEFAULT_BUFFER_SIZE );

		buffer->buffer = realloc(buffer->buffer, buffer->end);
	}

	memcpy(&buffer->buffer[buffer->needle], s, n);
	buffer->needle += n;

	return n;
}

static struct HMS_buffer *
HMS_connect (const char *format, ...)
{
	va_list ap;
	CURL *curl;
	char *url;
	char *quack_token;
	size_t seek;
	size_t token_length;
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

	if (cv_masterserver_token.string[0])
	{
		quack_token = curl_easy_escape(curl, cv_masterserver_token.string, 0);
		token_length = ( sizeof "?token="-1 )+ strlen(quack_token);
	}
	else
	{
		quack_token = NULL;
		token_length = 0;
	}

#ifdef HAVE_THREADS
	I_lock_mutex(&hms_api_mutex);
#endif

	seek = strlen(hms_api) + 1;/* + '/' */

	va_start (ap, format);
	url = malloc(seek + vsnprintf(0, 0, format, ap) + token_length + 1);
	va_end (ap);

	sprintf(url, "%s/", hms_api);

#ifdef HAVE_THREADS
	I_unlock_mutex(hms_api_mutex);
#endif

	va_start (ap, format);
	seek += vsprintf(&url[seek], format, ap);
	va_end (ap);

	if (quack_token)
		sprintf(&url[seek], "?token=%s", quack_token);

	CONS_Printf("HMS: connecting '%s'...\n", url);

	buffer = malloc(sizeof *buffer);
	buffer->curl = curl;
	buffer->end = DEFAULT_BUFFER_SIZE;
	buffer->buffer = malloc(buffer->end);
	buffer->needle = 0;

	if (cv_masterserver_debug.value)
	{
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
		curl_easy_setopt(curl, CURLOPT_STDERR, logstream);
	}

	if (M_CheckParm("-bindaddr") && M_IsNextParm())
	{
		curl_easy_setopt(curl, CURLOPT_INTERFACE, M_GetNextParm());
	}

	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);

	curl_easy_setopt(curl, CURLOPT_TIMEOUT, cv_masterserver_timeout.value);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, HMS_on_read);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, buffer);

	curl_free(quack_token);
	free(url);

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
	free(buffer->buffer);
	free(buffer);
}

int
HMS_fetch_rooms (int joining, int query_id)
{
	struct HMS_buffer *hms;
	int ok;

	int doing_shit;

	char *id;
	char *title;
	char *room_motd;

	int id_no;

	char *p;
	char *end;

	int i;

	(void)query_id;

	hms = HMS_connect("rooms");

	if (! hms)
		return 0;

	if (HMS_do(hms))
	{
		doing_shit = 1;

		p = hms->buffer;

		for (i = 0; i < NUM_LIST_ROOMS && ( end = strstr(p, "\n\n\n") );)
		{
			*end = '\0';

			id    = strtok(p, "\n");
			title = strtok(0, "\n");
			room_motd = strtok(0, "");

			if (id && title && room_motd)
			{
				id_no = atoi(id);

				/*
				Don't show the 'All' room if hosting. And it's a hack like this
				because I'm way too lazy to add another feature to the MS.
				*/
				if (joining || id_no != 0)
				{
#ifdef HAVE_THREADS
					I_lock_mutex(&ms_QueryId_mutex);
					{
						if (query_id != ms_QueryId)
							doing_shit = 0;
					}
					I_unlock_mutex(ms_QueryId_mutex);

					if (! doing_shit)
						break;
#endif

					room_list[i].header.buffer[0] = 1;

					room_list[i].id = id_no;
					strlcpy(room_list[i].name, title, sizeof room_list[i].name);
					strlcpy(room_list[i].motd, room_motd, sizeof room_list[i].motd);

					i++;
				}

				p = ( end + 3 );/* skip the three linefeeds */
			}
			else
				break;
		}

		if (doing_shit)
			room_list[i].header.buffer[0] = 0;

		ok = 1;

		if (doing_shit)
		{
#ifdef HAVE_THREADS
			I_lock_mutex(&m_menu_mutex);
#endif
			{
				for (i = 0; room_list[i].header.buffer[0]; i++)
				{
					if(*room_list[i].name != '\0')
					{
						MP_RoomMenu[i+1].text = room_list[i].name;
						roomIds[i] = room_list[i].id;
						MP_RoomMenu[i+1].status = IT_STRING|IT_CALL;
					}
				}
			}
#ifdef HAVE_THREADS
			I_unlock_mutex(m_menu_mutex);
#endif
		}
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

	if (! hms)
		return 0;

	title = curl_easy_escape(hms->curl, cv_servername.string, 0);

	snprintf(post, sizeof post,
			"port=%d&"
			"title=%s&"
			"version=%s",

			current_port,

			title,

			SRB2VERSION
	);

	curl_free(title);

	curl_easy_setopt(hms->curl, CURLOPT_POSTFIELDS, post);

	ok = HMS_do(hms);

	if (ok)
	{
		hms_server_token = strdup(strtok(hms->buffer, "\n"));
	}

	HMS_end(hms);

	return ok;
}

int
HMS_unlist (void)
{
	struct HMS_buffer *hms;
	int ok;

	hms = HMS_connect("servers/%s/unlist", hms_server_token);

	if (! hms)
		return 0;

	curl_easy_setopt(hms->curl, CURLOPT_CUSTOMREQUEST, "POST");

	ok = HMS_do(hms);
	HMS_end(hms);

	free(hms_server_token);

	return ok;
}

int
HMS_update (void)
{
	struct HMS_buffer *hms;
	int ok;

	char post[256];

	char *title;

	hms = HMS_connect("servers/%s/update", hms_server_token);

	if (! hms)
		return 0;

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

	char *list;
	char *p;

	hms = HMS_connect("servers");

	if (! hms)
		return;

	if (HMS_do(hms))
	{
		list = curl_easy_unescape(hms->curl, hms->buffer, 0, NULL);

		p = strtok(list, "\n");

		while (p != NULL)
		{
			CONS_Printf("\x80%s\n", p);
			p = strtok(NULL, "\n");
		}

		curl_free(list);
	}

	HMS_end(hms);
}

msg_server_t *
HMS_fetch_servers (msg_server_t *list, int room_number, int query_id)
{
	struct HMS_buffer *hms;

	int doing_shit;

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

	(void)query_id;

	if (room_number > 0)
	{
		hms = HMS_connect("rooms/%d/servers", room_number);
	}
	else
		hms = HMS_connect("servers");

	if (! hms)
		return NULL;

	if (HMS_do(hms))
	{
		doing_shit = 1;

		snprintf(local_version, sizeof local_version,
				"%s",
				SRB2VERSION
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
#ifdef HAVE_THREADS
					I_lock_mutex(&ms_QueryId_mutex);
					{
						if (query_id != ms_QueryId)
							doing_shit = 0;
					}
					I_unlock_mutex(ms_QueryId_mutex);

					if (! doing_shit)
						break;
#endif

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

			if (! doing_shit)
				break;

			p = ( section_end + 2 );
		}
		while (section_end) ;

		if (doing_shit)
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

	if (! hms)
		return 0;

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

void
HMS_set_api (char *api)
{
#ifdef HAVE_THREADS
	I_lock_mutex(&hms_api_mutex);
#endif
	{
		free(hms_api);
		hms_api = api;
	}
#ifdef HAVE_THREADS
	I_unlock_mutex(hms_api_mutex);
#endif
}

#endif/*MASTERSERVER*/

static void
MasterServer_Debug_OnChange (void)
{
#ifdef MASTERSERVER
	/* TODO: change to 'latest-log.txt' for log files revision. */
	if (cv_masterserver_debug.value)
		CONS_Printf("Master server debug messages will appear in log.txt\n");
#endif
}
