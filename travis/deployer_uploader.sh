#!/bin/bash

# Travis-CI Deployer
# FTP Uploader
#
# Package files are uploaded to, e.g., ftp://username:password@example.com:21/path/to/upload/STJr/SRB2/master/460873812-151.1
# With file `commit.txt` and folder(s) `bin` and `package`
#
# Set these environment variables in your Travis-CI settings, where they are stored securely.
# See other shell scripts for more options.
#
# DEPLOYER_FTP_BINARY = 1                        (upload bin/ folder; leave blank to disable)
# DEPLOYER_FTP_NOPACKAGE = 1                     (do NOT upload package/ folder; leave blank to upload it)
#
# DEPLOYER_FTP_PROTOCOL = ftp                    (ftp or sftp or ftps or however your FTP URI begins)
# DEPLOYER_FTP_USER = username
# DEPLOYER_FTP_PASS = password
# DEPLOYER_FTP_HOSTNAME = example.com
# DEPLOYER_FTP_PORT = 21
# DEPLOYER_FTP_PATH = path/to/upload             (do not add trailing slash)

if [[ "$__DEPLOYER_ACTIVE" == "1" ]]; then
	# Install wput if we don't already have it
	if [[ "$TRAVIS_OS_NAME" == "osx" ]]; then
		brew install wput;
	fi;

	# Generate commit.txt file
	echo "Travis-CI Build $TRAVIS_OS_NAME - $TRAVIS_REPO_SLUG/$TRAVIS_BRANCH - $TRAVIS_JOB_NUMBER - $TRAVIS_JOB_NAME" > "commit.txt";
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
	__DEPLOYER_FTP_LOCATION=$DEPLOYER_FTP_PROTOCOL://$DEPLOYER_FTP_USER:$DEPLOYER_FTP_PASS@$DEPLOYER_FTP_HOSTNAME:$DEPLOYER_FTP_PORT/$DEPLOYER_FTP_PATH/$TRAVIS_REPO_SLUG/$TRAVIS_BRANCH/$TRAVIS_JOB_ID-$TRAVIS_JOB_NUMBER;

	# Upload to FTP!
	echo "Uploading to FTP...";
	wput "commit.txt" "$__DEPLOYER_FTP_LOCATION/commit.txt";

	if [[ "$DEPLOYER_FTP_BINARY" == "1" ]]; then
		wput "bin" "$__DEPLOYER_FTP_LOCATION/";
	fi;

	# For some reason (permissions?), wput stalls when uploading "package" as a folder, so loop files manually
	if [[ "$DEPLOYER_FTP_NOPACKAGE" != "1" ]]; then
		for f in package/*.*; do
			wput "$f" "$__DEPLOYER_FTP_LOCATION/";
		done;
	fi;
fi
