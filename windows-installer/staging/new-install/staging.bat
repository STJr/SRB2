@echo off

setlocal enabledelayedexpansion

cls

:: SRB2 Install Staging
::
:: This accomplishes the following tasks:
::
:: 1. Creates a user profile folder if SRB2 is installed in AppData or Program Files, and config.cfg is not already in the install folder
::
:: 2. Moves old installation files into old-install
::
:: 3. Moves new installaton files into install folder
::

:: For 2.1.21, we are changing the DLL structure
:: So move everything that's EXE or DLL

set MoveOldExesDlls=1

:: Get Parent folder (the SRB2 install folder)
::
:: https://wiert.me/2011/08/30/batch-file-to-get-parent-directory-not-the-directory-of-the-batch-file-but-the-parent-of-that-directory/

set "STAGINGDIR=%~dp0"
:: strip trailing backslash
set "STAGINGDIR=!STAGINGDIR:~0,-1!"
::  ~dp only works for batch file parameters and loop indexes
for %%d in ("!STAGINGDIR!") do set "INSTALLDIR=%%~dpd"
set "INSTALLDIR=!INSTALLDIR:~0,-1!"

:: FAILSAFE: Check if staging.txt exists in the directory
:: If not, exit, so we don't mess up anything by accident.

if exist "!STAGINGDIR!\staging.txt" (
	echo.
) else (
	exit
)

: CheckPermissionsInstall

:: Write a dummy file and check for an error. If error, we need administrator rights

:: NOTE: We should never have to deal with this because the main installer should
:: already have the rights.

mkdir "!INSTALLDIR!\install-dummy"

:: TODO elevate automatically
if errorlevel 1 (
	echo Finish installing SRB2 with these steps:
	echo.
	echo 1. Go to your SRB2 install folder
	echo.
	echo     !INSTALLDIR!
	echo.
	echo 2. Copy all files from the "new-install" subfolder into the main folder
	echo    and DELETE staging.bat and staging.txt!!!
	echo.
	echo 3. Optionally, create a folder in your user profile named "SRB2".
	echo    This is where your game data and addons may live.
	echo    To create the folder, go here:
	echo.
	echo     !USERPROFILE!
	echo.
	echo If anything fails, you may delete the files and try to run the installer
	echo again with Administrator Rights: Right-click on the icon and click
	echo "Run as administrator."
	echo.
	"!SystemRoot!\explorer.exe" "!INSTALLDIR!"
	set /p ADMINFINAL="Press Enter key to exit. "

	exit
) else (
	rmdir /s /q "!INSTALLDIR!\install-dummy"
	goto CheckUserDir
)

: CheckUserDir

:: Check if we need to create !userprofile!\SRB2

set "USERDIR=!INSTALLDIR!"

:: Is config.cfg in our install dir?
if exist "!INSTALLDIR!\config.cfg" goto MoveOldInstall

:: Are we in AppData?
echo.!STAGINGDIR! | findstr /C:"!LocalAppData!" 1>nul
if errorlevel 1 (
	echo.
) else (
	goto SetUserDir
)

: Are we in Program Files?
echo.!STAGINGDIR! | findstr /C:"!ProgramFiles!" 1>nul
if errorlevel 1 (
	echo.
) else (
	goto SetUserDir
)

:: Are we in Program Files (x86)?
echo.!STAGINGDIR! | findstr /C:"!ProgramFiles(X86)!" 1>nul
if errorlevel 1 (
	echo.
) else (
	goto SetUserDir
)

:: Are we 32-bit and actually in Program Files?
echo.!STAGINGDIR! | findstr /C:"!ProgramW6432!" 1>nul
if errorlevel 1 (
	echo.
) else (
	goto SetUserDir
)

goto MoveOldInstall

: SetUserDir
: CheckPermissionsUserDir

set "USERDIR=!UserProfile!\SRB2"

:: Check for permissions and create the folder
if exist "!USERDIR!\*" (
 	mkdir "!USERDIR!\install-dummy"

	if errorlevel 1 (
		echo User profile folder exists, but we won't operate on it.
		echo.
		goto MoveOldInstall
	) else (
		rmdir /s /q "!USERDIR!\install-dummy"
	)
) else (
	mkdir "!USERDIR!"

	if errorlevel 1 (
		echo Could not create user profile folder
		echo Defaulting to install dir: "!INSTALLDIR!"
		echo.
		set "USERDIR=!INSTALLDIR!"
		goto MoveOldInstall
	)
)

