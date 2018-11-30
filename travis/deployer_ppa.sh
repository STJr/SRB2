#!/bin/bash

# Deployer for Travis-CI
# Launchpad PPA uploader
#

if [[ "$__DEPLOYER_PPA_ACTIVE" == "1" ]]; then
    # Get the key to sign
    gpg --keyserver hkp://keyserver.ubuntu.com:80 --recv-keys ${DEPLOYER_PPA_KEY_FINGERPRINT}

    if [[ "$PACKAGE_MAIN_NOBUILD" != "1" ]]; then
        OLDPWD=$PWD
        PACKAGEFILENAME=${PACKAGE_NAME}_${PACKAGE_VERSION}-${PACKAGE_SUBVERSION}
        cd ../.. # level above repo root

        debsign -k ${DEPLOYER_PPA_KEY_FINGERPRINT} ${PACKAGEFILENAME}.dsc
        debsign -k ${DEPLOYER_PPA_KEY_FINGERPRINT} ${PACKAGEFILENAME}.changes
        dput ppa:${DEPLOYER_PPA_PATH} "${PACKAGEFILENAME}_source.changes"
        cd $OLDPWD
    fi;

    if [[ "$PACKAGE_ASSET_BUILD" == "1" ]]; then
        OLDPWD=$PWD
        PACKAGEFILENAME=${PACKAGE_NAME}-data_${PACKAGE_VERSION}-${PACKAGE_SUBVERSION}
        cd .. # repo root

        debsign -k ${DEPLOYER_PPA_KEY_FINGERPRINT} ${PACKAGEFILENAME}.dsc
        debsign -k ${DEPLOYER_PPA_KEY_FINGERPRINT} ${PACKAGEFILENAME}.changes
        dput ppa:${DEPLOYER_PPA_PATH} "${PACKAGEFILENAME}_source.changes"
        cd $OLDPWD
    fi;
fi;
