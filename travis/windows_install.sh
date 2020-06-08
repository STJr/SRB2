#!/bin/bash

# Deployer for Travis-CI
# Windows Setup

# Install MSYS and MINGW-W64
# Right now, we only use MSYS2 for installing packages. IF YOU USE MSYS2 IN OTHER WAYS, NOTE:
# - MSYS2 does NOT inherit environment variables from Bash.
# - MSYS2 CAN inherit PATH from Windows because we invoke msys2_shell.cmd with --full-path.
# - Each call to MSYS2 is a NEW SHELL!
if [ ! -f "C:/tools/msys64/msys2_shell.cmd" ]; then
  rm -rf C:/tools/msys64;
fi;
choco uninstall -y mingw
choco upgrade --no-progress -y msys2
export msys2='cmd //C RefreshEnv.cmd '
export msys2+='& set MSYS=winsymlinks:nativestrict '
export msys2+='& C:\\tools\\msys64\\msys2_shell.cmd -defterm -no-start'
export mingw64="$msys2 -mingw64 -full-path -here -c "\"\$@"\" --"
export msys2+=" -msys2 -c "\"\$@"\" --"
# Install current toolchain
if [[ "$MSYS2_PACKAGES" == "" ]]; then
  MSYS2_PACKAGES="mingw-w64-i686-toolchain;mingw-w64-i686-nasm;mingw-w64-i686-ccache";
fi
IFS=';' read -r -a pkgs <<< "$MSYS2_PACKAGES";
for pkg in "${pkgs[@]}";
do
  $msys2 pacman --sync -u --noconfirm --needed $pkg;
done;
# Get specific GCC version. This is lazy, but we'll overwrite the current
# install with the linked package. 
if [[ "$MSYS2_STORES" == "" ]]; then
  MSYS2_STORES="http://repo.msys2.org/mingw/i686/mingw-w64-i686-gcc-10.1.0-3-any.pkg.tar.zst";
fi
IFS=';' read -r -a pkgs <<< "$MSYS2_STORES";
for pkg in "${pkgs[@]}";
do
  wget "$pkg";
  $msys2 pacman -U --noconfirm "$(basename $pkg)";
done;
## Install more MSYS2 packages from https://packages.msys2.org/base here
#taskkill //IM gpg-agent.exe //F  # https://travis-ci.community/t/4967
export PATH=/c/tools/msys64/mingw64/bin:/c/tools/msys64/mingw32/bin:$PATH
export MAKE=mingw32-make  # so that Autotools can find it