:: Now copy READMEs
:: echo f answers xcopy's prompt as to whether the destination is a file or a folder
echo f | xcopy /y "!STAGINGDIR!\README.txt" "!USERDIR!\README.txt"
echo f | xcopy /y "!STAGINGDIR!\LICENSE.txt" "!USERDIR!\LICENSE.txt"
echo f | xcopy /y "!STAGINGDIR!\LICENSE-3RD-PARTY.txt" "!USERDIR!\LICENSE-3RD-PARTY.txt"
echo Your game data and mods folder is: > "!USERDIR!\^! Data and Mods Go Here ^!.txt"
echo. >> "!USERDIR!\^! Data and Mods Go Here ^!.txt"
echo     !USERDIR! >> "!USERDIR!\^! Data and Mods Go Here ^!.txt"
echo. >> "!USERDIR!\^! Data and Mods Go Here ^!.txt"
echo Your install folder is: >> "!USERDIR!\^! Data and Mods Go Here ^!.txt"
echo. >> "!USERDIR!\^! Data and Mods Go Here ^!.txt"
echo     !INSTALLDIR! >> "!USERDIR!\^! Data and Mods Go Here ^!.txt"
echo. >> "!USERDIR!\^! Data and Mods Go Here ^!.txt"
echo To run SRB2, go to: >> "!USERDIR!\^! Data and Mods Go Here ^!.txt"
echo. >> "!USERDIR!\^! Data and Mods Go Here ^!.txt"
echo     Start Menu ^> Programs ^> Sonic Robo Blast 2 >> "!USERDIR!\^! Data and Mods Go Here ^!.txt"

:: Copy path to install folder

set "SCRIPT=!TEMP!\!RANDOM!-!RANDOM!-!RANDOM!-!RANDOM!.vbs"
echo Set oWS = WScript.CreateObject("WScript.Shell") >> "!SCRIPT!"
echo sLinkFile = "!USERDIR!\^! SRB2 Install Folder ^!.lnk" >> "!SCRIPT!"
echo Set oLink = oWS.CreateShortcut(sLinkFile) >> "!SCRIPT!"
echo oLink.TargetPath = "!INSTALLDIR!" >> "!SCRIPT!"
echo oLink.WorkingDirectory = "!INSTALLDIR!" >> "!SCRIPT!"
echo oLink.Arguments = "" >> "!SCRIPT!"
echo oLink.IconLocation = "!INSTALLDIR!\srb2win.exe,0" >> "!SCRIPT!"
echo oLink.Save >> "!SCRIPT!"
cscript /nologo "!SCRIPT!"
del "!SCRIPT!"

:: Also do it the other way around

set "SCRIPT=!TEMP!\!RANDOM!-!RANDOM!-!RANDOM!-!RANDOM!.vbs"
echo Set oWS = WScript.CreateObject("WScript.Shell") >> "!SCRIPT!"
echo sLinkFile = "!INSTALLDIR!\^! SRB2 Data Folder ^!.lnk" >> "!SCRIPT!"
echo Set oLink = oWS.CreateShortcut(sLinkFile) >> "!SCRIPT!"
echo oLink.TargetPath = "!USERDIR!" >> "!SCRIPT!"
echo oLink.WorkingDirectory = "!USERDIR!" >> "!SCRIPT!"
echo oLink.Arguments = "" >> "!SCRIPT!"
echo oLink.IconLocation = "!INSTALLDIR!\srb2win.exe,0" >> "!SCRIPT!"
echo oLink.Save >> "!SCRIPT!"
cscript /nologo "!SCRIPT!"
del "!SCRIPT!"

: MoveOldInstall

if exist "!INSTALLDIR!\old-install\*" (
	set "OLDINSTALLDIR=!INSTALLDIR!\old-install-!RANDOM!"
) else (
	set "OLDINSTALLDIR=!INSTALLDIR!\old-install"
)

mkdir "!OLDINSTALLDIR!"

:
: Move all EXEs and DLLs
:

set OLDINSTALLCHANGED=

if ["!MoveOldExesDlls!"] == ["1"] (
	goto MoveOldInstallExeDll
) else (
	goto MoveOldInstallNewFiles
)

: MoveOldInstallExeDll

xcopy /y /v "!INSTALLDIR!\*.exe" "!OLDINSTALLDIR!"
if errorlevel 0 del /f /q "!INSTALLDIR!\*.exe"
xcopy /y /v "!INSTALLDIR!\*.dll" "!OLDINSTALLDIR!"
if errorlevel 0 del /f /q "!INSTALLDIR!\*.dll"

