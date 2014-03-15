//It's 4am and im writing replacement string functions....

#include <stdlib.h>
#include <Windows.h>
#include "wince_stuff.h"

char* _strlwr( char *string )
{
	int i;

	if(!string)
		return NULL;

	for(i=0 ; i < strlen(string); i++)
	{
		if((*(string + i) >= 65) && (*(string + i) <= 90))
			*(string+i) = *(string+i) + 32;
	}

	return string;
}

int _strnicmp(const char *first,const char *last, size_t count )
{
	int f, l;

	do
	{
		if ( ((f = (unsigned char)(*(first++))) >= 'A') && (f <= 'Z') )
			f -= 'A' - 'a';

		if ( ((l = (unsigned char)(*(last++))) >= 'A') && (l <= 'Z') )
			l -= 'A' - 'a';

	} while ( --count && f && (f == l) );

	return ( f - l );
}


int _stricmp( const char *dst, const char *src )
{
	int f, l;

	do
	{
		if ( ((f = (unsigned char)(*(dst++))) >= 'A') && (f <= 'Z') )
			f -= 'A' - 'a';

		if ( ((l = (unsigned char)(*(src++))) >= 'A') && (l <= 'Z') )
			l -= 'A' - 'a';

	} while ( f && (f == l) );

	return(f - l);
}


char* _strupr( char *string )
{
	int i;

	if(!string)
		return NULL;

	for(i=0 ; i < strlen(string); i++)
	{
		if((*(string + i) >= 97) && (*(string + i) <= 122))
			*(string + i) = *(string + i) - 32;
	}

	return string;
}




int isprint( int c )
{
	if(c <= 31)
		return FALSE;

	return TRUE;
}



char* _strdup (const char * string)
{
	char *memory = NULL;

	if (!string)
		return(NULL);

	if (memory = malloc(strlen(string) + 1))
		return(strcpy(memory,string));

	return(NULL);
}



char* strrchr (const char * string,int ch)
{
	char *start = (char *)string;

	while (*string++)                       /* find end of string */
		;
	/* search towards front */
	while (--string != start && *string != (char)ch)
		;

	if (*string == (char)ch)                /* char found ? */
		return( (char *)string );

	return(NULL);
}

char* GetMyCWD(void)
{
	TCHAR fn[MAX_PATH];
	char* my_cwd,*p;

	GetModuleFileName(NULL,fn,MAX_PATH);

	my_cwd = (char*)malloc(MAX_PATH*sizeof(char));

	WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR | WC_SEPCHARS, fn, -1, my_cwd, MAX_PATH, NULL, NULL);
	p = strrchr(my_cwd,'\\');

	if(p)
		*(p+1) = '\0';

	return my_cwd;
}
