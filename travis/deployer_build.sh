#!/bin/bash

# Travis-CI Deployer
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

		if [[ "$TRAVIS_OS_NAME" == "osx" ]]; then
			# Make an OSX package; superuser is required for library bundling
			sudo make -k package;
		else if [[ "$TRAVIS_OS_NAME" == "linux" ]]; then
			# Make a Debian package
			# TODO support other packages like RPM
			debuild;
		else
			# Some day, when Windows is supported, we'll just make a standard package
			make -k package;
		fi;
	fi;
fi;

if [[ "$__DEPLOYER_PPA_ACTIVE" == "1" ]]; then
	echo "Building a Source Package for PPA";

	# Make a source package for PPA
	debuild -S;
fi;
