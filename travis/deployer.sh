#!/bin/bash

# Deployer for Travis-CI
# Initialization
#
# Performs validity checks to ensure that Deployer is allowed to run
# e.g., is an FTP hostname specified? Are we whitelisted by OSNAMES and BRANCHES?
#
# Set these environment variables in your Travis-CI settings, where they are stored securely.
# See other shell scripts for more options.
#
# DEPLOYER_ENABLED = 1                       (leave blank to disable)
# DEPLOYER_JOB_ALL = 1                       (run Deployer on all jobs; leave blank to act on specific jobs, see below)
# DEPLOYER_OSNAMES = osx                     (whitelist of OS names to allow uploading; leave blank to upload from all OSes)
# DEPLOYER_BRANCHES = master,branch1,branch2 (whitelist of branches to upload; leave blank to upload all branches)
#
# To enable Deployer on specific jobs, set _DEPLOYER_JOB_ENABLED=1 for that job. Example:
# - matrix:
#   - os: osx
#     env:
#     - _DEPLOYER_JOB_ENABLED=1
#
# DO NOT set __DEPLOYER_ACTIVE, because that would bypass these validity checks.

# Validate Deployer state
if [[ "$DEPLOYER_ENABLED" == "1" ]] && [[ "$TRAVIS_PULL_REQUEST" == "false" ]]; then
    # Logging message for trigger word
    if [[ "$DEPLOYER_TRIGGER" != "" ]]; then
        echo "Testing for trigger $DEPLOYER_TRIGGER, commit message: $TRAVIS_COMMIT_MESSAGE";
        echo "[${DEPLOYER_TRIGGER}]";
        echo "[${DEPLOYER_TRIGGER}-${_DEPLOYER_JOB_NAME}]";
        echo "[${DEPLOYER_TRIGGER}-${TRAVIS_OS_NAME}]";
    fi;

    #
    # Search for the trigger word
    #
    if [[ "$DEPLOYER_TRIGGER" == "" ]] || [[ $TRAVIS_COMMIT_MESSAGE == *"[$DEPLOYER_TRIGGER]"* ]] \
    || [[ $TRAVIS_COMMIT_MESSAGE == *"[${DEPLOYER_TRIGGER}-${_DEPLOYER_JOB_NAME}]"* ]] \
    || [[ $TRAVIS_COMMIT_MESSAGE == *"[${DEPLOYER_TRIGGER}-${TRAVIS_OS_NAME}]"* ]]; then
        #
        # Whitelist by branch name
        #
        if [[ "$DEPLOYER_BRANCHES" == "" ]] || [[ $DEPLOYER_BRANCHES == *"$TRAVIS_BRANCH"* ]]; then
            # Set this so we only early-terminate builds when we are specifically deploying
            # Trigger string and branch are encompassing conditions; the rest are job-specific
            __DEPLOYER_ACTIVE_GLOBALLY=1;

            #
            # Is the job enabled for deployment?
            #
            if [[ "$DEPLOYER_JOB_ALL" == "1" ]] || [[ "$_DEPLOYER_JOB_ENABLED" == "1" ]]; then
                #
                # Whitelist by OS names
                #
                if [[ "$DEPLOYER_OSNAMES" == "" ]] || [[ $DEPLOYER_OSNAMES == *"$TRAVIS_OS_NAME"* ]]; then
                    # Base Deployer is eligible for becoming active

                    # Are we building for Linux?
                    if [[ "$_DEPLOYER_PACKAGE" == "1" ]] || [[ "$_DEPLOYER_SOURCEPACKAGE" == "1" ]]; then
                        if [[ "$PACKAGE_MAIN_NOBUILD" != "1" ]] || [[ "$PACKAGE_ASSET_BUILD" == "1" ]]; then
                            if [[ "$TRAVIS_OS_NAME" == "linux" ]]; then
                                __DEPLOYER_DEBIAN_ACTIVE=1;
                            fi;
                        fi;
                    fi;

                    # Now check for deployment targets
                    if [[ "$_DEPLOYER_FTP_TARGET" == "1" ]] && [[ "$DEPLOYER_FTP_HOSTNAME" != "" ]]; then
                        if [[ "$TRAVIS_OS_HOST" == "linux" ]] && [[ "$DEPLOYER_FTP_PROTOCOL" == "ftp"]]; then
                            echo "Non-secure FTP will not work on Linux Travis-CI jobs!";
                            echo "Try SFTP or another target. Details:";
                            echo "https://blog.travis-ci.com/2018-07-23-the-tale-of-ftp-at-travis-ci";
                        else
                            if [[ "$__DEPLOYER_DEBIAN_ACTIVE" == "1" ]] || [[ "$_DEPLOYER_PACKAGE" == "1" ]] || [[ "$_DEPLOYER_BINARY" == "1" ]]; then
                                echo "Deployer FTP target is enabled";
                                __DEPLOYER_FTP_ACTIVE=1;
                            else
                                echo "Deployer FTP target cannot be enabled: You must specify _DEPLOYER_PACKAGE=1,";
                                echo "and/or _DEPLOYER_BINARY=1 in your job's environment variables.";
                            fi;
                        fi;
                    fi;

                    if [[ "$_DEPLOYER_DPUT_TARGET" == "1" ]] && [[ "$__DEPLOYER_DEBIAN_ACTIVE" == "1" ]]; then
                        echo "Deployer DPUT target is enabled";
                        __DEPLOYER_DPUT_ACTIVE=1;
                    fi;

                    # If any deployment targets are active, then so is the Deployer at large
                    if [[ "$__DEPLOYER_FTP_ACTIVE" == "1" ]] || [[ "$__DEPLOYER_DPUT_ACTIVE" == "1" ]]; then
                        __DEPLOYER_ACTIVE=1;
                    fi;
                fi;
            fi;
        fi;
    else
        if [[ "$DEPLOYER_TRIGGER" != "" ]]; then
            echo "Testing for global trigger [$DEPLOYER_TRIGGER, commit message: $TRAVIS_COMMIT_MESSAGE";
        fi;
        if [[ "$DEPLOYER_TRIGGER" != "" ]] && [[ $TRAVIS_COMMIT_MESSAGE == *"[$DEPLOYER_TRIGGER"* ]]; then
            if [[ "$DEPLOYER_BRANCHES" == "" ]] || [[ $DEPLOYER_BRANCHES == *"$TRAVIS_BRANCH"* ]]; then
                # Assume that some job received the trigger, so mark this for early termination
                __DEPLOYER_ACTIVE_GLOBALLY=1;
            fi;
        fi;
    fi;
fi;

if [[ "$__DEPLOYER_ACTIVE_GLOBALLY" == "1" ]] && [[ "$__DEPLOYER_ACTIVE" != "1" ]]; then
    echo "Deployer is active in another job";
    if [[ "$DEPLOYER_JOB_TERMINATE_DISABLED" == "1" ]]; then
        echo "Terminating this job due to non-deployment";
    fi;
fi;

if [[ "$__DEPLOYER_ACTIVE_GLOBALLY" != "1" ]] && [[ "$__DEPLOYER_ACTIVE" != "1" ]]; then
    echo "Deployer is not active";
fi;
