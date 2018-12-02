set "SCRIPTDIR=%~dp0"
set "SCRIPTDIR=%SCRIPTDIR:~0,-1%"

IF [%SRB2VERSIONNAME%] == [] set /p SRB2VERSIONNAME=<"%SCRIPTDIR%\VersionFileName.txt"

:: Find 7z

set SVZIP=

if [%1] == [] (
	echo.
) else (
	echo.%~1 | findstr /C:"7z" 1>nul
	if errorlevel 1 (
		echo.
	) else (
		if exist "%~1" set "SVZIP=%~1"
	)
)

if ["%SVZIP%"] == [""] (
	if exist "%ProgramFiles(x86)%\7-Zip\7z.exe" set "SVZIP=%ProgramFiles(x86)%\7-Zip\7z.exe"
	if exist "%ProgramFiles%\7-Zip\7z.exe" set "SVZIP=%ProgramFiles%\7-Zip\7z.exe"
	if exist "%ProgramW6432%\7-Zip\7z.exe" set "SVZIP=%ProgramW6432%\7-Zip\7z.exe"
)

:: Operate on install archives

if exist "%SCRIPTDIR%\Installer.7z" (
	if ["%SVZIP%"] == [""] (
		echo.
	) else (
		"%SVZIP%" a "%SCRIPTDIR%\Installer.7z" "%SCRIPTDIR%\new-install"
	)
	copy /y /b "%SCRIPTDIR%\sfx\7zsd_LZMA2.sfx" + "%SCRIPTDIR%\sfx\config-installer.txt" + "%SCRIPTDIR%\Installer.7z" "%SCRIPTDIR%\SRB2-%SRB2VERSIONNAME%-Installer.exe"
)

if exist "%SCRIPTDIR%\Patch.7z" (
	if ["%SVZIP%"] == [""] (
		echo.
	) else (
		"%SVZIP%" a "%SCRIPTDIR%\Patch.7z" "%SCRIPTDIR%\new-install\"
	)
	copy /y /b "%SCRIPTDIR%\sfx\7zsd_LZMA2.sfx" + "%SCRIPTDIR%\sfx\config-patch.txt" + "%SCRIPTDIR%\Patch.7z" "%SCRIPTDIR%\SRB2-%SRB2VERSIONNAME%-Patch.exe"
)

if exist "%SCRIPTDIR%\Installer_x64.7z" (
	if ["%SVZIP%"] == [""] (
		echo.
	) else (
		"%SVZIP%" a "%SCRIPTDIR%\Installer_x64.7z" "%SCRIPTDIR%\new-install\"
	)
	copy /y /b "%SCRIPTDIR%\sfx\7zsd_LZMA2_x64.sfx" + "%SCRIPTDIR%\sfx\config-installer.txt" + "%SCRIPTDIR%\Installer_x64.7z" "%SCRIPTDIR%\SRB2-%SRB2VERSIONNAME%-x64-Installer.exe"
)

if exist "%SCRIPTDIR%\Patch_x64.7z" (
	if ["%SVZIP%"] == [""] (
		echo.
	) else (
		"%SVZIP%" a "%SCRIPTDIR%\Patch_x64.7z" "%SCRIPTDIR%\new-install\"
	)
	copy /y /b "%SCRIPTDIR%\sfx\7zsd_LZMA2_x64.sfx" + "%SCRIPTDIR%\sfx\config-patch.txt" + "%SCRIPTDIR%\Patch_x64.7z" "%SCRIPTDIR%\SRB2-%SRB2VERSIONNAME%-x64-Patch.exe"
)