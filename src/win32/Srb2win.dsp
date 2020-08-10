# Microsoft Developer Studio Project File - Name="Srb2win" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=Srb2win - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE
!MESSAGE NMAKE /f "Srb2win.mak".
!MESSAGE
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE
!MESSAGE NMAKE /f "Srb2win.mak" CFG="Srb2win - Win32 Debug"
!MESSAGE
!MESSAGE Possible choices for configuration are:
!MESSAGE
!MESSAGE "Srb2win - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "Srb2win - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Srb2win - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "..\..\objs\Release"
# PROP BASE Intermediate_Dir "..\..\objs\Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\bin\VC\Release\Win32"
# PROP Intermediate_Dir "..\..\objs\VC\Release\Win32"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /G5 /W3 /GX /Zi /Ot /Og /Oi /Op /Oy /Ob1 /Gy /I "..\..\libs\libpng-src" /I "..\..\libs\zlib" /D "NDEBUG" /D "_WINDOWS" /D "USEASM" /D "HAVE_PNG" /FR /FD /GF /Gs /GF /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /o "NUL" /win32
# SUBTRACT MTL /mktyplib203
# ADD BASE RSC /l 0x40c /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo /o"..\..\objs\Release\Srb2win.bsc"
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 dxguid.lib user32.lib gdi32.lib winmm.lib advapi32.lib ws2_32.lib dinput.lib /nologo /subsystem:windows /pdb:"C:\srb2demo2\srb2.pdb" /debug /machine:I386 /out:"C:\srb2demo2\srb2win.exe"
# SUBTRACT LINK32 /profile /pdb:none /incremental:yes /nodefaultlib

!ELSEIF  "$(CFG)" == "Srb2win - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "..\..\objs\Debug"
# PROP BASE Intermediate_Dir "..\..\objs\Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\bin\VC\Debug\Win32"
# PROP Intermediate_Dir "..\..\objs\VC\Debug\Win32"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /G6 /W4 /Gm /GX /ZI /Od /Op /Oy /I "libs\libpng-src" /I "..\..\libs\libpng-src" /I "..\..\libs\zlib" /D "_DEBUG" /D "_WINDOWS" /D "USEASM" /D "HAVE_PNG" /FAcs /FR /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x40c /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 dxguid.lib user32.lib gdi32.lib winmm.lib advapi32.lib ws2_32.lib dinput.lib /nologo /subsystem:windows /profile /debug /machine:I386 /out:"C:\srb2demo2\srb2debug.exe"
# SUBTRACT LINK32 /nodefaultlib

!ENDIF

# Begin Target

# Name "Srb2win - Win32 Release"
# Name "Srb2win - Win32 Debug"
# Begin Group "Win32app"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\afxres.h
# End Source File
# Begin Source File

SOURCE=.\dx_error.c
# End Source File
# Begin Source File

SOURCE=.\dx_error.h
# End Source File
# Begin Source File

SOURCE=.\fabdxlib.c
# End Source File
# Begin Source File

SOURCE=.\fabdxlib.h
# End Source File
# Begin Source File

SOURCE=..\filesrch.c
# End Source File
# Begin Source File

SOURCE=..\filesrch.h
# End Source File
# Begin Source File

SOURCE=.\mid2strm.c
# End Source File
# Begin Source File

SOURCE=.\mid2strm.h
# End Source File
# Begin Source File

SOURCE=.\midstuff.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\Srb2win.rc

!IF  "$(CFG)" == "Srb2win - Win32 Release"

# ADD BASE RSC /l 0x40c /i "win32"
# ADD RSC /l 0x409 /i "win32"

!ELSEIF  "$(CFG)" == "Srb2win - Win32 Debug"

!ENDIF

# End Source File
# Begin Source File

SOURCE=.\win_cd.c
# End Source File
# Begin Source File

SOURCE=.\win_dbg.c
# End Source File
# Begin Source File

SOURCE=.\win_dbg.h
# End Source File
# Begin Source File

SOURCE=.\win_dll.c
# End Source File
# Begin Source File

SOURCE=.\win_dll.h
# End Source File
# Begin Source File

SOURCE=.\win_main.c
# End Source File
# Begin Source File

SOURCE=.\win_main.h
# End Source File
# Begin Source File

SOURCE=.\win_net.c
# End Source File
# Begin Source File

SOURCE=.\win_snd.c
# End Source File
# Begin Source File

SOURCE=.\win_sys.c
# End Source File
# Begin Source File

SOURCE=.\win_vid.c
# End Source File
# End Group
# Begin Group "A_Asm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\p5prof.h
# End Source File
# Begin Source File

SOURCE=..\tmap.nas

