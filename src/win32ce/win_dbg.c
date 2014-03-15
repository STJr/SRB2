// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 1998-2000 by DooM Legacy Team.
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
//-----------------------------------------------------------------------------
/// \file
/// \brief Sources from GameDeveloper magazine article, January 1998, by Bruce Dawson.
///	this source file contains the exception handler for recording error
///	information after crashes.

#include <tchar.h>
#include "win_main.h"
#include "../doomdef.h" //just for VERSION
#include "win_dbg.h"
#include "../m_argv.h" //print the parameter in the log


#define NumCodeBytes    16          // Number of code bytes to record.
#define MaxStackDump    2048    // Maximum number of DWORDS in stack dumps.
#define StackColumns    8               // Number of columns in stack dump.

#define ONEK                    1024
#define SIXTYFOURK              (64*ONEK)
#define ONEM                    (ONEK*ONEK)
#define ONEG                    (ONEK*ONEK*ONEK)


// --------------------------------------------------------------------------
// return a description for an ExceptionCode
// --------------------------------------------------------------------------
static LPCSTR GetExceptionDescription (DWORD ExceptionCode)
{
	unsigned int i;

	struct ExceptionNames
	{
		DWORD   ExceptionCode;
		LPCSTR  ExceptionName;
	};

	struct ExceptionNames ExceptionMap[] =
	{
		{EXCEPTION_ACCESS_VIOLATION, "an Access Violation"},
		{EXCEPTION_ARRAY_BOUNDS_EXCEEDED, "a Array Bounds Exceeded"},
		{EXCEPTION_BREAKPOINT, "a Breakpoint"},
		{EXCEPTION_DATATYPE_MISALIGNMENT, "a Datatype Misalignment"},
		{EXCEPTION_FLT_DENORMAL_OPERAND, "a Float Denormal Operand"},
		{EXCEPTION_FLT_DIVIDE_BY_ZERO, "a Float Divide By Zero"},
		{EXCEPTION_FLT_INEXACT_RESULT, "a Float Inexact Result"},
		{EXCEPTION_FLT_INVALID_OPERATION, "a Float Invalid Operation"},
		{EXCEPTION_FLT_OVERFLOW, "a Float Overflow"},
		{EXCEPTION_FLT_STACK_CHECK, "a Float Stack Check"},
		{EXCEPTION_FLT_UNDERFLOW, "a Float Underflow"},
		{EXCEPTION_ILLEGAL_INSTRUCTION, "an Illegal Instruction"},
		{EXCEPTION_IN_PAGE_ERROR, "an In Page Error"},
		{EXCEPTION_INT_DIVIDE_BY_ZERO, "an Integer Divide By Zero"},
		{EXCEPTION_INT_OVERFLOW, "an Integer Overflow"},
		{EXCEPTION_INVALID_DISPOSITION, "an Invalid Disposition"},
		{EXCEPTION_NONCONTINUABLE_EXCEPTION, "Noncontinuable Exception"},
		{EXCEPTION_PRIV_INSTRUCTION, "a Privileged Instruction"},
		{EXCEPTION_SINGLE_STEP, "a Single Step"},
		{EXCEPTION_STACK_OVERFLOW, "a Stack Overflow"},
		{0x40010005, "a Control-C"},
		{0x40010008, "a Control-Break"},
		{0xc0000006, "an In Page Error"},
		{0xc0000017, "a No Memory"},
		{0xc000001d, "an Illegal Instruction"},
		{0xc0000025, "a Noncontinuable Exception"},
		{0xc0000142, "a DLL Initialization Failed"},
		{0xe06d7363, "a Microsoft C++ Exception"},
	};

	for (i = 0; i < (sizeof (ExceptionMap) / sizeof (ExceptionMap[0])); i++)
		if (ExceptionCode == ExceptionMap[i].ExceptionCode)
			return ExceptionMap[i].ExceptionName;

	return "Unknown exception type";
}


// --------------------------------------------------------------------------
// Directly output a formatted string to the errorlog file, using win32 funcs
// --------------------------------------------------------------------------
static VOID FPrintf (HANDLE fileHandle, LPCSTR lpFmt, ...)
{
	CHAR    str[1999];
	va_list arglist;
	DWORD   bytesWritten;

	va_start (arglist, lpFmt);
	vsprintf (str, lpFmt, arglist);
	va_end   (arglist);

	WriteFile (fileHandle, str, (DWORD)strlen(str), &bytesWritten, NULL);
}

