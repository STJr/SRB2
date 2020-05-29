@setlocal enableextensions enabledelayedexpansion

::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
: Appveyor Deployer
: See appveyor.yml for default variables
::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
: Evaluate whether we should be deploying
::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

if not [%DPL_ENABLED%] == [1] (
    echo Deployer is not enabled...
    exit /b
)

: Don't do DD installs because fmodex DLL handling is not implemented.
if [%CONFIGURATION%] == [DD] (
    echo Deployer does not support DD builds...
    exit /b
)

if [%CONFIGURATION%] == [DD64] (
    echo Deployer does not support DD builds...
    exit /b
)

: Substring match from https://stackoverflow.com/questions/7005951/batch-file-find-if-substring-is-in-string-not-in-a-file
: The below line says "if deployer is NOT in string"
: Note that APPVEYOR_REPO_BRANCH for pull request builds is the BASE branch that PR is merging INTO
if x%APPVEYOR_REPO_BRANCH:deployer=%==x%APPVEYOR_REPO_BRANCH% (
    if not [%APPVEYOR_REPO_TAG%] == [true] (
        echo Deployer is enabled but we are not in a release tag or a 'deployer' branch...
        exit /b
    ) else (
        if not [%DPL_TAG_ENABLED%] == [1] (
            echo Deployer is not enabled for release tags...
            exit /b
        )
    )
)

: Release tags always get optional assets (music.dta)
if [%APPVEYOR_REPO_TAG%] == [true] (
    set "ASSET_FILES_OPTIONAL_GET=1"
)

::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
: Get asset archives
::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

if exist "C:\Users\appveyor\srb2_cache\archives\" (
    if [%ASSET_CLEAN%] == [1] (
        echo Cleaning asset archives...
        rmdir /s /q "C:\Users\appveyor\srb2_cache\archives"
    )
)

if not exist "C:\Users\appveyor\srb2_cache\archives\" mkdir "C:\Users\appveyor\srb2_cache\archives"

goto EXTRACT_ARCHIVES

::::::::::::::::::::::::::::::::
: ARCHIVE_NAME_PARTS
: Call this like a function. %archivepath% is the path to extract parts from.
::::::::::::::::::::::::::::::::

for %%a in (%archivepath%) do (
    set "file=%%~fa"
    set "filepath=%%~dpa"
    set "filename=%%~nxa"
)

set "localarchivepath=C:\Users\appveyor\srb2_cache\archives\%filename%"

goto EOF

::::::::::::::::::::::::::::::::
: EXTRACT_ARCHIVES
::::::::::::::::::::::::::::::::

set "archivepath=%ASSET_ARCHIVE_PATH%"
call :ARCHIVE_NAME_PARTS
set "ASSET_ARCHIVE_PATH_LOCAL=%localarchivepath%"
if not exist "%localarchivepath%" appveyor DownloadFile "%ASSET_ARCHIVE_PATH%" -FileName "%localarchivepath%"

set "archivepath=%ASSET_ARCHIVE_PATCH_PATH%"
call :ARCHIVE_NAME_PARTS
set "ASSET_ARCHIVE_PATCH_PATH_LOCAL=%localarchivepath%"
if not exist "%localarchivepath%" appveyor DownloadFile "%ASSET_ARCHIVE_PATCH_PATH%" -FileName "%localarchivepath%"

if not [%X86_64%] == [1] (
    set "archivepath=%ASSET_ARCHIVE_X86_PATH%"
    call :ARCHIVE_NAME_PARTS
    set "ASSET_ARCHIVE_X86_PATH_LOCAL=!localarchivepath!"
    if not exist "!localarchivepath!" appveyor DownloadFile "%ASSET_ARCHIVE_X86_PATH%" -FileName "!localarchivepath!"
)

if [%X86_64%] == [1] (
    set "archivepath=%ASSET_ARCHIVE_X64_PATH%"
    call :ARCHIVE_NAME_PARTS
    set "ASSET_ARCHIVE_X64_PATH_LOCAL=!localarchivepath!"
    if not exist "!localarchivepath!" appveyor DownloadFile "%ASSET_ARCHIVE_X64_PATH%" -FileName "!localarchivepath!"
)

