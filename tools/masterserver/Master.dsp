# Microsoft Developer Studio Project File - Name="MasterServer" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=MasterServer - Win32 Client Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Master.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Master.mak" CFG="MasterServer - Win32 Client Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "MasterServer - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "MasterServer - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE "MasterServer - Win32 Client Debug" (based on "Win32 (x86) Console Application")
!MESSAGE "MasterServer - Win32 Client Release" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "MasterServer - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../../bin/VC/Release/MasterServer"
# PROP Intermediate_Dir "../../objs/VC/Release/MasterServer"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W4 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "__STDC__" /D "_POSIX_" /FR /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib wsock32.lib /nologo /subsystem:console /machine:I386 /out:"../../bin/VC/Release/MasterServer.exe"

!ELSEIF  "$(CFG)" == "MasterServer - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "../../bin/VC/Debug/MasterServer"
# PROP Intermediate_Dir "../../objs/VC/Debug/MasterServer"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /W4 /Gm /GX /ZI /Od /D "_DEBUG" /D "_WIN32" /D "__DEBUG__" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "__STDC__" /D "_POSIX_" /FR /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib wsock32.lib /nologo /subsystem:console /debug /machine:I386 /out:"../../bin/VC/Debug/MasterServer.exe" /pdbtype:sept

!ELSEIF  "$(CFG)" == "MasterServer - Win32 Client Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "MasterServer___Win32_Client_Debug"
# PROP BASE Intermediate_Dir "MasterServer___Win32_Client_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "../../bin/VC/Debug/MasterClient"
# PROP Intermediate_Dir "../../objs/VC/Debug/MasterClient"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W4 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D "_WIN32" /D "__STDC__" /YX /FD /GZ /c
# ADD CPP /nologo /W4 /Gm /GX /ZI /Od /D "_DEBUG" /D "_WIN32" /D "__DEBUG__" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "__STDC__" /D "_POSIX_" /FR /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib wsock32.lib /nologo /subsystem:console /debug /machine:I386 /out:"../../bin/VC/Debug/MasterServer.exe" /pdbtype:sept
# ADD LINK32 kernel32.lib wsock32.lib /nologo /subsystem:console /debug /machine:I386 /out:"../../bin/VC/Debug/MasterClient.exe" /pdbtype:sept

!ELSEIF  "$(CFG)" == "MasterServer - Win32 Client Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "MasterServer___Win32_Client_Release"
# PROP BASE Intermediate_Dir "MasterServer___Win32_Client_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../../bin/VC/Release/MasterClient"
# PROP Intermediate_Dir "../../objs/VC/Release/MasterClient"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W4 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W4 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "__STDC__" /D "_POSIX_" /FR /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib wsock32.lib /nologo /subsystem:console /machine:I386 /out:"../../bin/VC/Release/MasterServer.exe"
# ADD LINK32 kernel32.lib wsock32.lib /nologo /subsystem:console /machine:I386 /out:"../../bin/VC/Release/MasterClient.exe"

!ENDIF 

# Begin Target

# Name "MasterServer - Win32 Release"
# Name "MasterServer - Win32 Debug"
# Name "MasterServer - Win32 Client Debug"
# Name "MasterServer - Win32 Client Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\client.cpp

!IF  "$(CFG)" == "MasterServer - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "MasterServer - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "MasterServer - Win32 Client Debug"

# PROP BASE Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "MasterServer - Win32 Client Release"

# PROP BASE Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\common.cpp
# End Source File
# Begin Source File

SOURCE=.\crypt.cpp
# End Source File
# Begin Source File

SOURCE=.\ipcs.cpp
# End Source File
# Begin Source File

SOURCE=.\server.cpp

!IF  "$(CFG)" == "MasterServer - Win32 Release"

!ELSEIF  "$(CFG)" == "MasterServer - Win32 Debug"

!ELSEIF  "$(CFG)" == "MasterServer - Win32 Client Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "MasterServer - Win32 Client Release"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\srvlist.cpp
# End Source File
# Begin Source File

SOURCE=.\stats.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\common.h
# End Source File
# Begin Source File

SOURCE=.\ipcs.h
# End Source File
# Begin Source File

SOURCE=.\srvlist.h
# End Source File
# Begin Source File

SOURCE=.\stats.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
