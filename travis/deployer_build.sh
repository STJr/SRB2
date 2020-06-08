#!/bin/bash

# Deployer for Travis-CI
# Build Script for Debian packages
#
# NOTE: RETAINED FOR REFERENCE; NOT CURRENTLY USED

if [[ "$__DPL_DEBIAN_ACTIVE" == "1" ]]; then
	echo "Building Debian package(s)"

	sudo apt-get install devscripts debhelper fakeroot secure-delete expect;

	# Build source packages first, since they zip up the entire source folder,
	# binaries and all
	if [[ "$_DPL_PACKAGE_MAIN" == "1" ]]; then
		. ../debian_template.sh main;
		OLDPWD=$PWD; # [repo]/build
		cd ..; # repo root

		if [[ "$_DPL_PACKAGE_SOURCE" == "1" ]]; then
			echo "Building main source Debian package";
			expect <(cat <<EOD
spawn debuild -S -us -uc;
expect "continue anyway? (y/n)"
send "y\r"
interact
EOD
);
		fi;

		if [[ "$_DPL_PACKAGE_BINARY" == "1" ]]; then
			echo "Building main binary Debian package";
			expect <(cat <<EOD
spawn debuild -us -uc;
expect "continue anyway? (y/n)"
send "y\r"
interact
EOD
);
		fi;

		cd $OLDPWD;
	fi;

	# Also an asset package
	if [[ "$_DPL_PACKAGE_ASSET" == "1" ]]; then
		. ../debian_template.sh asset;
		OLDPWD=$PWD; # [repo]/build
		cd ../assets;

		# make sure the asset files exist, download them if they don't
		#echo "Checking asset files for asset Debian package";
		#debuild -T build;

		if [[ "$_DPL_PACKAGE_SOURCE" == "1" ]]; then
			echo "Building asset source Debian package";
			expect <(cat <<EOD
spawn debuild -S -us -uc;
expect "continue anyway? (y/n)"
send "y\r"
interact
EOD
);
		fi;

		if [[ "$_DPL_PACKAGE_BINARY" == "1" ]]; then
			echo "Building asset binary Debian package";
			expect <(cat <<EOD
spawn debuild -us -uc;
expect "continue anyway? (y/n)"
send "y\r"
interact
EOD
);
		fi;

		cd $OLDPWD;
	fi;

	# Now sign our packages
	if [[ "$DPL_PGP_KEY_PRIVATE" != "" ]] && [[ "$DPL_PGP_KEY_PASSPHRASE" != "" ]]; then
		# Get the key to sign
		# Do this AFTER debuild so that we can specify the passphrase in command line
		echo "$DPL_PGP_KEY_PRIVATE" | base64 --decode > key.asc;
		echo "$DPL_PGP_KEY_PASSPHRASE" > phrase.txt;
		gpg --import key.asc;

		if [[ "$_DPL_PACKAGE_MAIN" == "1" ]]; then
			echo "Signing main package(s)";

			PACKAGEFILENAME=${PACKAGE_NAME}_${PACKAGE_VERSION}${PACKAGE_SUBVERSION}${PACKAGE_REVISION};
			PACKAGEDBGFILENAME=${PACKAGE_NAME}-dbg_${PACKAGE_VERSION}${PACKAGE_SUBVERSION}${PACKAGE_REVISION};
			#PACKAGENIGHTLYFILENAME=${PACKAGE_NAME}-nightly_${PACKAGE_VERSION}${PACKAGE_SUBVERSION}${PACKAGE_REVISION};
			#PACKAGENIGHTLYDBGFILENAME=${PACKAGE_NAME}-nightly-dbg_${PACKAGE_VERSION}${PACKAGE_SUBVERSION}${PACKAGE_REVISION};
			#PACKAGEPATCHFILENAME=${PACKAGE_NAME}-patch_${PACKAGE_VERSION}${PACKAGE_SUBVERSION}${PACKAGE_REVISION};
			#PACKAGEPATCHDBGFILENAME=${PACKAGE_NAME}-patch-dbg_${PACKAGE_VERSION}${PACKAGE_SUBVERSION}${PACKAGE_REVISION};
			#PACKAGEPATCHNIGHTLYFILENAME=${PACKAGE_NAME}-patch-nightly_${PACKAGE_VERSION}${PACKAGE_SUBVERSION}${PACKAGE_REVISION};
			#PACKAGEPATCHNIGHTLYDBGFILENAME=${PACKAGE_NAME}-patch-nightly-dbg_${PACKAGE_VERSION}${PACKAGE_SUBVERSION}${PACKAGE_REVISION};

			PACKAGEFILENAMES=(
				$PACKAGEFILENAME
				$PACKAGEDBGFILENAME
				#$PACKAGENIGHTLYFILENAME
				#$PACKAGENIGHTLYDBGFILENAME
				#$PACKAGEPATCHFILENAME
				#$PACKAGEPATCHDBGFILENAME
				#$PACKAGEPATCHNIGHTLYFILENAME
				#$PACKAGEPATCHNIGHTLYDBGFILENAME
			);

			# Main packages are in parent of root repo folder
			OLDPWD=$PWD; # [repo]/build
			cd ../..; # parent of repo root

			for n in ${PACKAGEFILENAMES}; do
				for f in ./$n*.changes; do
					debsign --no-re-sign -p"gpg --passphrase-file $OLDPWD/phrase.txt --batch" "$f";
				done;
			done;

			cd $OLDPWD;
		fi;

		if [[ "$_DPL_PACKAGE_ASSET" == "1" ]]; then
			echo "Signing asset package(s)";

			PACKAGEFILENAME=${PACKAGE_NAME}-data_${PACKAGE_VERSION}${PACKAGE_SUBVERSION}${PACKAGE_REVISION};
			#PACKAGENIGHTLYFILENAME=${PACKAGE_NAME}-nightly-data_${PACKAGE_VERSION}${PACKAGE_SUBVERSION}${PACKAGE_REVISION};
			#PACKAGEPATCHFILENAME=${PACKAGE_NAME}-patch-data_${PACKAGE_VERSION}${PACKAGE_SUBVERSION}${PACKAGE_REVISION};
			#PACKAGEPATCHNIGHTLYFILENAME=${PACKAGE_NAME}-patch-nightly-data_${PACKAGE_VERSION}${PACKAGE_SUBVERSION}${PACKAGE_REVISION};

			PACKAGEFILENAMES=(
				$PACKAGEFILENAME
				#$PACKAGENIGHTLYFILENAME
				#$PACKAGEPATCHFILENAME
				#$PACKAGEPATCHNIGHTLYFILENAME
			)

			# Asset packages are in root repo folder
			OLDPWD=$PWD; # [repo]/build
			cd ..; # repo root

			for n in ${PACKAGEFILENAMES}; do
				for f in ./$n*.changes; do
					debsign --no-re-sign -p"gpg --passphrase-file $OLDPWD/phrase.txt --batch" "$f";
				done;
			done;

			cd $OLDPWD;
		fi;

		# Delete the keys :eyes:
		srm key.asc;
		srm phrase.txt;
	fi;
fi;
