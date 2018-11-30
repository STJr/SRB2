#!/bin/bash

# Deployer for Travis-CI
# FTP Uploader
#
# Package files are uploaded to, e.g., ftp://username:password@example.com:21/path/to/upload/STJr/SRB2/master/460873812-151.1
# With file `commit.txt` and folder(s) `bin` and `package`
#
# Set these environment variables in your Travis-CI settings, where they are stored securely.
# See other shell scripts for more options.
#
# DEPLOYER_FTP_PROTOCOL = ftp                    (ftp or sftp or ftps or however your FTP URI begins)
# DEPLOYER_FTP_USER = username
# DEPLOYER_FTP_PASS = password
# DEPLOYER_FTP_HOSTNAME = example.com
# DEPLOYER_FTP_PORT = 21
# DEPLOYER_FTP_PATH = path/to/upload             (do not add trailing slash)

if [[ "$__DEPLOYER_FTP_ACTIVE" == "1" ]]; then
	# Install wput if we don't already have it
	if [[ "$TRAVIS_OS_NAME" == "osx" ]]; then
		brew install wput;
	else
		if [[ "$TRAVIS_OS_NAME" == "linux" ]]; then
			sudo apt-get install wput;
		fi;
	fi;

	if [[ "$TRAVIS_JOB_NAME" != "" ]]; then
		JOBNAME=$TRAVIS_JOB_NAME;
	else
		if [[ "$_DEPLOYER_JOB_NAME" != "" ]]; then
			JOBNAME=$_DEPLOYER_JOB_NAME;
		else
			JOBNAME=$TRAVIS_OS_NAME;
		fi;
	fi;

	# Generate commit.txt file
	echo "Travis-CI Build $TRAVIS_OS_NAME - $TRAVIS_REPO_SLUG/$TRAVIS_BRANCH - $TRAVIS_JOB_NUMBER - $JOBNAME" > "commit.txt";
	echo "Job ID $TRAVIS_JOB_ID" >> "commit.txt";
	echo "" >> "commit.txt";
	echo "Commit $TRAVIS_COMMIT" >> "commit.txt";
	echo "$TRAVIS_COMMIT_MESSAGE" >> "commit.txt";
	echo "" >> "commit.txt";

	# Initialize FTP parameters
	if [[ "$DEPLOYER_FTP_PORT" == "" ]]; then
		DEPLOYER_FTP_PORT=21;
	fi;
	if [[ "$DEPLOYER_FTP_PROTOCOL" == "" ]]; then
		DEPLOYER_FTP_PROTOCOL=ftp;
	fi;
	#__DEPLOYER_FTP_LOCATION=$DEPLOYER_FTP_PROTOCOL://$DEPLOYER_FTP_USER:$DEPLOYER_FTP_PASS@$DEPLOYER_FTP_HOSTNAME:$DEPLOYER_FTP_PORT/$DEPLOYER_FTP_PATH/$TRAVIS_REPO_SLUG/$TRAVIS_BRANCH/$TRAVIS_JOB_ID-$TRAVIS_JOB_NUMBER-$JOBNAME;
	__DEPLOYER_FTP_LOCATION=$DEPLOYER_FTP_PROTOCOL://$DEPLOYER_FTP_HOSTNAME:$DEPLOYER_FTP_PORT/$DEPLOYER_FTP_PATH/$TRAVIS_REPO_SLUG/$TRAVIS_BRANCH/$TRAVIS_JOB_ID-$TRAVIS_JOB_NUMBER-$JOBNAME;

	# Upload to FTP!
	echo "Uploading to FTP...";
	#wput -v "commit.txt" "$__DEPLOYER_FTP_LOCATION/commit.txt";
	curl --ftp-create-dirs -T "commit.txt" -u $DEPLOYER_FTP_USER:$DEPLOYER_FTP_PASS  "$__DEPLOYER_FTP_LOCATION/commit.txt";

	if [[ "$__DEPLOYER_DEBIAN_ACTIVE" == "1" ]]; then
		if [[ "$PACKAGE_MAIN_NOBUILD" != "1" ]]; then
			PACKAGEFILENAME=${PACKAGE_NAME}_${PACKAGE_VERSION}~${PACKAGE_SUBVERSION};
			# Main packages are in parent of root repo folder
			OLDPWD=$PWD;
			cd ../..;
			for f in ./${PACKAGEFILENAME}*; do
				#wput -v "$f" "$__DEPLOYER_FTP_LOCATION/package/main/$f";
				curl --ftp-create-dirs -T "$f" -u $DEPLOYER_FTP_USER:$DEPLOYER_FTP_PASS  "$__DEPLOYER_FTP_LOCATION/package/main/$f";
			done;
			cd $OLDPWD;
		fi;

		if [[ "$PACKAGE_ASSET_BUILD" == "1" ]]; then
			PACKAGEFILENAME=${PACKAGE_NAME}-data_${PACKAGE_VERSION}~${PACKAGE_SUBVERSION};
			# Asset packages are in root repo folder
			OLDPWD=$PWD;
			cd ..;
			for f in ./${PACKAGEFILENAME}*; do
				#wput -v "$f" "$__DEPLOYER_FTP_LOCATION/package/asset/$f";
				curl --ftp-create-dirs -T "$f" -u $DEPLOYER_FTP_USER:$DEPLOYER_FTP_PASS  "$__DEPLOYER_FTP_LOCATION/package/asset/$f";
			done;
			cd $OLDPWD;
		fi;
	else
		if [[ "$_DEPLOYER_BINARY" == "1" ]]; then
			wput -v "bin" "$__DEPLOYER_FTP_LOCATION/";
		fi;

		# For some reason (permissions?), wput -v stalls when uploading "package" as a folder, so loop files manually
		if [[ "$_DEPLOYER_PACKAGE" == "1" ]]; then
			for f in package/*.*; do
				wput -v "$f" "$__DEPLOYER_FTP_LOCATION/";
			done;
		fi;
	fi;
fi
