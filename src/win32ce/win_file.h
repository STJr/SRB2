#ifndef FILE_H
#define FILE_H

// New File I/O functions

int		FileAccess(LPCTSTR, DWORD);
HANDLE	FileAppend(LPCTSTR FileName);
void	FileClose(HANDLE);
HANDLE	FileCreate(LPCTSTR);
DWORD	FileLength(HANDLE);
HANDLE	FileOpen(LPCTSTR);
DWORD	FileRead(HANDLE, LPCVOID, DWORD);
DWORD	FileSeek(HANDLE hFile, LONG distance, DWORD method);
DWORD	FileWrite(HANDLE, LPCVOID, DWORD);

int access(char* file,int type);
unsigned int file_len(char* file);

#endif