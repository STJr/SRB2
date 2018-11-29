#!/bin/bash

# Travis-CI Deployer
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
# DEPLOYER_OPTIONALFILES = 1                 (to download and package music.dta; leave blank to disable)
#
# To enable Deployer on specific jobs, set _DEPLOYER_JOB_ENABLED=1 for that job. Example:
# - matrix:
#   - os: osx
#     env:
#     - _DEPLOYER_JOB_ENABLED=1
#
# DO NOT set __DEPLOYER_ACTIVE, because that would bypass these validity checks.

if [[ "$DEPLOYER_ENABLED" == "1" ]] && [[ "$DEPLOYER_FTP_HOSTNAME" != "" ]]; then \
	if [[ "$DEPLOYER_JOB_ALL" == "1" ]] || [[ "$_DEPLOYER_JOB_ENABLED" == "1" ]]; then \
		if [[ "$DEPLOYER_OSNAMES" == "" ]] || [[ $DEPLOYER_OSNAMES == *"$TRAVIS_OS_NAME"* ]]; then \
			if [[ "$TRAVIS_PULL_REQUEST" == "false" ]]; then \
				if [[ "$DEPLOYER_BRANCHES" == "" ]] || [[ $DEPLOYER_BRANCHES == *"$TRAVIS_BRANCH"* ]]; then \
					__DEPLOYER_ACTIVE=1;
				fi; \
			fi; \
		fi; \
	fi; \
fi;