if [%ASSET_FILES_OPTIONAL_GET%] == [1] (
    set "archivepath=%ASSET_ARCHIVE_OPTIONAL_PATH%"
    call :ARCHIVE_NAME_PARTS
    set "ASSET_ARCHIVE_OPTIONAL_PATH_LOCAL=!localarchivepath!"
    if not exist "!localarchivepath!" appveyor DownloadFile "%ASSET_ARCHIVE_OPTIONAL_PATH%" -FileName "!localarchivepath!"
)

::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
: Build the installers
::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

mkdir "assets\installer"
mkdir "assets\patch"

7z x -y "%ASSET_ARCHIVE_PATH_LOCAL%" -o"assets\installer" >null
7z x -y "%ASSET_ARCHIVE_PATCH_PATH_LOCAL%" -o"assets\patch" >null

: Copy optional files to full installer (music.dta)
if [%ASSET_FILES_OPTIONAL_GET%] == [1] (
    7z x -y "%ASSET_ARCHIVE_OPTIONAL_PATH_LOCAL%" -o"assets\installer" >null
)

: Copy EXE -- BUILD_PATH is from appveyor.yml
robocopy /S /ns /nc /nfl /ndl /np /njh /njs "%BUILD_PATH%" "assets\installer" /XF "*.debug" ".gitignore"
robocopy /S /ns /nc /nfl /ndl /np /njh /njs "%BUILD_PATH%" "assets\patch" /XF "*.debug" ".gitignore"

: Are we building DD? (we were supposed to exit earlier!)
if [%CONFIGURATION%] == [DD] ( set "DPL_INSTALLER_NAME=%DPL_INSTALLER_NAME%-DD" )
if [%CONFIGURATION%] == [DD64] ( set "DPL_INSTALLER_NAME=%DPL_INSTALLER_NAME%-DD" )

: If we are not a release tag, suffix the filename
if not [%APPVEYOR_REPO_TAG%] == [true] (
    set "INSTALLER_SUFFIX=-%APPVEYOR_REPO_BRANCH%-%GITSHORT%-%CONFIGURATION%"
) else (
    set "INSTALLER_SUFFIX="
)

if not [%X86_64%] == [1] ( goto X86_INSTALL )

::::::::::::::::::::::::::::::::
: X64_INSTALL
::::::::::::::::::::::::::::::::

: Extract DLL binaries
7z x -y "%ASSET_ARCHIVE_X64_PATH_LOCAL%" -o"assets\installer" >null
if [%PACKAGE_PATCH_DLL_GET%] == [1] (
    7z x -y "!ASSET_ARCHIVE_X64_PATH_LOCAL!" -o"assets\patch" >null
)

: Build the installer
7z a -sfx7z.sfx "%DPL_INSTALLER_NAME%-x64-Installer%INSTALLER_SUFFIX%.exe" .\assets\installer\*

: Build the patch
7z a "%DPL_INSTALLER_NAME%-x64-Patch%INSTALLER_SUFFIX%.zip" .\assets\patch\*

: Upload artifacts
appveyor PushArtifact "%DPL_INSTALLER_NAME%-x64-Installer%INSTALLER_SUFFIX%.exe"
appveyor PushArtifact "%DPL_INSTALLER_NAME%-x64-Patch%INSTALLER_SUFFIX%.zip"

: We only do x86 OR x64, one at a time, so exit now.
goto EOF

::::::::::::::::::::::::::::::::
: X86_INSTALL
::::::::::::::::::::::::::::::::

: Extract DLL binaries
7z x -y "%ASSET_ARCHIVE_X86_PATH_LOCAL%" -o"assets\installer" >null
if [%PACKAGE_PATCH_DLL_GET%] == [1] (
    7z x -y "!ASSET_ARCHIVE_X86_PATH_LOCAL!" -o"assets\patch" >null
)

: Build the installer
7z a -sfx7z.sfx "%DPL_INSTALLER_NAME%-Installer%INSTALLER_SUFFIX%.exe" .\assets\installer\*

: Build the patch
7z a "%DPL_INSTALLER_NAME%-Patch%INSTALLER_SUFFIX%.zip" .\assets\patch\*

: Upload artifacts
appveyor PushArtifact "%DPL_INSTALLER_NAME%-Installer%INSTALLER_SUFFIX%.exe"
appveyor PushArtifact "%DPL_INSTALLER_NAME%-Patch%INSTALLER_SUFFIX%.zip"

: We only do x86 OR x64, one at a time, so exit now
goto EOF

::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
: EOF
::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

endlocal
