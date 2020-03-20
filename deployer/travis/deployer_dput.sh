#!/bin/bash

# Deployer for Travis-CI
# DPUT uploader (e.g., Launchpad PPA)
#

if [[ "$__DPL_DPUT_ACTIVE" == "1" ]]; then
    # Install APT dependencies
    # paramiko required for ssh
    sudo apt-get install python-paramiko expect dput; # python-pip
    #pip install paramiko;

    # Output the DPUT config
    # Dput only works if you're using secure FTP, so that's what we default to.
    cat > "./dput.cf" << EOM
[deployer]
fqdn = ${DPL_DPUT_DOMAIN}
method = ${DPL_DPUT_METHOD}
incoming = ${DPL_DPUT_INCOMING}
login = ${DPL_DPUT_LOGIN}
allow_unsigned_uploads = 0
EOM

    # Output SSH config
    # Don't let SSH prompt us for untrusted hosts
    cat >> "./ssh_config" << EOM

Host *
    StrictHostKeyChecking no
    UserKnownHostsFile=/dev/null
    PubKeyAuthentication yes
    IdentityFile ${PWD}/key.private
    IdentitiesOnly yes
EOM
    sudo sh -c "cat < ${PWD}/ssh_config >> /etc/ssh/ssh_config";

    # Get the private key
    echo "$DPL_SSH_KEY_PRIVATE" | base64 --decode > key.private;
    chmod 700 ./key.private;

    if [[ "$_DPL_PACKAGE_MAIN" == "1" ]]; then
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
        cd ../..;

        # Enter passphrase if required
        for n in ${PACKAGEFILENAMES}; do
            for f in $n*.changes; do
                # Binary builds also generate source builds, so exclude the source
                # builds if desired
                if [[ "$_DPL_PACKAGE_SOURCE" != "1" ]]; then
                    if [[ "$f" == *"_source"* ]] || [[ "$f" == *".tar.xz"* ]]; then
                        continue;
                    fi;
                fi;

                expect <(cat <<EOD
spawn dput -c "${OLDPWD}/dput.cf" deployer "$f";
expect "Enter passphrase for key"
send "${DPL_SSH_KEY_PASSPHRASE}\r"
interact
EOD
);
            done;
        done;

        # Go back to [repo]/build folder
        cd $OLDPWD;
    fi;

    if [[ "$_DPL_PACKAGE_ASSET" == "1" ]]; then
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
        cd ..;

        # Enter passphrase if required
        for n in ${PACKAGEFILENAMES}; do
            for f in $n*.changes; do
                # Binary builds also generate source builds, so exclude the source
                # builds if desired
                if [[ "$_DPL_PACKAGE_SOURCE" != "1" ]]; then
                    if [[ "$f" == *"_source"* ]] || [[ "$f" == *".tar.xz"* ]]; then
                        continue;
                    fi;
                fi;
                expect <(cat <<EOD
spawn dput -c "${OLDPWD}/dput.cf" deployer "$f";
expect "Enter passphrase for key"
send "${DPL_SSH_KEY_PASSPHRASE}\r"
interact
EOD
);
            done;
        done;

        # Go back to [repo]/build folder
        cd $OLDPWD;
    fi;

    srm ./key.private;
fi;
