/*
Copyright 2020 James R.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#define strcasecmp _stricmp
#else
#include <strings.h>
#endif

int
main (int ac, char **av)
{
	char line[256];
	char buf[256];
	char *var;
	char *val;
	char *p;
	int n;
	(void)ac;
	(void)av;
	fputs(
			"Copyright 2020 James R.\n"
			"All rights reserved.\n"
			"\n"
			"Usage: musicdef-2.2.1 < old-MUSICDEF > new-MUSICDEF\n"
			"\n"
			,stderr);
	while (fgets(line, sizeof line, stdin))
	{
		memcpy(buf, line, sizeof buf);
		if (( var = strtok(buf, " =") ))
		{
			if (!(
						strcasecmp(var, "TITLE") &&
						strcasecmp(var, "ALTTITLE") &&
						strcasecmp(var, "AUTHORS")
			)){
				if (( val = strtok(0, "") ))
				{
					for (p = val; ( p = strchr(p, '_') ); )
					{
						n = strspn(p, "_");
						memset(p, ' ', n);
						p += n;
					}
					printf("%s %s", var, val);
					continue;
				}
			}
		}
		fputs(line, stdout);
	}
	return 0;
}
