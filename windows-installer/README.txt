Windows Install Builder
for SRB2

This installer is much like the 7-Zip self-extracting archive, except
this allows for scripting the post-install step.

This also allows for some light customization, including dialog messages
and program shortcuts.

The included install scripts manage the game data location depending on the
install location -- if installed in Program Files or AppData\Local, the
game data location is set to %UserProfile%\SRB2.

Program shortcuts are also added, as well as an uninstaller that
will remove the icons and also selectively uninstall the core game files.
The uninstaller gives you the option to preserve your game data and mods.

How to Use
----------

1. Zip up the install contents in 7z format.
    * ALL FILES MUST BE IN THE `new-install/` ARCHIVE SUBFOLDER, OR THE
      POST-INSTALL SCRIPT WILL NOT WORK!
    * Make sure you are using the LZMA2 algorithm, which is 7-Zip's default.

2. Copy the 7z archive to this folder using the following names:
    * Installer.7z     - 32-bit full installer
    * Patch.7z         - 32-bit patch
    * Installer_x64.7z - 64-bit full installer
    * Patch_x64.7z     - 64-bit full installer

3. Set the text in VersionFilename.txt to the version identifier for the
   installer's filename.
    * e.g., v2121 for v2.1.21, from "SRB2-v2121-Installer.exe"
    * Also look through sfx/config-installer.txt and sfx/config-patch.txt
      and update the version strings. Templating is TODO.

4. Run BuildInstaller.bat [7z.exe install path]
    * First argument is the path to 7z.exe. If this is not specified, the
      script will try to look for it in C:\Program Files [(x86)]
    * This script will automatically add the install scripts to each
      installer.

Credit
------

OlegScherbakov/7zSFX
https://github.com/OlegScherbakov/7zSFX
