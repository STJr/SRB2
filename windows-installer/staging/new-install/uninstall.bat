@echo off

setlocal enabledelayedexpansion

cls

set "INSTALLDIR=%~dp0"
set "INSTALLDIR=!INSTALLDIR:~0,-1!"
set /p USERDIR=<"!INSTALLDIR!\uninstall-userdir.txt"
set "USERDIR=!USERDIR:~0,-1!"

: ProceedPrompt

set PROCEED=
set /p PROCEED="Are you sure you want to uninstall SRB2? [yes/no] "

if /I ["!PROCEED:~0,1!"] == ["n"] exit
if /I ["!PROCEED!"] == ["y"] (
	echo Type Yes or No
	echo.
	goto ProceedPrompt
) else (
	if /I ["!PROCEED!"] == ["yes"] (
		set PROCEED=1
	) else (
		echo.
		goto ProceedPrompt
	)
)

:: Failsafe, in case we Ctrl+C and decline "Terminate batch file?"

if ["!PROCEED!"] == ["1"] (
	echo.
) else (
	exit
)

: CheckPermissions

:: Write a dummy file and check for an error. If error, we need administrator rights

mkdir "!INSTALLDIR!\uninstall-dummy"

:: TODO elevate automatically
if errorlevel 1 (
	echo We need Administrator Rights to uninstall SRB2.
	echo.
	echo Try running this uninstaller by right-clicking on the icon
	echo and click "Run as administrator"
	echo.
	set /p ADMINFINAL="Press Enter key to exit. "
	exit
) else (
	rmdir /s /q "!INSTALLDIR!\uninstall-dummy"
	goto DeleteFiles
)

: DeleteFiles

:: Our deletion list is a list of filenames, no paths, in the current folder
::
:: We apply the following failsafes:
:: 1. Is filename the script itself?
:: 2. Does filename have illegal characters? https://stackoverflow.com/a/33625339/241046
:: 3. Is filename a directory?
::
:: TODO hack this to support .\file.txt relative paths
:: Can %%A be substring'd to get only the filename and extension?
:: If so, print that to the temp file instead of the whole line
:: And possibly do the folder check before the invalid char check.
::
set "TESTFILE=!TEMP!\!RANDOM!.txt"

for /F "usebackq tokens=*" %%A in ("!INSTALLDIR!\uninstall-list.txt") do (
	if exist "!INSTALLDIR!\%%A" (
		if ["%%A"] == ["%~nx0"] (
			echo.
		) else (
			echo %%A> "!TESTFILE!"
			findstr /r ".*[<>:\"\"/\\|?*%%].*" "!TESTFILE!" >nul
			if !errorlevel! equ 0 (
				echo %%A has invalid characters, skipping...
			) else (
				if exist "!INSTALLDIR!\%%A\*" (
					echo %%A is a folder, skipping...
				) else (
					echo Deleting !INSTALLDIR!\%%A
					del /q /f "!INSTALLDIR!\%%A"
				)
			)
		)
	)
)

del /q /f "!TESTFILE!"

: AllDone

:: Delete the program icons
echo Deleting your program icons...
echo.

cd \
rmdir /s /q "!AppData!\Microsoft\Windows\Start Menu\Programs\Sonic Robo Blast 2"

:: Check if our install folder is non-empty

set USERDIRFILLED=
set INSTALLDIRFILLED=
for /F %%i in ('dir /b /a "!USERDIR!\*"') do (
    if ["%%i"] == ["%~nx0"] (
		echo.
	) else (
		set USERDIRFILLED=1
		goto InstallFilledCheck
	)
)

: InstallFilledCheck

if /I ["!USERDIR!"] == ["!INSTALLDIR!"] (
	echo.
) else (
	for /F %%i in ('dir /b /a "!INSTALLDIR!\*"') do (
		if ["%%i"] == ["%~nx0"] (
			echo.
		) else (
			set INSTALLDIRFILLED=1
			goto Final
		)
	)
)

: Final

echo All done^^! Visit http://www.srb2.org if you want to play SRB2 again^^!
echo.

set "FINALPROMPT=Press Enter key to exit."
if ["!USERDIRFILLED!"] == ["1"] (
	echo We left your game data and mods alone, so you may delete those manually.
	echo.
	echo    !USERDIR!
	echo.
	set "FINALPROMPT=Do you want to view your data? [yes/no]"
)

if ["!INSTALLDIRFILLED!"] == ["1"] (
	echo We left some extra files alone in your install folder.
	echo.
	echo    !INSTALLDIR!
	echo.
	set "FINALPROMPT=Do you want to view your data? [yes/no]"
)

set FINALRESPONSE=
set /p FINALRESPONSE="!FINALPROMPT! "

if ["!FINALPROMPT!"] == ["Press Enter key to exit."] (
	echo.
) else (
	if /I ["!FINALRESPONSE:~0,1!"] == ["y"] (
		if ["!USERDIRFILLED!"] == ["1"] (
			"!SystemRoot!\explorer.exe" "!USERDIR!"
		)
		if ["!INSTALLDIRFILLED!"] == ["1"] (
			"!SystemRoot!\explorer.exe" "!INSTALLDIR!"
		)
	) else (
		if ["!FINALRESPONSE!"] == [""] (
			if ["!USERDIRFILLED!"] == ["1"] (
				"!SystemRoot!\explorer.exe" "!USERDIR!"
			)
			if ["!INSTALLDIRFILLED!"] == ["1"] (
				"!SystemRoot!\explorer.exe" "!INSTALLDIR!"
			)
		)
	)
)

: DeferredDelete

:: Now let's delete our installation folder!
cd \
start "" /b "cmd" /s /c " del /q /f "%INSTALLDIR%\uninstall.bat"&timeout /t 2 > NUL&rmdir "%INSTALLDIR%"&exit /b "