!IF  "$(CFG)" == "Srb2win - Win32 Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build - Compiling $(InputName).nas with NASM...
IntDir=.\..\..\objs\VC\Release\Win32
InputPath=..\tmap.nas
InputName=tmap

"$(IntDir)/$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -g -o $(IntDir)/$(InputName).obj -f win32 $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "Srb2win - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build - Compiling $(InputName).nas with NASM...
IntDir=.\..\..\objs\VC\Debug\Win32
InputPath=..\tmap.nas
InputName=tmap

"$(IntDir)/$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -g -o $(IntDir)/$(InputName).obj -f win32 $(InputPath)

# End Custom Build

!ENDIF

# End Source File
# Begin Source File

SOURCE=..\tmap_mmx.nas

!IF  "$(CFG)" == "Srb2win - Win32 Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build - Compiling $(InputName).nas with NASM...
IntDir=.\..\..\objs\VC\Release\Win32
InputPath=..\tmap_mmx.nas
InputName=tmap_mmx

"$(IntDir)/$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -g -o $(IntDir)/$(InputName).obj -f win32 $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "Srb2win - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build - Compiling $(InputName).nas with NASM...
IntDir=.\..\..\objs\VC\Debug\Win32
InputPath=..\tmap_mmx.nas
InputName=tmap_mmx

"$(IntDir)/$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -g -o $(IntDir)/$(InputName).obj -f win32 $(InputPath)

# End Custom Build

!ENDIF

# End Source File
# Begin Source File

SOURCE=..\tmap_vc.nas

!IF  "$(CFG)" == "Srb2win - Win32 Release"

# Begin Custom Build - Compiling $(InputName).nas with NASM...
IntDir=.\..\..\objs\VC\Release\Win32
InputPath=..\tmap_vc.nas
InputName=tmap_vc

"$(IntDir)/$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -g -o $(IntDir)/$(InputName).obj -f win32 $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "Srb2win - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build - Compiling $(InputName).nas with NASM...
IntDir=.\..\..\objs\VC\Debug\Win32
InputPath=..\tmap_vc.nas
InputName=tmap_vc

"$(IntDir)/$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -g -o $(IntDir)/$(InputName).obj -f win32 $(InputPath)

# End Custom Build

!ENDIF

# End Source File
# End Group
# Begin Group "D_Doom"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\comptime.c
# End Source File
# Begin Source File

SOURCE=..\d_clisrv.c
# End Source File
# Begin Source File

SOURCE=..\d_clisrv.h
# End Source File
# Begin Source File

SOURCE=..\d_event.h
# End Source File
# Begin Source File

SOURCE=..\d_main.c
# End Source File
# Begin Source File

SOURCE=..\d_main.h
# End Source File
# Begin Source File

SOURCE=..\d_net.c
# End Source File
# Begin Source File

SOURCE=..\d_net.h
# End Source File
# Begin Source File

SOURCE=..\d_netcmd.c
# End Source File
# Begin Source File

SOURCE=..\d_netcmd.h
# End Source File
# Begin Source File

SOURCE=..\d_netfil.c
# End Source File
# Begin Source File

SOURCE=..\d_netfil.h
# End Source File
# Begin Source File

SOURCE=..\d_player.h
# End Source File
# Begin Source File

SOURCE=..\d_think.h
# End Source File
# Begin Source File

SOURCE=..\d_ticcmd.h
# End Source File
# Begin Source File

SOURCE=..\dehacked.c
# End Source File
# Begin Source File

SOURCE=..\dehacked.h
# End Source File
# Begin Source File

SOURCE=..\doomdata.h
# End Source File
# Begin Source File

SOURCE=..\doomdef.h
# End Source File
# Begin Source File

SOURCE=..\doomstat.h
# End Source File
# Begin Source File

SOURCE=..\doomtype.h
# End Source File
# Begin Source File

SOURCE=..\z_zone.c
# End Source File
# Begin Source File

SOURCE=..\z_zone.h
# End Source File
# End Group
# Begin Group "F_Frame"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\f_finale.c
# End Source File
# Begin Source File

SOURCE=..\f_finale.h
# End Source File
# Begin Source File

SOURCE=..\f_wipe.c
# End Source File
# End Group
# Begin Group "G_Game"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\g_game.c
# End Source File
# Begin Source File

SOURCE=..\g_game.h
# End Source File
# Begin Source File

SOURCE=..\g_input.c
# End Source File
# Begin Source File

SOURCE=..\g_input.h
# End Source File
# Begin Source File

SOURCE=..\g_state.h
# End Source File
# End Group
# Begin Group "H_Hud"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\am_map.c
# End Source File
# Begin Source File

SOURCE=..\am_map.h
# End Source File
# Begin Source File

SOURCE=..\command.c
# End Source File
# Begin Source File

SOURCE=..\command.h
# End Source File
# Begin Source File

