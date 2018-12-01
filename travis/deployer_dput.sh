#!/bin/bash

# Deployer for Travis-CI
# DPUT uploader (e.g., Launchpad PPA)
#

if [[ "$__DEPLOYER_DPUT_ACTIVE" == "1" ]]; then
    # Output the DPUT config
    # Dput only works if you're using secure FTP, so that's what we default to.
    cat > "./dput.cf" << EOM
[deployer]
fqdn = ${DEPLOYER_DPUT_DOMAIN}
method = ${DEPLOYER_DPUT_METHOD}
incoming = ${DEPLOYER_DPUT_INCOMING}
login = ${DEPLOYER_DPUT_USER}
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
    echo "$DEPLOYER_SSH_KEY_PRIVATE" | base64 --decode > key.private;
    chmod 700 ./key.private;

    # paramiko required for ssh
    sudo apt-get install python-paramiko expect; # python-pip
    #pip install paramiko;

    if [[ "$PACKAGE_MAIN_NOBUILD" != "1" ]]; then
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
        cd ../..;

        # Enter passphrase if required
        for n in ${PACKAGEFILENAMES}; do
            for f in $n*.changes; do
                # Binary builds also generate source builds, so exclude the source
                # builds if desired
                if [[ "$_DEPLOYER_SOURCEPACKAGE" != "1" ]]; then
                    if [[ "$f" == *"_source"* ]] || [[ "$f" == *".tar.xz"* ]]; then
                        continue;
                    fi;
                fi;

                expect <(cat <<EOD
spawn dput -c "${OLDPWD}/dput.cf" deployer "$f";
expect "Enter passphrase for key"
send "${DEPLOYER_SSH_KEY_PASSPHRASE}\r"
interact
EOD
);
            done;
        done;

        # Go back to [repo]/build folder
        cd $OLDPWD;
    fi;

    if [[ "$PACKAGE_ASSET_BUILD" == "1" ]]; then
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
        cd ..;

        # Enter passphrase if required
        for n in ${PACKAGEFILENAMES}; do
            for f in $n*.changes; do
                # Binary builds also generate source builds, so exclude the source
                # builds if desired
                if [[ "$_DEPLOYER_SOURCEPACKAGE" != "1" ]]; then
                    if [[ "$f" == *"_source"* ]] || [[ "$f" == *".tar.xz"* ]]; then
                        continue;
                    fi;
                fi;
                expect <(cat <<EOD
spawn dput -c "${OLDPWD}/dput.cf" deployer "$f";
expect "Enter passphrase for key"
send "${DEPLOYER_SSH_KEY_PASSPHRASE}\r"
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
