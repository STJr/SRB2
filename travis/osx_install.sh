#!/bin/bash

# Deployer for Travis-CI
# OS X Setup

# If cache file exists, extract it
if [ -f "$HOME/macports_cache/macports.tar" ]; then
    echo "Extracting MacPorts cache...";
    sudo tar -C / -xf "$HOME/macports_cache/macports.tar";
    unlink "$HOME/macports_cache/macports.tar";
else
    echo "MacPorts cache not found; installing from scratch...";
fi

# Verbose?
if [[ "$VERBOSE" == "1" ]]; then
    __V="-v";
fi

# Install MacPorts
# https://github.com/GiovanniBussi/macports-ci
echo "Installing MacPorts..."
curl -LO https://raw.githubusercontent.com/GiovanniBussi/macports-ci/master/macports-ci
source ./macports-ci install

# Enable ccache in MacPorts
echo "Configuring MacPorts..."
echo "configureccache yes" | sudo tee -a /opt/local/etc/macports/macports.conf
# Set compiler defaults
# Asinine -- https://lists.macports.org/pipermail/macports-users/2009-February/013828.html
sudo cp /opt/local/libexec/macports/lib/port1.0/portconfigure.tcl /opt/local/libexec/macports/lib/port1.0/portconfigure.tcl~
sudo sed -i.bak "s@default configure.march     {}@default configure.march     ${MARCH:-core2}@" /opt/local/libexec/macports/lib/port1.0/portconfigure.tcl
sudo sed -i.bak "s@default configure.mtune     {}@default configure.mtune     ${MTUNE:-haswell}@" /opt/local/libexec/macports/lib/port1.0/portconfigure.tcl
# Undocumented and unreliable -- https://trac.macports.org/ticket/54332
echo "macosx_deployment_target ${MACOSX_DEPLOYMENT_TARGET:-10.9}" | sudo tee -a /opt/local/etc/macports/macports.conf
# Hail mary option, in case the above does not work
__OPTS="macosx_deployment_target=${MACOSX_DEPLOYMENT_TARGET:-10.9} configure.march=${MARCH:-core2} configure.mtune=${MTUNE:-haswell}"

# Add local Portfile repository
# These take priority over the public repository
echo "Initiating local Portfile repository..."
source macports-ci localports "libs/localports"

# Build dependencies
echo "Installing build dependencies..."
# First install zlib as a source package, since this is a program dependency AND a ccache dependency
sudo port -s -N $__V install zlib $__OPTS
# Now install ccache, so the subsequent packages can use it
sudo port -N $__V install ccache
# Now the rest
sudo port -N $__V install p7zip
sudo port -N $__V install cmake
sudo port -N $__V install pkgconfig
sudo port -N $__V install autoconf
sudo port -N $__V install automake
echo "Installing library dependencies..."
sudo port -s -N $__V install libsdl2 $__OPTS
sudo port -s -N $__V install libsdl2_mixer $__OPTS
sudo port -s -N $__V install libgme $__OPTS
sudo port -s -N $__V install libopenmpt $__OPTS
sudo port -s -N $__V install libpng $__OPTS

# Clean up from earlier
sudo unlink /opt/local/libexec/macports/lib/port1.0/portconfigure.tcl
sudo mv /opt/local/libexec/macports/lib/port1.0/portconfigure.tcl~ /opt/local/libexec/macports/lib/port1.0/portconfigure.tcl

# Clean prefix tree
# https://superuser.com/a/165670
#sudo port -f clean --all all
sudo rm -rf /opt/local/var/macports/build/*
sudo rm -rf /opt/local/var/macports/distfiles/*
sudo rm -rf /opt/local/var/macports/packages/*
sudo port -f uninstall inactive

# Make cache
echo "Making MacPorts cache in $HOME/macports_cache/macports.tar...";
tar -cf $HOME/macports_cache/macports.tar /opt/local

# Set compiler flags
export CFLAGS="$CFLAGS -I/opt/local/include -I/opt/local/include/gme -I/opt/local/include/libopenmpt"
export CXXFLAGS="$CXXFLAGS -I/opt/local/include -I/opt/local/include/gme -I/opt/local/include/libopenmpt"
export LDFLAGS="$LDFLAGS -L/opt/local/lib"

echo "OS X Setup Complete!"