// --------------------------------------------------------------------------
// Print the specified FILETIME to output in a human readable format,
// without using the C run time.
// --------------------------------------------------------------------------
static VOID PrintTime (LPSTR output, FILETIME TimeToPrint)
{
	WORD Date, Time;
	if (FileTimeToLocalFileTime (&TimeToPrint, &TimeToPrint) &&
	    FileTimeToDosDateTime (&TimeToPrint, &Date, &Time))
	{
		// What a silly way to print out the file date/time.
		wsprintfA(output, "%d/%d/%d %02d:%02d:%02d",
		   (Date / 32) & 15, Date & 31, (Date / 512) + 1980,
		   (Time / 2048), (Time / 32) & 63, (Time & 31) * 2);
	}
	else
		output[0] = 0;
}


static LPTSTR GetFilePart(LPTSTR source)
{
	LPTSTR result = _tcsrchr(source, '\\');
	if (result)
		result++;
	else
		result = source;
	return result;
}

// --------------------------------------------------------------------------
// Print information about a code module (DLL or EXE) such as its size,
// location, time stamp, etc.
// --------------------------------------------------------------------------
static VOID ShowModuleInfo(HANDLE LogFile, HMODULE ModuleHandle)
{
	CHAR ModName[MAX_PATH];
	IMAGE_DOS_HEADER *DosHeader;
	IMAGE_NT_HEADERS *NTHeader;
	HANDLE ModuleFile;
	CHAR TimeBuffer[100] = "";
	DWORD FileSize = 0;
#ifdef NO_SEH_MINGW
	__try1(EXCEPTION_EXECUTE_HANDLER)
#else
	__try
#endif
	{
		if (GetModuleFileNameA(ModuleHandle, ModName, sizeof (ModName)) > 0)
		{
			// If GetModuleFileName returns greater than zero then this must
			// be a valid code module address. Therefore we can try to walk
			// our way through its structures to find the link time stamp.
			DosHeader = (IMAGE_DOS_HEADER*)ModuleHandle;
			if (IMAGE_DOS_SIGNATURE != DosHeader->e_magic)
				return;
			NTHeader = (IMAGE_NT_HEADERS*)((char *)DosHeader
				+ DosHeader->e_lfanew);
			if (IMAGE_NT_SIGNATURE != NTHeader->Signature)
				return;
			// Open the code module file so that we can get its file date
			// and size.
			ModuleFile = CreateFileA(ModName, GENERIC_READ,
				FILE_SHARE_READ, 0, OPEN_EXISTING,
				FILE_ATTRIBUTE_NORMAL, 0);
			if (ModuleFile != INVALID_HANDLE_VALUE)
			{
				FILETIME        LastWriteTime;
				FileSize = GetFileSize(ModuleFile, 0);
				if (GetFileTime(ModuleFile, 0, 0, &LastWriteTime))
				{
					wsprintfA(TimeBuffer, " - file date is ");
					PrintTime(TimeBuffer + strlen(TimeBuffer), LastWriteTime);
				}
				CloseHandle(ModuleFile);
			}
			FPrintf (LogFile, "%s, loaded at 0x%08x - %d bytes - %08x%s\r\n",
				ModName, ModuleHandle, FileSize,
				NTHeader->FileHeader.TimeDateStamp, TimeBuffer);
		}
	}
	// Handle any exceptions by continuing from this point.
#ifdef NO_SEH_MINGW
	__except1
#else
	__except(EXCEPTION_EXECUTE_HANDLER)
#endif
	{
	}
}

