#!/bin/bash

# Deployer for Travis-CI
# Asset Paths
#
# If these are not in your settings, defaults below will be assigned.
#
# ASSET_ARCHIVE_PATH = http://example.com/assets.7z (path to single archive of assets. must be 7z.
#                                                    you should set the default filenames below to blank if
#                                                    those files are in the archive)
# ASSET_BASE_PATH = http://example.com/path         (base URL to single asset downloads)
# ASSET_FILES_REQUIRED = file1.ext file2.ext        (required files in the build)
# ASSET_FILES_DOCS = README.txt LICENSE.txt         (documentation files; will not error out if not found, but will always be downloaded)
# ASSET_FILES_OPTIONAL = music.dta                  (optional files; will only be downloaded if ASSET_GET_OPTIONAL=1
#                                                    note that these will NOT be copied to cache, and will always be downloaded.)
# ASSET_GET_OPTIONAL = 1                            (default is to NOT download optional files)

# Set variables for assets
: ${ASSET_ARCHIVE_PATH:=http://rosenthalcastle.org/srb2/SRB2-v2115-assets-2.7z}
: ${ASSET_BASE_PATH:=http://alam.srb2.org/SRB2/2.1.21-Final/Resources}
: ${ASSET_FILES_REQUIRED:=srb2.srb zones.dta player.dta rings.dta patch.dta}
: ${ASSET_FILES_DOCS:=README.txt LICENSE.txt LICENSE-3RD-PARTY.txt}
: ${ASSET_FILES_OPTIONAL:=music.dta}

# Package Parameters
#
# PACKAGE_ASSET_BUILD = 1 (build the data package, default is to skip)

: ${PACKAGE_NAME:=srb2}
: ${PACKAGE_VERSION:=2.1.21}
: ${PACKAGE_ASSET_MINVERSION:=2.1.15}
: ${PACKAGE_INSTALL_PATH:=/usr/games/SRB2}
: ${PACKAGE_DISTRO:=trusty}
: ${PACKAGE_URGENCY:=high}
: ${PACKAGE_NAME_EMAIL:=Sonic Team Junior <stjr@srb2.org>}
: ${PACKAGE_GROUP_NAME_EMAIL:=Sonic Team Junior <stjr@srb2.org>}
: ${PACKAGE_WEBSITE:=<http://www.srb2.org>}

: ${PROGRAM_NAME:=Sonic Robo Blast 2}
: ${PROGRAM_VERSION:=2.1.21}
: ${PROGRAM_DESCRIPTION:=A free 3D Sonic the Hedgehog fangame closely inspired by the original Sonic games on the Sega Genesis.}
: ${PROGRAM_FILENAME:=srb2}

# If not running on travis, this is later set to the current datetime
# if [[ "$TRAVIS_JOB_ID" != "" ]]; then
#     : ${PACKAGE_SUBVERSION:=$TRAVIS_JOB_ID};
# fi;

# This file is called in debian_template.sh, so mark our completion so we don't run it again
__DEBIAN_PARAMETERS_INITIALIZED=1
