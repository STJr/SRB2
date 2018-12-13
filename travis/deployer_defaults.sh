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
# ASSET_FILES_OPTIONAL = music.dta                  (optional files; will only be downloaded if ASSET_OPTIONAL_GET=1
#                                                    note that these will NOT be copied to cache, and will always be downloaded.)
# ASSET_OPTIONAL_GET = 1                            (default is to NOT download optional files)

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
: ${PACKAGE_SUBVERSION} # configured in debian_template.sh
: ${PACKAGE_REVISION} # configured in debian_template.sh
: ${PACKAGE_ASSET_MINVERSION:=2.1.15}
: ${PACKAGE_INSTALL_PATH:=/usr/games/SRB2}
: ${PACKAGE_LINK_PATH:=/usr/games}
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

# DPUT Variables
: ${DEPLOYER_DPUT_DOMAIN:=ppa.launchpad.net}
: ${DEPLOYER_DPUT_METHOD:=sftp}
: ${DEPLOYER_DPUT_INCOMING}
: ${DEPLOYER_DPUT_USER:=anonymous}

# Export all variables for envsubst templating

export ASSET_ARCHIVE_PATH="${ASSET_ARCHIVE_PATH}"
export ASSET_BASE_PATH="${ASSET_BASE_PATH}"
export ASSET_FILES_REQUIRED="${ASSET_FILES_REQUIRED}"
export ASSET_FILES_DOCS="${ASSET_FILES_DOCS}"
export ASSET_FILES_OPTIONAL="${ASSET_FILES_OPTIONAL}"

export PACKAGE_NAME="${PACKAGE_NAME}"
export PACKAGE_VERSION="${PACKAGE_VERSION}"
export PACKAGE_SUBVERSION="${PACKAGE_SUBVERSION}" # in case we have this
export PACKAGE_REVISION="${PACKAGE_REVISION}"
export PACKAGE_ASSET_MINVERSION="${PACKAGE_ASSET_MINVERSION}"
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
