#!/bin/bash

# Deployer for Travis-CI
# Build Script
#
# Builds the required targets depending on which sub-modules are enabled

if [[ "$__DEPLOYER_FTP_ACTIVE" == "1" ]]; then
	# Check for binary building
	if [[ "$_DEPLOYER_FTP_BINARY" == "1" ]]; then
		echo "Building a Binary for FTP";
		make -k;
	fi;

	# Check for package building
	if [[ "$_DEPLOYER_FTP_PACKAGE" == "1" ]]; then
		echo "Building a Package for FTP";

		# Make an OSX package; superuser is required for library bundling
		#
		# HACK: OSX packaging can't write libraries to .app package unless we're superuser
		# because the original library files don't have WRITE permission
		# Bug may be sidestepped by using CHMOD_BUNDLE_ITEMS=TRUE
		# But I don't know where this is set. Not `cmake -D...` because this var is ignored.
		# https://cmake.org/Bug/view.php?id=9284
		if [[ "$TRAVIS_OS_NAME" == "osx" ]]; then
			sudo make -k package;
		else
			if [[ "$TRAVIS_OS_NAME" == "linux" ]]; then
				# Make a Debian package
				# TODO support other packages like RPM
				debuild;
			else
				# Some day, when Windows is supported, we'll just make a standard package
				make -k package;
			fi;
		fi;
	fi;
fi;

if [[ "$__DEPLOYER_PPA_ACTIVE" == "1" ]]; then
	echo "Building a Source Package for PPA";

	# Make a source package for PPA
	debuild -S;
fi;
