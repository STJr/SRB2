#!/bin/bash

# Deployer for Travis-CI
# Launchpad PPA uploader
#

if [[ "$__DEPLOYER_PPA_ACTIVE" == "1" ]]; then
    # Get the key to sign
    # Do this AFTER debuild so that we can specify the passphrase in command line
	echo "$DEPLOYER_PPA_KEY_PRIVATE" | base64 --decode > key.asc;
    echo "$DEPLOYER_PPA_KEY_PASSPHRASE" > phrase.txt;
	gpg --import key.asc;
    srm key.asc;

    echo "-- build";
    ls .;
    echo "-- SRB2";
    ls ..;
    echo "-- mazmazz";
    ls ../..;
    echo "-- assets";
    ls ../assets;

    if [[ "$PACKAGE_MAIN_NOBUILD" != "1" ]]; then
        OLDPWD=$PWD;
        PACKAGEFILENAME=${PACKAGE_NAME}_${PACKAGE_VERSION}~${PACKAGE_SUBVERSION};
        cd ../..; # level above repo root

        debsign -k ${DEPLOYER_PPA_KEY_FINGERPRINT} ${PACKAGEFILENAME}.dsc \
            -p'gpg --passphrase-file $OLDPWD/phrase.txt --batch --no-use-agent';
        debsign -k ${DEPLOYER_PPA_KEY_FINGERPRINT} ${PACKAGEFILENAME}.changes \
            -p'gpg --passphrase-file $OLDPWD/phrase.txt --batch --no-use-agent';

        dput ppa:${DEPLOYER_PPA_PATH} "${PACKAGEFILENAME}_source.changes";
        cd $OLDPWD;
    fi;

    if [[ "$PACKAGE_ASSET_BUILD" == "1" ]]; then
        OLDPWD=$PWD;
        PACKAGEFILENAME=${PACKAGE_NAME}-data_${PACKAGE_VERSION}~${PACKAGE_SUBVERSION};
        cd ..; # repo root

        debsign -k ${DEPLOYER_PPA_KEY_FINGERPRINT} ${PACKAGEFILENAME}.dsc \
            -p'gpg --passphrase-file $OLDPWD/phrase.txt --batch --no-use-agent';
        debsign -k ${DEPLOYER_PPA_KEY_FINGERPRINT} ${PACKAGEFILENAME}.changes \
            -p'gpg --passphrase-file $OLDPWD/phrase.txt --batch --no-use-agent';
        dput ppa:${DEPLOYER_PPA_PATH} "${PACKAGEFILENAME}_source.changes";
        cd $OLDPWD;
    fi;

    srm phrase.txt;
fi;
