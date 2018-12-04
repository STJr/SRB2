@echo off

set "SCRIPTDIR=%~dp0"
set "SCRIPTDIR=%SCRIPTDIR:~0,-1%"

IF [%SRB2VERSIONNAME%] == [] set /p SRB2VERSIONNAME=<"%SCRIPTDIR%\VersionFileName.txt"

:: Find 7z

set SVZIP=

if NOT [%1] == [] (
	echo.%~1 | findstr /C:"7z" 1>nul
	if NOT errorlevel 1 (
		if exist "%~1" set "SVZIP=%~1"
	)
)

if ["%SVZIP%"] == [""] (
	if exist "%ProgramFiles(x86)%\7-Zip\7z.exe" set "SVZIP=%ProgramFiles(x86)%\7-Zip\7z.exe"
	if exist "%ProgramFiles%\7-Zip\7z.exe" set "SVZIP=%ProgramFiles%\7-Zip\7z.exe"
	if exist "%ProgramW6432%\7-Zip\7z.exe" set "SVZIP=%ProgramW6432%\7-Zip\7z.exe"
)

:: Is it in PATH?

if ["%SVZIP%"] == [""] (
	"7z.exe" --help > NUL 2> NUL
	if NOT errorlevel 1 (
		set "SVZIP=7z.exe"
	)
)

:: Create the uninstaller

if NOT ["%SVZIP%"] == [""] (
	del /f /q "%SCRIPTDIR%\Uninstaller.7z" 2> NUL
	"%SVZIP%" a -t7z "%SCRIPTDIR%\Uninstaller.7z" "%SCRIPTDIR%\uninstaller\*" -m5=LZMA2
	copy /y /b "%SCRIPTDIR%\sfx\7zsd_LZMA2.sfx" + "%SCRIPTDIR%\sfx\config-uninstaller.txt" + "%SCRIPTDIR%\Uninstaller.7z" "%SCRIPTDIR%\staging\new-install\uninstall.exe"
	del /f /q "%SCRIPTDIR%\uninstaller.7z"
)

:: Operate on install archives

type NUL > "%SCRIPTDIR%\staging\new-install\staging.txt"

if exist "%SCRIPTDIR%\Installer.7z" (
	if NOT ["%SVZIP%"] == [""] (
		"%SVZIP%" a "%SCRIPTDIR%\Installer.7z" "%SCRIPTDIR%\staging\*"
	)
	copy /y /b "%SCRIPTDIR%\sfx\7zsd_LZMA2.sfx" + "%SCRIPTDIR%\sfx\config-installer.txt" + "%SCRIPTDIR%\Installer.7z" "%SCRIPTDIR%\SRB2-%SRB2VERSIONNAME%-Installer.exe"
)

if exist "%SCRIPTDIR%\Patch.7z" (
	if NOT ["%SVZIP%"] == [""] (
		"%SVZIP%" a "%SCRIPTDIR%\Patch.7z" "%SCRIPTDIR%\staging\*"
	)
	copy /y /b "%SCRIPTDIR%\sfx\7zsd_LZMA2.sfx" + "%SCRIPTDIR%\sfx\config-patch.txt" + "%SCRIPTDIR%\Patch.7z" "%SCRIPTDIR%\SRB2-%SRB2VERSIONNAME%-Patch.exe"
)

if exist "%SCRIPTDIR%\Installer_x64.7z" (
	if NOT ["%SVZIP%"] == [""] (
		"%SVZIP%" a "%SCRIPTDIR%\Installer_x64.7z" "%SCRIPTDIR%\staging\*"
	)
	copy /y /b "%SCRIPTDIR%\sfx\7zsd_LZMA2_x64.sfx" + "%SCRIPTDIR%\sfx\config-installer.txt" + "%SCRIPTDIR%\Installer_x64.7z" "%SCRIPTDIR%\SRB2-%SRB2VERSIONNAME%-x64-Installer.exe"
)

if exist "%SCRIPTDIR%\Patch_x64.7z" (
	if NOT ["%SVZIP%"] == [""] (
		"%SVZIP%" a "%SCRIPTDIR%\Patch_x64.7z" "%SCRIPTDIR%\staging\*"
	)
	copy /y /b "%SCRIPTDIR%\sfx\7zsd_LZMA2_x64.sfx" + "%SCRIPTDIR%\sfx\config-patch.txt" + "%SCRIPTDIR%\Patch_x64.7z" "%SCRIPTDIR%\SRB2-%SRB2VERSIONNAME%-x64-Patch.exe"
)

del /f /q "%SCRIPTDIR%\staging\new-install\staging.txt"
del /f /q "%SCRIPTDIR%\staging\new-install\uninstall.exe"
