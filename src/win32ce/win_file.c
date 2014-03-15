#include <stdlib.h>
#include <windows.h>
#include "win_file.h"


int FileAccess(LPCTSTR FileName, DWORD mode)
{
	HANDLE hFile;

	hFile = CreateFile( FileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );

	if (hFile == INVALID_HANDLE_VALUE)
		return -1;
	else
	{
		FileClose(hFile);
		return 0;
	}
}

HANDLE FileCreate(LPCTSTR FileName)
{
	HANDLE hFile;

	if (FileAccess( FileName, 0) == 0)
		hFile = CreateFile( FileName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, TRUNCATE_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	else
		hFile = CreateFile( FileName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );

	return hFile;
}

void FileClose(HANDLE hFile)
{
	CloseHandle(hFile);
}

DWORD FileLength(HANDLE hFile)
{
	return GetFileSize(hFile, NULL);
}

HANDLE FileOpen(LPCTSTR FileName)
{
	HANDLE hFile;

	hFile = CreateFile( FileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );

	return hFile;
}

HANDLE FileAppend(LPCTSTR FileName)
{
	HANDLE hFile;

	hFile = CreateFile( FileName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	FileSeek(hFile, 0, FILE_END );

	return hFile;
}

DWORD FileRead(HANDLE hFile, LPVOID data, DWORD size)
{
	DWORD readin = 0;

	ReadFile(hFile, data, size, &readin, NULL);

	return readin;
}

DWORD FileSeek(HANDLE hFile, LONG distance, DWORD method)
{
	DWORD position;

	position = SetFilePointer(hFile, distance, NULL, method);

	return position;
}

DWORD FileWrite(HANDLE hFile, LPCVOID data, DWORD size)
{
	DWORD written = 0;

	WriteFile(hFile, data, size, &written, NULL);

	return written;
}

//These functions are provided as CRT replacements. (missing from WinCE)

int access(char* file,int type)
{
	FILE* file_access = 0;

	file_access = fopen(file,"rb");

	if(file_access)
	{
		fclose(file_access);
		return 0;
	}

	return -1;
}

unsigned int file_len(char* file)
{
	FILE* file_access;
	unsigned int len = 0;

	file_access = fopen(file,"rb");

	if(!file_access)
		return 0;

	fseek(file_access,0,SEEK_END);

	len = ftell(file_access);

	fclose(file_access);

	return len;
}
