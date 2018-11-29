#!/bin/bash

# Travis-CI Deployer
# Initialization
#
# Performs validity checks to ensure that Deployer is allowed to run
# e.g., is an FTP hostname specified? Are we whitelisted by OSNAMES and BRANCHES?
#
# Set these environment variables in your Travis-CI settings, where they are stored securely.
# See other shell scripts for more options.
#
# DEPLOYER_ENABLED = 1                       (leave blank to disable)
# DEPLOYER_JOB_ALL = 1                       (run Deployer on all jobs; leave blank to act on specific jobs, see below)
# DEPLOYER_OSNAMES = osx                     (whitelist of OS names to allow uploading; leave blank to upload from all OSes)
# DEPLOYER_BRANCHES = master,branch1,branch2 (whitelist of branches to upload; leave blank to upload all branches)
#
# To enable Deployer on specific jobs, set _DEPLOYER_JOB_ENABLED=1 for that job. Example:
# - matrix:
#   - os: osx
#     env:
#     - _DEPLOYER_JOB_ENABLED=1
#
# DO NOT set __DEPLOYER_ACTIVE, because that would bypass these validity checks.
#
# See below blocks for more configuration options.

# Validate Deployer state
if [[ "$DEPLOYER_ENABLED" == "1" ]]; then
	if [[ "$DEPLOYER_JOB_ALL" == "1" ]] || [[ "$_DEPLOYER_JOB_ENABLED" == "1" ]]; then
		if [[ "$DEPLOYER_OSNAMES" == "" ]] || [[ $DEPLOYER_OSNAMES == *"$TRAVIS_OS_NAME"* ]]; then
			if [[ "$TRAVIS_PULL_REQUEST" == "false" ]]; then
				if [[ "$DEPLOYER_BRANCHES" == "" ]] || [[ $DEPLOYER_BRANCHES == *"$TRAVIS_BRANCH"* ]]; then
                    # Base Deployer is eligible for becoming active
                    # Now check for sub-modules
                    if [[ "$_DEPLOYER_FTP_ENABLED" == "1" ]] && [[ "$DEPLOYER_FTP_HOSTNAME" != "" ]]; then
                        if [[ "$_DEPLOYER_FTP_PACKAGE" == "1" ]] || [[ "$_DEPLOYER_FTP_BINARY" == "1" ]]; then
                            echo "Deployer FTP target is enabled";
                            __DEPLOYER_FTP_ACTIVE=1;
                        else
                            echo "Developer FTP target cannot be enabled: You must specify _DEPLOYER_FTP_PACKAGE=1";
                            echo "and/or _DEPLOYER_FTP_BINARY=1 in your job's environment variables.";
                        fi;
                    fi;

                    if [[ "$_DEPLOYER_PPA_ENABLED" == "1" ]] && [[ "$TRAVIS_OS_NAME" == "linux" ]]; then
                        echo "Deployer PPA target is enabled";
                        __DEPLOYER_PPA_ACTIVE=1;
                    fi;

                    # If any sub-modules are active, then so is the main module
                    if [[ "$__DEPLOYER_FTP_ACTIVE" == "1" ]] || [[ "$__DEPLOYER_PPA_ACTIVE" == "1" ]]; then
					    __DEPLOYER_ACTIVE=1;
                    fi;
				fi;
			fi;
		fi;
	fi;
fi;

# Asset Paths
# If these are not in your settings, defaults below will be assigned.
#
# ASSET_ARCHIVE_PATH = http://example.com/assets.7z (path to single archive of assets. must be 7z.
#                                                    filenames in this archive will not be downloaded again, but are required for packaging)
# ASSET_BASE_PATH = http://example.com/path         (base URL to single asset downloads)
# ASSET_FILES_REQUIRED = file1.ext file2.ext        (required files in the build)
# ASSET_FILES_DOCS = README.txt LICENSE.txt         (documentation files; will not error out if not found, but will always be downloaded)
# ASSET_FILES_OPTIONAL = music.dta                  (optional files; will only be downloaded if ASSET_GET_OPTIONAL=1)
# ASSET_GET_OPTIONAL = 1                            (default is to NOT download optional files)

# Set variables for assets
${ASSET_ARCHIVE_PATH:=http://rosenthalcastle.org/srb2/SRB2-v2115-assets-2.7z}
${ASSET_BASE_PATH:=http://alam.srb2.org/SRB2/2.1.21-Final/Resources}
${ASSET_FILES_REQUIRED:=srb2.srb zones.dta player.dta rings.dta patch.dta}
${ASSET_FILES_DOCS:=README.txt LICENSE.txt LICENSE-3RD-PARTY.txt}
${ASSET_FILES_OPTIONAL:=music.dta}

# Package Parameters
#
# DEVELOPER_PACKAGE_ASSET_BUILD = 1 (build the data package, default is to skip)

${PACKAGE_NAME:=srb2}
${PACKAGE_ASSET_MINVERSION:=2.1.15}
${PACKAGE_INSTALL_PATH:=/usr/games/SRB2}
${PACKAGE_DISTRO:=trusty}
${PACKAGE_NAME_EMAIL:=Sonic Team Junior <stjr@srb2.org>}
${PACKAGE_GROUP_NAME_EMAIL:=Sonic Team Junior <stjr@srb2.org>}
${PACKAGE_WEBSITE:=<http://www.srb2.org>}

${PROGRAM_NAME:=Sonic Robo Blast 2}
${PROGRAM_VERSION:=2.1.21}
${PROGRAM_DESCRIPTION:=A free 3D Sonic the Hedgehog fangame closely inspired by the original Sonic games on the Sega Genesis.}
${PROGRAM_FILENAME:=srb2}

# This file is called in deployer_debian_template.sh, so mark our completion so we don't run it again
__DEBIAN_PARAMETERS_INITIALIZED=1