SOURCE=..\console.c
# End Source File
# Begin Source File

SOURCE=..\console.h
# End Source File
# Begin Source File

SOURCE=..\hu_stuff.c
# End Source File
# Begin Source File

SOURCE=..\hu_stuff.h
# End Source File
# Begin Source File

SOURCE=..\st_stuff.c
# End Source File
# Begin Source File

SOURCE=..\st_stuff.h
# End Source File
# Begin Source File

SOURCE=..\y_inter.c
# End Source File
# Begin Source File

SOURCE=..\y_inter.h
# End Source File
# End Group
# Begin Group "Hw_Hardware"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\hardware\hw3dsdrv.h
# End Source File
# Begin Source File

SOURCE=..\hardware\hw3sound.c
# End Source File
# Begin Source File

SOURCE=..\hardware\hw3sound.h
# End Source File
# Begin Source File

SOURCE=..\hardware\hw_bsp.c
# End Source File
# Begin Source File

SOURCE=..\hardware\hw_cache.c
# End Source File
# Begin Source File

SOURCE=..\hardware\hw_data.h
# End Source File
# Begin Source File

SOURCE=..\hardware\hw_defs.h
# End Source File
# Begin Source File

SOURCE=..\hardware\hw_dll.h
# End Source File
# Begin Source File

SOURCE=..\hardware\hw_draw.c
# End Source File
# Begin Source File

SOURCE=..\hardware\hw_drv.h
# End Source File
# Begin Source File

SOURCE=..\hardware\hw_glob.h
# End Source File
# Begin Source File

SOURCE=..\hardware\hw_light.c
# End Source File
# Begin Source File

SOURCE=..\hardware\hw_light.h
# End Source File
# Begin Source File

SOURCE=..\hardware\hw_main.c
# End Source File
# Begin Source File

SOURCE=..\hardware\hw_main.h
# End Source File
# Begin Source File

SOURCE=..\hardware\hw_md2.c
# End Source File
# Begin Source File

SOURCE=..\hardware\hw_md2.h
# End Source File
# Begin Source File

SOURCE=..\hardware\hws_data.h
# End Source File
# End Group
# Begin Group "I_Interface"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\byteptr.h
# End Source File
# Begin Source File

SOURCE=..\i_joy.h
# End Source File
# Begin Source File

SOURCE=..\i_net.h
# End Source File
# Begin Source File

SOURCE=..\i_sound.h
# End Source File
# Begin Source File

SOURCE=..\i_system.h
# End Source File
# Begin Source File

SOURCE=..\i_tcp.c
# End Source File
# Begin Source File

SOURCE=..\i_tcp.h
# End Source File
# Begin Source File

SOURCE=..\i_video.h
# End Source File
# Begin Source File

SOURCE=..\keys.h
# End Source File
# Begin Source File

SOURCE=..\mserv.c
# End Source File
# Begin Source File

SOURCE=..\mserv.h
# End Source File
# End Group
# Begin Group "M_Misc"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\m_argv.c
# End Source File
# Begin Source File

SOURCE=..\m_argv.h
# End Source File
# Begin Source File

SOURCE=..\m_bbox.c
# End Source File
# Begin Source File

SOURCE=..\m_bbox.h
# End Source File
# Begin Source File

SOURCE=..\m_cheat.c
# End Source File
# Begin Source File

SOURCE=..\m_cheat.h
# End Source File
# Begin Source File

SOURCE=..\m_dllist.h
# End Source File
# Begin Source File

SOURCE=..\m_fixed.c
# End Source File
# Begin Source File

SOURCE=..\m_fixed.h
# End Source File
# Begin Source File

SOURCE=..\m_menu.c
# End Source File
# Begin Source File

SOURCE=..\m_menu.h
# End Source File
# Begin Source File

SOURCE=..\m_misc.c
# End Source File
# Begin Source File

SOURCE=..\m_misc.h
# End Source File
# Begin Source File

SOURCE=..\m_queue.c
# End Source File
# Begin Source File

SOURCE=..\m_queue.h
# End Source File
# Begin Source File

SOURCE=..\m_random.c
# End Source File
# Begin Source File

SOURCE=..\m_random.h
# End Source File
# Begin Source File

SOURCE=..\m_swap.h
# End Source File
# Begin Source File

SOURCE=..\string.c
# End Source File
# End Group
# Begin Group "P_Play"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\info.c
# End Source File
# Begin Source File

SOURCE=..\info.h
# End Source File
# Begin Source File

SOURCE=..\p_ceilng.c
# End Source File
# Begin Source File

SOURCE=..\p_enemy.c
# End Source File
# Begin Source File

SOURCE=..\p_fab.c
# End Source File
# Begin Source File

SOURCE=..\p_floor.c
# End Source File
# Begin Source File