// --------------------------------------------------------------------------
// Scan memory looking for code modules (DLLs or EXEs). VirtualQuery is used
// to find all the blocks of address space that were reserved or committed,
// and ShowModuleInfo will display module information if they are code
// modules.
// --------------------------------------------------------------------------
static VOID RecordModuleList(HANDLE LogFile)
{
	SYSTEM_INFO     SystemInfo;
	size_t PageSize;
	size_t NumPages;
	size_t pageNum = 0;
	LPVOID LastAllocationBase = 0;

	FPrintf (LogFile, "\r\n"
		"\tModule list: names, addresses, sizes, time stamps "
		"and file times:\r\n");

	// Set NumPages to the number of pages in the 4GByte address space,
	// while being careful to avoid overflowing ints.
	GetSystemInfo(&SystemInfo);
	PageSize = SystemInfo.dwPageSize;
	NumPages = 4 * (unsigned int)(ONEG / PageSize);
	while (pageNum < NumPages)
	{
		MEMORY_BASIC_INFORMATION        MemInfo;
		if (VirtualQuery((LPVOID)(pageNum * PageSize), &MemInfo,
			sizeof (MemInfo)))
		{
			if (MemInfo.RegionSize > 0)
			{
				// Adjust the page number to skip over this block of memory.
				pageNum += MemInfo.RegionSize / PageSize;
				if (MemInfo.State == MEM_COMMIT && MemInfo.AllocationBase >
					LastAllocationBase)
				{
					// Look for new blocks of committed memory, and try
					// recording their module names - this will fail
					// gracefully if they aren't code modules.
					LastAllocationBase = MemInfo.AllocationBase;
					ShowModuleInfo(LogFile, (HMODULE)LastAllocationBase);
				}
			}
			else
				pageNum += SIXTYFOURK / PageSize;
		}
		else
			pageNum += SIXTYFOURK / PageSize;
		// If VirtualQuery fails we advance by 64K because that is the
		// granularity of address space doled out by VirtualAlloc().
	}
}


// --------------------------------------------------------------------------
// Record information about the user's system, such as processor type, amount
// of memory, etc.
// --------------------------------------------------------------------------
static VOID RecordSystemInformation(HANDLE fileHandle)
{
	FILETIME     CurrentTime;
	CHAR         TimeBuffer[100];
	CHAR         ModuleName[MAX_PATH];
	CHAR         UserName[200];
	DWORD        UserNameSize;
	SYSTEM_INFO  SystemInfo;
	MEMORYSTATUS MemInfo;

	GetSystemTimeAsFileTime (&CurrentTime);
	PrintTime (TimeBuffer, CurrentTime);
	FPrintf(fileHandle, "Error occurred at %s.\r\n", TimeBuffer);

	if (GetModuleFileNameA(NULL, ModuleName, sizeof (ModuleName)) <= 0)
		strcpy (ModuleName, "Unknown");
	UserNameSize = sizeof (UserName);
	if (!GetUserNameA(UserName, &UserNameSize))
		strcpy (UserName, "Unknown");
	FPrintf(fileHandle, "%s, run by %s.\r\n", ModuleName, UserName);

	GetSystemInfo (&SystemInfo);
	FPrintf (fileHandle, "%d processor(s), type %d %d.%d.\r\n"
	         "Program Memory from 0x%p to 0x%p\r\n",
	         SystemInfo.dwNumberOfProcessors,
	         SystemInfo.dwProcessorType,
	         SystemInfo.wProcessorLevel,
	         SystemInfo.wProcessorRevision,
	         SystemInfo.lpMinimumApplicationAddress,
	         SystemInfo.lpMaximumApplicationAddress);

	MemInfo.dwLength = sizeof (MemInfo);
	GlobalMemoryStatus(&MemInfo);
	// Print out the amount of physical memory, rounded up.
	FPrintf(fileHandle, "%d MBytes physical memory.\r\n", (MemInfo.dwTotalPhys +
	        ONEM - 1) / ONEM);
}

