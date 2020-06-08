#!/bin/bash

# Deployer for Travis-CI
# Default Variables
#
# Here are all of the user-set variables used by Deployer.
# See the "Cross-platform deployment" page on SRB2 Wiki for documentation.

# Core Parameters
: ${DPL_FORCE_ON}               # Force-enable Deployer for branches
: ${DPL_FORCE_ON_GITHUB}        # Force-enable Deployer for GitHub Releases
: ${DPL_FORCE_ON_FTP}           # Force-enable Deployer for FTP
: ${DPL_FORCE_OFF}              # Force-disable Deployer for tags
: ${DPL_FORCE_OFF_GITHUB}       # Force-disable Deployer for GitHub Releases
: ${DPL_FORCE_OFF_FTP}          # Force-disable Deployer for FTP
: ${DPL_BRANCH_TRIGGER:=deployer} # Use a word in the branch name to trigger Deployer
: ${DPL_COMMIT_TRIGGER:=deployer} # Use a [word] in the commit message to trigger Deployer
: ${DPL_UPLOAD_BINARY:=1}       # Upload an archive containing the binary file plus $ASSET_FILES_DOCS
: ${DPL_UPLOAD_ASSETS}          # Upload an archive containing the binary file plus $ASSET_FILES_DOCS plus all other assets
: ${DPL_UPLOAD_INSTALLER}       # Upload an archive containing an installer
: ${DPL_ARCHIVE_NAME}           # Name to use for deployed archives. Defaults to $TRAVIS_TAG-$TRAVIS_OS_NAME or "srb2$TRAVIS_OS_NAME-$TRAVIS_BRANCH-${TRAVIS_COMMIT:0:8}-${TRAVIS_JOB_ID}"

# Asset File Parameters
: ${ASSET_ARCHIVE_PATHS:=https://github.com/mazmazz/SRB2/releases/download/SRB2_assets_220/srb2-2.2.4-assets.7z;https://github.com/mazmazz/SRB2/releases/download/SRB2_assets_220/srb2-2.2.4-optional-assets.7z}
: ${ASSET_FILES_HASHED:=srb2.pk3 zones.pk3 player.dta patch.pk3}
: ${ASSET_FILES_DOCS:=README.txt LICENSE.txt LICENSE-3RD-PARTY.txt README-SDL.txt}

# FTP Parameters
: ${DPL_FTP_PROTOCOL:=ftp}
: ${DPL_FTP_USER}
: ${DPL_FTP_PASS}
: ${DPL_FTP_HOSTNAME}
: ${DPL_FTP_PORT:=21}
: ${DPL_FTP_PATH}

# Debian Package Parameters
: ${PACKAGE_NAME:=srb2}
: ${PACKAGE_VERSION:=2.2.0}
: ${PACKAGE_SUBVERSION}         # Highly recommended to set this to reflect the distro series target (e.g., ~18.04bionic)
: ${PACKAGE_REVISION}           # Defaults to UTC timestamp
: ${PACKAGE_INSTALL_PATH:=/usr/games/SRB2}
: ${PACKAGE_LINK_PATH:=/usr/games}
: ${PACKAGE_DISTRO:=trusty}
: ${PACKAGE_URGENCY:=high}
: ${PACKAGE_NAME_EMAIL:=Sonic Team Junior <stjr@srb2.org>}
: ${PACKAGE_GROUP_NAME_EMAIL:=Sonic Team Junior <stjr@srb2.org>}
: ${PACKAGE_WEBSITE:=<http://www.srb2.org>}

: ${PACKAGE_ASSET_MINVERSION:=2.1.26}  # Number this the version BEFORE the actual required version, because we do a > check
: ${PACKAGE_ASSET_MAXVERSION:=2.2.1}  # Number this the version AFTER the actual required version, because we do a < check

: ${PROGRAM_NAME:=Sonic Robo Blast 2}
: ${PROGRAM_VENDOR:=Sonic Team Junior}
: ${PROGRAM_VERSION:=2.2.0}
: ${PROGRAM_DESCRIPTION:=A free 3D Sonic the Hedgehog fangame closely inspired by the original Sonic games on the Sega Genesis.}
: ${PROGRAM_FILENAME:=srb2}

# Export Asset and Package Parameters for envsubst templating

export ASSET_ARCHIVE_PATH="${ASSET_ARCHIVE_PATH}"
export ASSET_ARCHIVE_OPTIONAL_PATH="${ASSET_ARCHIVE_OPTIONAL_PATH}"
export ASSET_FILES_HASHED="${ASSET_FILES_HASHED}"
export ASSET_FILES_DOCS="${ASSET_FILES_DOCS}"
export ASSET_FILES_OPTIONAL_GET="${ASSET_FILES_OPTIONAL_GET}"

export PACKAGE_NAME="${PACKAGE_NAME}"
export PACKAGE_VERSION="${PACKAGE_VERSION}"
export PACKAGE_SUBVERSION="${PACKAGE_SUBVERSION}" # in case we have this
export PACKAGE_REVISION="${PACKAGE_REVISION}"
export PACKAGE_ASSET_MINVERSION="${PACKAGE_ASSET_MINVERSION}"
export PACKAGE_ASSET_MAXVERSION="${PACKAGE_ASSET_MAXVERSION}"
export PACKAGE_INSTALL_PATH="${PACKAGE_INSTALL_PATH}"
export PACKAGE_LINK_PATH="${PACKAGE_LINK_PATH}"
export PACKAGE_DISTRO="${PACKAGE_DISTRO}"
export PACKAGE_URGENCY="${PACKAGE_URGENCY}"
export PACKAGE_NAME_EMAIL="${PACKAGE_NAME_EMAIL}"
export PACKAGE_GROUP_NAME_EMAIL="${PACKAGE_GROUP_NAME_EMAIL}"
export PACKAGE_WEBSITE="${PACKAGE_WEBSITE}"

export PROGRAM_NAME="${PROGRAM_NAME}"
export PROGRAM_VERSION="${PROGRAM_VERSION}"
export PROGRAM_DESCRIPTION="${PROGRAM_DESCRIPTION}"
export PROGRAM_FILENAME="${PROGRAM_FILENAME}"

# This file is called in debian_template.sh, so mark our completion so we don't run it again
__DEBIAN_PARAMETERS_INITIALIZED=1