SOURCE=..\p_inter.c
# End Source File
# Begin Source File

SOURCE=..\p_lights.c
# End Source File
# Begin Source File

SOURCE=..\p_local.h
# End Source File
# Begin Source File

SOURCE=..\p_map.c
# End Source File
# Begin Source File

SOURCE=..\p_maputl.c
# End Source File
# Begin Source File

SOURCE=..\p_maputl.h
# End Source File
# Begin Source File

SOURCE=..\p_mobj.c
# End Source File
# Begin Source File

SOURCE=..\p_mobj.h
# End Source File
# Begin Source File

SOURCE=..\p_polyobj.c
# End Source File
# Begin Source File

SOURCE=..\p_polyobj.h
# End Source File
# Begin Source File

SOURCE=..\p_pspr.h
# End Source File
# Begin Source File

SOURCE=..\p_saveg.c
# End Source File
# Begin Source File

SOURCE=..\p_saveg.h
# End Source File
# Begin Source File

SOURCE=..\p_setup.c
# End Source File
# Begin Source File

SOURCE=..\p_setup.h
# End Source File
# Begin Source File

SOURCE=..\p_sight.c
# End Source File
# Begin Source File

SOURCE=..\p_spec.c
# End Source File
# Begin Source File

SOURCE=..\p_spec.h
# End Source File
# Begin Source File

SOURCE=..\p_telept.c
# End Source File
# Begin Source File

SOURCE=..\p_tick.c
# End Source File
# Begin Source File

SOURCE=..\p_tick.h
# End Source File
# Begin Source File

SOURCE=..\p_user.c
# End Source File
# Begin Source File

SOURCE=..\tables.c
# End Source File
# Begin Source File

SOURCE=..\tables.h
# End Source File
# End Group
# Begin Group "R_Rend"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\r_bsp.c
# End Source File
# Begin Source File

SOURCE=..\r_bsp.h
# End Source File
# Begin Source File

SOURCE=..\r_data.c
# End Source File
# Begin Source File

SOURCE=..\r_data.h
# End Source File
# Begin Source File

SOURCE=..\r_defs.h
# End Source File
# Begin Source File

SOURCE=..\r_draw.c
# End Source File
# Begin Source File

SOURCE=..\r_draw.h
# End Source File
# Begin Source File

SOURCE=..\r_draw16.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\r_draw8.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\r_local.h
# End Source File
# Begin Source File

SOURCE=..\r_main.c
# End Source File
# Begin Source File

SOURCE=..\r_main.h
# End Source File
# Begin Source File

SOURCE=..\r_plane.c
# End Source File
# Begin Source File

SOURCE=..\r_plane.h
# End Source File
# Begin Source File

SOURCE=..\r_segs.c
# End Source File
# Begin Source File

SOURCE=..\r_segs.h
# End Source File
# Begin Source File

SOURCE=..\r_sky.c
# End Source File
# Begin Source File

SOURCE=..\r_sky.h
# End Source File
# Begin Source File

SOURCE=..\r_splats.c
# End Source File
# Begin Source File

SOURCE=..\r_splats.h
# End Source File
# Begin Source File

SOURCE=..\r_state.h
# End Source File
# Begin Source File

SOURCE=..\r_things.c
# End Source File
# Begin Source File

SOURCE=..\r_things.h
# End Source File
# Begin Source File

SOURCE=..\screen.c
# End Source File
# Begin Source File

SOURCE=..\screen.h
# End Source File
# Begin Source File

SOURCE=..\v_video.c
# End Source File
# Begin Source File

SOURCE=..\v_video.h
# End Source File
# End Group
# Begin Group "S_Sounds"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\s_sound.c
# End Source File
# Begin Source File

SOURCE=..\s_sound.h
# End Source File
# Begin Source File

SOURCE=..\sounds.c
# End Source File
# Begin Source File

SOURCE=..\sounds.h
# End Source File
# End Group
# Begin Group "W_Wad"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\lzf.c
# End Source File
# Begin Source File

SOURCE=..\lzf.h
# End Source File
# Begin Source File

SOURCE=..\md5.c
# End Source File
# Begin Source File

SOURCE=..\md5.h
# End Source File
# Begin Source File

SOURCE=..\w_wad.c
# End Source File
# Begin Source File

SOURCE=..\w_wad.h
# End Source File
# End Group
# Begin Group "Docs"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\doc\copying
# End Source File
# Begin Source File

SOURCE=..\..\doc\faq.txt
# End Source File
# Begin Source File

SOURCE=..\..\readme.txt
# End Source File
# Begin Source File

SOURCE=..\..\doc\source.txt
# End Source File
# End Group
# Begin Source File

SOURCE=.\Srb2win.ico
# End Source File
# End Target
# End Project
