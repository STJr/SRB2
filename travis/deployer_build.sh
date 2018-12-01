#!/bin/bash

# Deployer for Travis-CI
# Build Script
#
# Builds the required targets depending on which sub-modules are enabled

if [[ "$__DEPLOYER_FTP_ACTIVE" == "1" ]] || [[ "$__DEPLOYER_DPUT_ACTIVE" == "1" ]]; then
	if [[ "$__DEPLOYER_DEBIAN_ACTIVE" == "1" ]]; then
		echo "Building Debian package(s)"

		sudo apt-get install devscripts secure-delete;

		# Build source packages first, since they zip up the entire source folder,
		# binaries and all
		if [[ "$PACKAGE_MAIN_NOBUILD" != "1" ]]; then
			. ../debian_template.sh main;
			OLDPWD=$PWD; # [repo]/build
			cd ..; # repo root

			if [[ "$_DEPLOYER_PACKAGE" != "1" ]]; then
				echo "Building main source Debian package";
				debuild -S -us -uc;
			else
				echo "Building main source and binary Debian packages";
				debuild -us -uc;
			fi;

			cd $OLDPWD;
		fi;

		# Also an asset package
		if [[ "$PACKAGE_ASSET_BUILD" == "1" ]]; then
			. ../debian_template.sh asset;
			OLDPWD=$PWD; # [repo]/build
			cd ../assets;

			# make sure the asset files exist, download them if they don't
			debuild -T build;

			if [[ "$_DEPLOYER_PACKAGE" != "1" ]]; then
				echo "Building asset source Debian package";
				debuild -S -us -uc;
			else
				echo "Building asset source and binary Debian packages";
				debuild -us -uc;
			fi;

			cd $OLDPWD;
		fi;

		# Now sign our packages
		if [[ "$DEPLOYER_PGP_KEY_PRIVATE" != "" ]] && [[ "$DEPLOYER_PGP_KEY_PASSPHRASE" != "" ]]; then
			# Get the key to sign
			# Do this AFTER debuild so that we can specify the passphrase in command line
			echo "$DEPLOYER_PGP_KEY_PRIVATE" | base64 --decode > key.asc;
			echo "$DEPLOYER_PGP_KEY_PASSPHRASE" > phrase.txt;
			gpg --import key.asc;

			if [[ "$PACKAGE_MAIN_NOBUILD" != "1" ]]; then
				echo "Signing main package(s)";

				PACKAGEFILENAME=${PACKAGE_NAME}_${PACKAGE_VERSION}~${PACKAGE_SUBVERSION};
				PACKAGEDBGFILENAME=${PACKAGE_NAME}-dbg_${PACKAGE_VERSION}~${PACKAGE_SUBVERSION};
				#PACKAGENIGHTLYFILENAME=${PACKAGE_NAME}-nightly_${PACKAGE_VERSION}~${PACKAGE_SUBVERSION};
				#PACKAGENIGHTLYDBGFILENAME=${PACKAGE_NAME}-nightly-dbg_${PACKAGE_VERSION}~${PACKAGE_SUBVERSION};
				#PACKAGEPATCHFILENAME=${PACKAGE_NAME}-patch_${PACKAGE_VERSION}~${PACKAGE_SUBVERSION};
				#PACKAGEPATCHDBGFILENAME=${PACKAGE_NAME}-patch-dbg_${PACKAGE_VERSION}~${PACKAGE_SUBVERSION};
				#PACKAGEPATCHNIGHTLYFILENAME=${PACKAGE_NAME}-patch-nightly_${PACKAGE_VERSION}~${PACKAGE_SUBVERSION};
				#PACKAGEPATCHNIGHTLYDBGFILENAME=${PACKAGE_NAME}-patch-nightly-dbg_${PACKAGE_VERSION}~${PACKAGE_SUBVERSION};

				PACKAGEFILENAMES=(
					PACKAGEFILENAME
					PACKAGEDBGFILENAME
					#PACKAGENIGHTLYFILENAME
					#PACKAGENIGHTLYDBGFILENAME
					#PACKAGEPATCHFILENAME
					#PACKAGEPATCHDBGFILENAME
					#PACKAGEPATCHNIGHTLYFILENAME
					#PACKAGEPATCHNIGHTLYDBGFILENAME
				);

				# Main packages are in parent of root repo folder
				OLDPWD=$PWD; # [repo]/build
				cd ../..; # parent of repo root

				for n in ${PACKAGEFILENAMES}; do
					for f in ./$n*.changes; do
						debsign "$f" \
							-p"gpg --passphrase-file $OLDPWD/phrase.txt --batch";
					done;
				done;

				cd $OLDPWD;
			fi;

			if [[ "$PACKAGE_ASSET_BUILD" == "1" ]]; then
				echo "Signing asset package(s)";

				PACKAGEFILENAME=${PACKAGE_NAME}-data_${PACKAGE_VERSION}~${PACKAGE_SUBVERSION};
				#PACKAGENIGHTLYFILENAME=${PACKAGE_NAME}-nightly-data_${PACKAGE_VERSION}~${PACKAGE_SUBVERSION};
				#PACKAGEPATCHFILENAME=${PACKAGE_NAME}-patch-data_${PACKAGE_VERSION}~${PACKAGE_SUBVERSION};
				#PACKAGEPATCHNIGHTLYFILENAME=${PACKAGE_NAME}-patch-nightly-data_${PACKAGE_VERSION}~${PACKAGE_SUBVERSION};

				PACKAGEFILENAMES=(
					PACKAGEFILENAME
					#PACKAGENIGHTLYFILENAME
					#PACKAGEPATCHFILENAME
					#PACKAGEPATCHNIGHTLYFILENAME
				)

				# Asset packages are in root repo folder
				OLDPWD=$PWD; # [repo]/build
				cd ..; # repo root

				for n in ${PACKAGEFILENAMES}; do
					for f in ./$n*.changes; do
						debsign "$f" \
							-p"gpg --passphrase-file $OLDPWD/phrase.txt --batch";
					done;
				done;

				cd $OLDPWD;
			fi;

			# Delete the keys :eyes:
			srm key.asc;
			srm phrase.txt;
		fi;
	fi;

	# all other OSes
	if [[ "$TRAVIS_OS_NAME" != "linux" ]]; then
		#
		# Check for binary building
		#
		if [[ "$_DEPLOYER_BINARY" == "1" ]]; then
			echo "Building a Binary";
			make -k;
		fi;

		#
		# Check for package building
		#
		if [[ "$_DEPLOYER_PACKAGE" == "1" ]]; then
			echo "Building a Package";

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
				# Some day, when Windows is supported, we'll just make a standard package
				make -k package;
			fi;
		fi;
	fi;
fi;