for %%F in ("!OLDINSTALLDIR!\*") DO (
	set OLDINSTALLCHANGED=1
	goto MoveOldInstallNewFiles
)

: MoveOldInstallNewFiles

:: Save a list of standard files
:: So the uninstall script will know what to remove
:: Append to any existing file, in case we are a patch

dir /b /a-d "!STAGINGDIR!" >> "!INSTALLDIR!\uninstall-list.txt"

:: Overwrite the last known gamedata folder

echo !USERDIR! > "!INSTALLDIR!\uninstall-userdir.txt"

:: Add the install-generated to the uninstall list

echo uninstall-list.txt >> "!INSTALLDIR!\uninstall-list.txt"
echo uninstall-userdir.txt >> "!INSTALLDIR!\uninstall-list.txt"
:: *ahem* Prints as ^! SRB2 Data Folder ^!.lnk
:: We need to escape the exclamations (^^!) and the carets themselves (^^^^)
echo ^^^^^^! SRB2 Data Folder ^^^^^^!.lnk >> "!INSTALLDIR!\uninstall-list.txt"

:: Start moving files

for %%F in ("!STAGINGDIR!\*") DO (
	if exist "!INSTALLDIR!\%%~nxF" (
		set OLDINSTALLCHANGED=1
		move "!INSTALLDIR!\%%~nxF" "!OLDINSTALLDIR!\%%~nxF"
	)
	if ["%%~nxF"] == ["staging.bat"] (
		echo.
	) else (
		if ["%%~nxF"] == ["staging.txt"] (
			echo.
		) else (
			move "!STAGINGDIR!\%%~nxF" "!INSTALLDIR!\%%~nxF"
		)
	)
)

: Finished

del /q /f "!INSTALLDIR!\^! SRB2 INSTALL INSTRUCTIONS ^!.txt"

set MSGEXE=
if exist "!SystemRoot!\System32\msg.exe" (
	set MSGEXE=!SystemRoot!\System32\msg.exe
) else (
	if exist "!SystemRoot!\Sysnative\msg.exe" (
		set MSGEXE=!SystemRoot!\Sysnative\msg.exe
	)
)

if ["!OLDINSTALLCHANGED!"] == ["1"] (
	"!systemroot!\explorer.exe" /select, "!OLDINSTALLDIR!"
	echo Finished^^! Some of your old installation files were moved to the "old-install" folder. > !TEMP!\srb2msgprompt.txt
	echo. >> !TEMP!\srb2msgprompt.txt
	echo If you no longer need these files, you may delete the folder safely. >> !TEMP!\srb2msgprompt.txt
	echo. >> !TEMP!\srb2msgprompt.txt
	echo To run SRB2, go to: Start Menu ^> Programs ^> Sonic Robo Blast 2. >> !TEMP!\srb2msgprompt.txt
	!MSGEXE! "!username!" < !TEMP!\srb2msgprompt.txt
	del !TEMP!\srb2msgprompt.txt
) else (
	if /I ["!USERDIR!"] == ["!INSTALLDIR!"] (
		"!systemroot!\explorer.exe" "!INSTALLDIR!"
		echo Finished^^! > !TEMP!\srb2msgprompt.txt
		echo. >> !TEMP!\srb2msgprompt.txt
		echo To run SRB2, go to: Start Menu ^> Programs ^> Sonic Robo Blast 2. >> !TEMP!\srb2msgprompt.txt
		!MSGEXE! "!username!" < !TEMP!\srb2msgprompt.txt
		del !TEMP!\srb2msgprompt.txt
	) else (
		"!systemroot!\explorer.exe" "!USERDIR!"
		echo Finished^^! You may find your game data in this folder: > !TEMP!\srb2msgprompt.txt
		echo. >> !TEMP!\srb2msgprompt.txt
		echo     !USERDIR! >> !TEMP!\srb2msgprompt.txt
		echo. >> !TEMP!\srb2msgprompt.txt
		echo To run SRB2, go to: Start Menu ^> Programs ^> Sonic Robo Blast 2. >> !TEMP!\srb2msgprompt.txt
		!MSGEXE! "!username!" < !TEMP!\srb2msgprompt.txt
		del !TEMP!\srb2msgprompt.txt
	)
)

: Attempt to remove OLDINSTALLDIR, in case it's empty
rmdir /q "!OLDINSTALLDIR!"
cd \
start "" /b "cmd" /s /c " del /f /q "%STAGINGDIR%\*"&rmdir /s /q "%STAGINGDIR%"&exit /b "
