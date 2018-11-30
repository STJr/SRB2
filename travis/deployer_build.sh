#!/bin/bash

# Deployer for Travis-CI
# Build Script
#
# Builds the required targets depending on which sub-modules are enabled

# Get devscripts if we don't already have it
if [[ "$__DEPLOYER_PPA_ACTIVE" == "1" ]]; then
	sudo apt-get install devscripts;
else
	if [[ "$__DEPLOYER_FTP_ACTIVE" == "1" ]] && [[ "$_DEPLOYER_FTP_PACKAGE" == "1" ]] && [[ "$TRAVIS_OS_NAME" == "linux" ]]; then
		sudo apt-get install devscripts;
	fi;
fi;

# Build source packages first, since they zip up the entire source folder,
# binaries and all
if [[ "$__DEPLOYER_PPA_ACTIVE" == "1" ]]; then
	echo "Building a Source Package for PPA";

	# Get the key to sign
    #gpg --keyserver hkp://keyserver.ubuntu.com:80 --recv-keys ${DEPLOYER_PPA_KEY_FINGERPRINT}
	echo "$DEPLOYER_PPA_KEY_PRIVATE" | base64 --decode > key.asc
	gpg --import key.asc

	# Make a source package for PPA
	if [[ "$PACKAGE_MAIN_NOBUILD" != "1" ]]; then
		echo "Building main source Debian package";
		. ../debian_template.sh main;
		OLDPWD=$PWD;
		cd ..;

		/usr/bin/expect <(cat << EOF
spawn debuild -S
expect "Enter passphrase:"
send "${DEPLOYER_PPA_KEY_PASSPHRASE}\r"
interact
EOF
);

		cd $OLDPWD;
	fi;
	if [[ "$PACKAGE_ASSET_BUILD" == "1" ]]; then
		echo "Building asset source Debian package";
		. ../debian_template.sh asset;
		OLDPWD=$PWD;
		cd ../assets;

		debuild -T build; # make sure the asset files exist

		/usr/bin/expect <(cat << EOF
spawn debuild -S
expect "Enter passphrase:"
send "${DEPLOYER_PPA_KEY_PASSPHRASE}\r"
interact
EOF
);

		cd $OLDPWD;
	fi;
fi;

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
				# TODO x86 cross compile?
				if [[ "$PACKAGE_MAIN_NOBUILD" != "1" ]]; then
					echo "Building main binary Debian package";
					. ../debian_template.sh main;
					OLDPWD=$PWD;
					cd ..;
					debuild;
					cd $OLDPWD;
				fi;
				if [[ "$PACKAGE_ASSET_BUILD" == "1" ]]; then
					echo "Building asset binary Debian package";
					. ../debian_template.sh asset;
					OLDPWD=$PWD;
					cd ../assets;
					debuild -T build;
					debuild;
					cd $OLDPWD;
				fi;
			else
				# Some day, when Windows is supported, we'll just make a standard package
				make -k package;
			fi;
		fi;
	fi;
fi;
