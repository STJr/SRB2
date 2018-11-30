#!/bin/bash

# Deployer for Travis-CI
# Launchpad PPA uploader
#

if [[ "$__DEPLOYER_DPUT_ACTIVE" == "1" ]]; then
    if [[ "$PACKAGE_MAIN_NOBUILD" != "1" ]]; then
        OLDPWD=$PWD;
        PACKAGEFILENAME=${PACKAGE_NAME}_${PACKAGE_VERSION}~${PACKAGE_SUBVERSION};
        cd ../..; # level above repo root

        # Attempted FTP upload instead of dput
        # LAUNCHPADFTP="ftp://ppa.launchpad.net:21/~${DEPLOYER_DPUT_PATH}/"
        # wput ./${PACKAGEFILENAME}.dsc ./${PACKAGEFILENAME}.tar.xz \
        #     ./${PACKAGEFILENAME}_source.buildinfo ./${PACKAGEFILENAME}_source.changes \
        #     ${LAUNCHPADFTP}${PACKAGEFILENAME}.dsc ${LAUNCHPADFTP}${PACKAGEFILENAME}.tar.xz \
        #     ${LAUNCHPADFTP}${PACKAGEFILENAME}_source.buildinfo ${LAUNCHPADFTP}${PACKAGEFILENAME}_source.changes;

        dput ${DEPLOYER_DPUT_PATH} "${PACKAGEFILENAME}_source.changes";

        cd $OLDPWD;
    fi;

    if [[ "$PACKAGE_ASSET_BUILD" == "1" ]]; then
        OLDPWD=$PWD;
        PACKAGEFILENAME=${PACKAGE_NAME}-data_${PACKAGE_VERSION}~${PACKAGE_SUBVERSION};
        cd ..; # repo root

        # Attempted FTP upload instead of dput
        # LAUNCHPADFTP="ftp://ppa.launchpad.net:21/~${DEPLOYER_DPUT_PATH}/"
        # wput ./${PACKAGEFILENAME}.dsc ./${PACKAGEFILENAME}.tar.xz \
        #     ./${PACKAGEFILENAME}_source.buildinfo ./${PACKAGEFILENAME}_source.changes \
        #     ${LAUNCHPADFTP}${PACKAGEFILENAME}.dsc ${LAUNCHPADFTP}${PACKAGEFILENAME}.tar.xz \
        #     ${LAUNCHPADFTP}${PACKAGEFILENAME}_source.buildinfo ${LAUNCHPADFTP}${PACKAGEFILENAME}_source.changes;

        dput ${DEPLOYER_DPUT_PATH} "${PACKAGEFILENAME}_source.changes";

        cd $OLDPWD;
    fi;
fi;