// --------------------------------------------------------------------------
// What we do here is trivial : open a file, write out the register information
// from the PEXCEPTION_POINTERS structure, then return EXCEPTION_CONTINUE_SEARCH
// whose magic value tells Win32 to proceed with its normal error handling
// mechanism. This is important : an error dialog will popup if possible and
// the debugger will hopefully coexist peacefully with the structured exception
// handler.
// --------------------------------------------------------------------------
int __cdecl RecordExceptionInfo (PEXCEPTION_POINTERS data/*, LPCSTR Message, LPSTR lpCmdLine*/)
{
	PEXCEPTION_RECORD   Exception;
	PCONTEXT            Context;
	TCHAR               ModuleName[MAX_PATH];
	TCHAR               FileName[MAX_PATH] = TEXT("Unknown");
	LPTSTR              FilePart, lastperiod;
	TCHAR               CrashModulePathName[MAX_PATH];
	LPCTSTR             CrashModuleFileName = TEXT("Unknown");
	MEMORY_BASIC_INFORMATION    MemInfo;
	static int          BeenHere = false;
	HANDLE              fileHandle;
	UINT8              *code;
	int                 codebyte,i;

	if (data)
	{
		Exception = data->ExceptionRecord;
		Context = data->ContextRecord;
	}
	else
	{
		return EXCEPTION_CONTINUE_SEARCH;
	}

	if (BeenHere)       // Going recursive! That must mean this routine crashed!
		return EXCEPTION_CONTINUE_SEARCH;
	BeenHere = true;

	// Create a filename to record the error information to.
	// Store it in the executable directory.
	if (GetModuleFileName(NULL, ModuleName, sizeof (ModuleName)) <= 0)
		ModuleName[0] = 0;
	FilePart = GetFilePart(ModuleName);

	// Extract the file name portion and remove it's file extension. We'll
	// use that name shortly.
	lstrcpy (FileName, FilePart);
	lastperiod = _tcsrchr (FileName, '.');
	if (lastperiod)
		lastperiod[0] = 0;
	// Replace the executable filename with our error log file name.
	lstrcpy (FilePart, TEXT("errorlog.txt"));
	fileHandle = CreateFile (ModuleName, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, NULL);
	if (fileHandle == INVALID_HANDLE_VALUE)
	{
		OutputDebugString (TEXT("Error creating exception report"));
		return EXCEPTION_CONTINUE_SEARCH;
	}

	// Append to the error log.
	SetFilePointer (fileHandle, 0, 0, FILE_END);

	// Print out some blank lines to separate this error log from any previous ones.
	FPrintf (fileHandle, "Email Sonic Team Junior so we can fix the bugs\r\n"); // Tails
	FPrintf (fileHandle, "Make sure you tell us what you were doing to cause the crash, and if possible, record a demo!\r\n"); // Tails
	FPrintf (fileHandle, "\r\n\r\n\r\n\r\n");
	FPrintf (fileHandle, "SRB2 %s -ERROR LOG-\r\n\r\n", VERSIONSTRING);
	FPrintf (fileHandle, "\r\n");
	// VirtualQuery can be used to get the allocation base associated with a
	// code address, which is the same as the ModuleHandle. This can be used
	// to get the filename of the module that the crash happened in.
	if (VirtualQuery ((LPVOID)(size_t)Context->Eip, &MemInfo, sizeof (MemInfo)) &&
	     GetModuleFileName ((HMODULE)MemInfo.AllocationBase,
	                        CrashModulePathName,
	                        sizeof (CrashModulePathName)) > 0)
		CrashModuleFileName = GetFilePart(CrashModulePathName);

	// Print out the beginning of the error log in a Win95 error window
	// compatible format.
	FPrintf (fileHandle, "%s caused an %s in module %s at %04x:%08x.\r\n",
		FileName, GetExceptionDescription(Exception->ExceptionCode),
		CrashModuleFileName, Context->SegCs, Context->Eip);
	 //if (&Message = Null)
		FPrintf (fileHandle, "Exception handler called in %s.\r\n", "main thread");
	 //else
		//FPrintf (fileHandle, "Exception handler called in %s.\r\n", Message);

	RecordSystemInformation (fileHandle);

	// If the exception was an access violation, print out some additional
	// information, to the error log and the debugger.
	if (Exception->ExceptionCode == STATUS_ACCESS_VIOLATION &&
		Exception->NumberParameters >= 2)
	{
		TCHAR DebugMessage[1000];
		LPCTSTR readwrite = TEXT("Read from");
		if (Exception->ExceptionInformation[0])
			readwrite = TEXT("Write to");
		wsprintf(DebugMessage, TEXT("%s location %08x caused an access violation.\r\n"),
			readwrite, Exception->ExceptionInformation[1]);
#ifdef  _DEBUG
		// The VisualC++ debugger doesn't actually tell you whether a read
		// or a write caused the access violation, nor does it tell what
		// address was being read or written. So I fixed that.
		OutputDebugString(TEXT("Exception handler: "));
		OutputDebugString(DebugMessage);
#endif
		FPrintf(fileHandle, "%s", DebugMessage);
	}

	FPrintf(fileHandle, "\r\n");

	// Print out the register values in a Win95 error window compatible format.
	if ((Context->ContextFlags & CONTEXT_FULL) == CONTEXT_FULL)
	{
		FPrintf (fileHandle, "Registers:\r\n");
		FPrintf (fileHandle, "EAX=%.8lx CS=%.4x EIP=%.8lx EFLGS=%.8lx\r\n",
			Context->Eax,Context->SegCs,Context->Eip,Context->EFlags);
		FPrintf (fileHandle, "EBX=%.8lx SS=%.4x ESP=%.8lx EBP=%.8lx\r\n",
			Context->Ebx,Context->SegSs,Context->Esp,Context->Ebp);
		FPrintf (fileHandle, "ECX=%.8lx DS=%.4x ESI=%.8lx FS=%.4x\r\n",
			Context->Ecx,Context->SegDs,Context->Esi,Context->SegFs);
		FPrintf (fileHandle, "EDX=%.8lx ES=%.4x EDI=%.8lx GS=%.4x\r\n",
			Context->Edx,Context->SegEs,Context->Edi,Context->SegGs);
	}

	// moved down because it was causing the printout to stop
	FPrintf (fileHandle, "Command Line parameters: ");
	for (i = 1;i < myargc;i++)
		FPrintf (fileHandle, "%s ", myargv[i]);

	FPrintf (fileHandle, "Bytes at CS : EIP:\r\n");

	// Print out the bytes of code at the instruction pointer. Since the
	// crash may have been caused by an instruction pointer that was bad,
	// this code needs to be wrapped in an exception handler, in case there
	// is no memory to read. If the dereferencing of code[] fails, the
	// exception handler will print '??'.
	code = (UINT8 *)(size_t)Context->Eip;
	for (codebyte = 0; codebyte < NumCodeBytes; codebyte++)
	{
#ifdef NO_SEH_MINGW
		__try1(EXCEPTION_EXECUTE_HANDLER)
#else
		__try
#endif
		{
			FPrintf (fileHandle, "%02x ", code[codebyte]);
		}
#ifdef NO_SEH_MINGW
		__except1
#else
		__except(EXCEPTION_EXECUTE_HANDLER)
#endif
		{
			FPrintf (fileHandle, "?? ");
		}
	}

	// Time to print part or all of the stack to the error log. This allows
	// us to figure out the call stack, parameters, local variables, etc.
	FPrintf (fileHandle, "\r\n"
		"Stack dump:\r\n");
#ifdef NO_SEH_MINGW
	__try1(EXCEPTION_EXECUTE_HANDLER)
#else
	__try
#endif
	{
		// Esp contains the bottom of the stack, or at least the bottom of
		// the currently used area.
		DWORD*    pStack = (DWORD *)(size_t)Context->Esp;
		DWORD*    pStackTop = NULL;
		size_t    Count = 0;
		TCHAR     buffer[1000] = TEXT("");
		const int safetyzone = 50;
		LPTSTR    nearend = buffer + sizeof (buffer) - safetyzone*sizeof (TCHAR);
		LPTSTR    output = buffer;
		const void *Suffix;

		// Load the top (highest address) of the stack from the
		// thread information block. It will be found there in
		// Win9x and Windows NT.
#ifdef __GNUC__
		__asm__("movl %%fs : 4, %%eax": "=a"(pStackTop));
#else
		__asm
		{
			mov eax, fs:[4]
			mov pStackTop, eax
		}
#endif
		if (pStackTop == NULL)
			goto StackSkip;
		else if (pStackTop > pStack + MaxStackDump)
			pStackTop = pStack + MaxStackDump;
		// Too many calls to WriteFile can take a long time, causing
		// confusing delays when programs crash. Therefore I implemented
		// simple buffering for the stack dumping code instead of calling
		// FPrintf directly.
		while (pStack + 1 <= pStackTop)
		{
			if ((Count % StackColumns) == 0)
				output += wsprintf(output, TEXT("%08x: "), pStack);
			if ((++Count % StackColumns) == 0 || pStack + 2 > pStackTop)
				Suffix = TEXT("\r\n");
			else
				Suffix = TEXT(" ");
			output += wsprintf(output, TEXT("%08x%s"), *pStack, Suffix);
			pStack++;
			// Check for when the buffer is almost full, and flush it to disk.
			if (output > nearend)
			{
				FPrintf (fileHandle, "%s", buffer);
				buffer[0] = 0;
				output = buffer;
			}
		}
		// Print out any final characters from the cache.
		StackSkip:
		FPrintf (fileHandle, "%s", buffer);
	}
#ifdef NO_SEH_MINGW
	__except1
#else
	__except(EXCEPTION_EXECUTE_HANDLER)
#endif
	{
		FPrintf(fileHandle, "Exception encountered during stack dump.\r\n");
	}

	RecordModuleList (fileHandle);

	CloseHandle (fileHandle);

	// Return the magic value which tells Win32 that this handler didn't
	// actually handle the exception - so that things will proceed as per
	// normal.
	//BP: should put message for end user to send this file to fix any bug
	return EXCEPTION_CONTINUE_SEARCH;
}

	/*
	//
	//FPrintf ("e-mail this file to legacy@newdoom.com, so that we can fix the problem.\r\n\r\n");

	FPrintf ("Exception handler called in %s.\r\n", Message);

	GetSystemTime (&systemTime);
	FPrintf ("Error occured at %02d/%02d/%04d %02d:%02d:%02d.\r\n",
			 systemTime.wMonth, systemTime.wDay, systemTime.wYear,
			 systemTime.wHour, systemTime.wMinute, systemTime.wSecond);


	FPrintf ("%s\r\n", filename);
	FPrintf ("Cmd-line: %s\r\n", lpCmdLine);

	// Nested exceptions can occur, get info for each one

	nER = 1;
	while (ER)
	{
		if (nER++>1)
			FPrintf ("Exception Record %d.\r\n", nER);

		FPrintf ("application caused an %s", GetExceptionCodeStr(Exception->ExceptionCode));

		if (Context->ContextFlags & CONTEXT_CONTROL)
			FPrintf (" at %.4x:%.8x.\r\n", Context->SegCs, Context->Eip);

		// in case of..
		if (Context->Eip != (unsigned long)Exception->ExceptionAddress)
			FPrintf ("Exception Address = %.8x\r\n", Exception->ExceptionAddress);

		if (Exception->ExceptionCode == EXCEPTION_ACCESS_VIOLATION)
		{
			FPrintf ("\r\n%s location 0x%x caused an access violation.\r\n",
					(Exception->ExceptionInformation[0] ? "Write to" : "Read from"),
					 Exception->ExceptionInformation[1]);
		}

		ER = Exception->ExceptionRecord;
	}


	if (Context->ContextFlags & CONTEXT_DEBUG_REGISTERS)
	{
		FPrintf ("\r\nDebug Registers:\r\n");
		FPrintf ("Dr0=%.8x  Dr1=%.8x  Dr2=%.8x\r\n"
				 "Dr3=%.8x  Dr6=%.8x  Dr7=%.8x\r\n",
				 Context->Dr0, Context->Dr1, Context->Dr2,
				 Context->Dr3, Context->Dr6, Context->Dr7);
	}

	if (Context->ContextFlags & CONTEXT_FLOATING_POINT)
	{
		FPrintf ("\r\nFloating Save Area:\r\n");
		FPrintf ("ControlWord  =%.8x  TagWord      =%.8x  ErrorSelector=%.8x  DataSelector =%.8x\r\n"
				 "StatusWord   =%.8x  ErrorOffset  =%.8x  DataOffset   =%.8x  Cr0NpxState  =%.8x\r\n",
				 Context->FloatSave.ControlWord, Context->FloatSave.TagWord, Context->FloatSave.ErrorSelector, Context->FloatSave.DataSelector,
				 Context->FloatSave.StatusWord, Context->FloatSave.ErrorOffset, Context->FloatSave.DataOffset, Context->FloatSave.Cr0NpxState
				);

		//BYTE    RegisterArea[SIZE_OF_80387_REGISTERS];
	}


	// in case of...
	if ((Context->ContextFlags & CONTEXT_FULL) != CONTEXT_FULL)
	{
		if (!(Context->ContextFlags & CONTEXT_SEGMENTS))
			FPrintf ("Note! GS,FS,ES,DS are unspecified\r\n");
		if (!(Context->ContextFlags & CONTEXT_INTEGER))
			FPrintf ("Note! EDI,ESI,EBX,EDX,ECX,EAX are unspecified\r\n");
		if (!(Context->ContextFlags & CONTEXT_CONTROL))
			FPrintf ("Note! EBP,CS : EIP,EFlags,SS : ESP are unspecified\r\n");
	}

	FPrintf ("\r\nBytes at CS : EIP:\r\n");
	ucptr = (UINT8 *)Context->Eip;
	for (i = 0; i < 16; i++)
		FPrintf ("%.2x ", *ucptr++);

	FPrintf ("\r\n\r\nStack dump:\r\n");
	ulptr = (unsigned long*)Context->Esp;
	for (i = 0; i < 16; i++)
		FPrintf ("%.8x ", *ulptr++);

	//FPrintf ("Bytes at CS : EIP:\r\n");
	//FPrintf ("%.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x ");

	for (i = 0; i < 16; i++)
	{
		FPrintf ("%x
	}
*/
