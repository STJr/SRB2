@echo off

cls

set "INSTALLDIR=%~dp0"
set "INSTALLDIR=%INSTALLDIR:~0,-1%"
set /p USERDIR=<"%INSTALLDIR%\uninstall-userdir.txt"
set "USERDIR=%USERDIR:~0,-1%"

: ProceedPrompt

set PROCEED=
set /p PROCEED="Are you sure you want to uninstall SRB2? [yes/no] "

if /I ["%PROCEED:~0,1%"] == ["n"] exit
if /I ["%PROCEED%"] == ["y"] (
	echo Type Yes or No
	echo.
	goto ProceedPrompt
) else (
	if /I ["%PROCEED%"] == ["yes"] (
		set PROCEED=1
	) else (
		echo.
		goto ProceedPrompt
	)
)

:: Failsafe, in case we Ctrl+C and decline "Terminate batch file?"

if ["%PROCEED%"] == ["1"] (
	echo.
) else (
	exit
)

: DeleteAllPrompt

:: TODO: Let's DUMMY this out for now -- I'm not confident that a full-folder
:: deletion wouldn't be disastrous for edge-cases. E.g., what if we somehow got
:: Program Files as the folder to delete? Or C:\?
::
:: Let's just delete individual files, attempt to delete the folder if it is
:: empty, then leave it up to the user to clear the rest. Yes, their data
:: and mods will be left alone by the uninstaller.
::
:: mazmazz 12/2/2018

set DELETEALL=

goto DeleteFiles

echo Do you want to delete your game data and mods? In folder:
echo %USERDIR%

set /p DELETEALL="[Yes/no] "

if /I ["%DELETEALL:~0,1%"] == ["n"] (
	set DELETEALL=
	goto DeleteFiles
)

if /I ["%DELETEALL%"] == ["y"] (
	echo Type Yes or No
	goto DeleteAllPrompt
) else (
	if /I ["%DELETEALL%"] == ["yes"] (
		set DELETEALL=1
	) else (
		echo.
		goto DeleteAllPrompt
	)
)

:: TODO: Would be nice to have an additional prompt asking
:: Are you REALLY sure you want to delete your data? You cannot
:: recover it!

: DeleteAll

:: Dummy out for now

goto DeleteFiles

:: Failsafe to not delete the directory if, SOMEHOW,
:: your install dir or user dir is C:\
set SAMEFAILSAFE=

:: Don't delete the install folder until the very last step of the script!
if ["%DELETEALL%"] == ["1"] (
	echo "Deleting all data"
	:: TODO: This is wrong! The idea for this check is to see if the user
	:: somehow installed SRB2 in C:\. So we obviously don't want to do
	:: full folder deletion
	::
	:: However, the %CD% == %INSTALLDIR% check doesn't work properly
	:: because CD doesn't update when you change the directory?
	:: So maybe it will just delete files anyway. I'm really not sure.
	::
	if /I ["%CD%"] == ["%INSTALLDIR%"] cd \
	if /I ["%CD%"] == ["%INSTALLDIR%"] (
		echo Install directory is the same as your drive root!
		if /I ["%USERDIR%"] == ["%INSTALLDIR%"] (
			echo Let's just delete files instead...
			echo.
            set DELETEALL=
			goto DeleteFiles
		)
		set SAMEFAILSAFE=1
	) else (
		echo.
	)

	if /I ["%USERDIR%"] == ["%INSTALLDIR%"] goto AllDone

	if /I ["%CD%"] == ["%USERDIR%"] cd \
	if /I ["%CD%"] == ["%USERDIR%"] (
		echo User data directory is the same as your drive root!
		if ["%SAMEFAILSAFE%"] == ["1"] (
			echo Let's just delete files instead...
			echo.
            set DELETEALL=
			goto DeleteFiles
		) else (
			echo Let's not delete any user folders...
			goto AllDone
		)
	) else (
		rmdir /s /q "%USERDIR%"
		if ["%SAMEFAILSAFE%"] == ["1"] (
			echo Let's delete your installation files...
			echo.
            set DELETEALL=
			goto DeleteFiles
		) else (
			goto AllDone
		)
	)
)

: DeleteFiles

for /F "usebackq tokens=*" %%A in ("%INSTALLDIR%\uninstall-list.txt") do (
	if exist "%INSTALLDIR%\%%A" (
		if ["%%A"] == ["%~nx0"] (
			echo.
		) else (
			echo Deleting %INSTALLDIR%\%%A
			del /q /f "%INSTALLDIR%\%%A"
		)
	)
)

: AllDone

:: Delete the program icons
echo Deleting your program icons...
echo.

cd \
rmdir /s /q "%AppData%\Microsoft\Windows\Start Menu\Programs\Sonic Robo Blast 2"

:: Check if our install folder is non-empty

set USERDIRFILLED=
set INSTALLDIRFILLED=
for /F %%i in ('dir /b /a "%USERDIR%\*"') do (
    if ["%%i"] == ["%~nx0"] (
		echo.
	) else (
		set USERDIRFILLED=1
		goto InstallFilledCheck
	)
)

: InstallFilledCheck

if /I ["%USERDIR%"] == ["%INSTALLDIR%"] (
	echo.
) else (
	for /F %%i in ('dir /b /a "%INSTALLDIR%\*"') do (
		if ["%%i"] == ["%~nx0"] (
			echo.
		) else (
			set INSTALLDIRFILLED=1
			goto Final
		)
	)
)

: Final

echo All done! Visit http://www.srb2.org if you want to play SRB2 again!
echo.

set "FINALPROMPT=Press Enter key to exit."
if ["%USERDIRFILLED%"] == ["1"] (
	echo We left your game data and mods alone, so you may delete those manually.
	echo.
	echo    %USERDIR%
	echo.
	set "FINALPROMPT=Do you want to view your data? [yes/no]"
)

if ["%INSTALLDIRFILLED%"] == ["1"] (
	echo We left some extra files alone in your install folder.
	echo.
	echo    %INSTALLDIR%
	echo.
	set "FINALPROMPT=Do you want to view your data? [yes/no]"
)

set FINALRESPONSE=
set /p FINALRESPONSE="%FINALPROMPT% "

if ["%FINALPROMPT%"] == ["Press Enter key to exit."] (
	echo.
) else (
	if /I ["%FINALRESPONSE:~0,1%"] == ["y"] (
		if ["%USERDIRFILLED%"] == ["1"] (
			"%SystemRoot%\explorer.exe" "%USERDIR%"
		)
		if ["%INSTALLDIRFILLED%"] == ["1"] (
			"%SystemRoot%\explorer.exe" "%INSTALLDIR%"
		)
	) else (
		if ["%FINALRESPONSE%"] == [""] (
			if ["%USERDIRFILLED%"] == ["1"] (
				"%SystemRoot%\explorer.exe" "%USERDIR%"
			)
			if ["%INSTALLDIRFILLED%"] == ["1"] (
				"%SystemRoot%\explorer.exe" "%INSTALLDIR%"
			)
		)
	)
)

: DeferredDelete

cd \

:: Now let's delete our installation folder!
:: If we're not DELETEALL, just attempt to remove the install dir in case it's empty
::
:: Dummy out for now
:: if ["%DELETEALL%"] == ["1"] (
if ["0"] == ["1"] (
    start "" /b "cmd" /s /c " rmdir /s /q "%INSTALLDIR%"&exit /b "
) else (
    start "" /b "cmd" /s /c " del /q /f "%INSTALLDIR%\uninstall.bat"&timeout /t 2 > NUL&rmdir "%INSTALLDIR%"&exit /b "
)
