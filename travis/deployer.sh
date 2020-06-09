#!/bin/bash

# Deployer for Travis-CI
# Pre-Deployment Script

# Determine whether we are deploying this job.
# This does not include target-specific conditions.
#
# The concept is: 
# DPL_DISABLED conditions take highest priority.
# DPL_TRIGGERED indicates whether a deployment is triggered by tag, branch, or message
# DPL_FORCE_ENABLED can be true if the user specifies DPL_FORCE_ON despite a trigger not occurring

########################################
# DPL_DISABLED Conditions
########################################
if [[ ("$DPL_GITHUB_TOKEN" == "" || "$DPL_FORCE_OFF_GITHUB" == "1" ) ]]; then
  __DPL_DISABLED_GITHUB=1;
  DPL_UPLOAD_BINARY_GITHUB=0;
  DPL_UPLOAD_ASSETS_GITHUB=0;
  DPL_UPLOAD_INSTALLER_GITHUB=0;
  DPL_FORCE_ON_GITHUB=0;
fi;

if [[ ("$DPL_FTP_USER" == "" || 
      "$DPL_FTP_PASS" == "" || 
      "$DPL_FTP_HOSTNAME" == "" ||
      "$DPL_FORCE_OFF_FTP" == "1"
      ) ]]; then
  __DPL_DISABLED_FTP=1;
  DPL_UPLOAD_BINARY_FTP=0;
  DPL_UPLOAD_ASSETS_FTP=0;
  DPL_UPLOAD_INSTALLER_FTP=0;
  DPL_FORCE_ON_FTP=0;
fi;

if [[ ("$DPL_FORCE_OFF" == "1" || "$TRAVIS_PULL_REQUEST" != "false" ||
        ("$__DPL_DISABLED_FTP" == "1" && "$__DPL_DISABLED_GITHUB" == "1")
      ) ]]; then
  __DPL_DISABLED=1;
fi;

########################################
# DPL_TRIGGERED Conditions
########################################
if [[ ("$DPL_DISABLED" != "1" &&
        ("$DPL_FORCE_ON" == "1" ||
        ("$TRAVIS_TAG" != "" && "$DPL_TAG_ENABLED" == "1") ||
        "$TRAVIS_BRANCH" =~ ^.*$DPL_BRANCH_TRIGGER.*$ ||
        "$TRAVIS_COMMIT_MESSAGE" =~ ^.*\[$DPL_COMMIT_TRIGGER\].*$
        )
      ) ]]; then
  __DPL_TRIGGERED=1;
fi;

########################################
# DPL_FORCE_ENABLED Conditions
########################################
if [[ ("$DPL_DISABLED" != "1" &&
        ("$DPL_FORCE_ON" == "1" || "$DPL_FORCE_ON_GITHUB" == "1" || "$DPL_FORCE_ON_FTP" == "1")
      ) ]]; then
  __DPL_FORCE_ENABLED=1;
fi;

if [[ ("$__DPL_TRIGGERED" == "1" || "$__DPL_FORCE_ENABLED" == "1") ]]; then
  __DPL_TRIGGERED_OR_FORCED=1;
fi;

########################################
# Package Generation Rules
########################################

# This monstrosity accounts for negating rules when, e.g.,
# DPL_UPLOAD_BINARY is true but DPL_UPLOAD_BINARY_GITHUB and/or DPL_UPLOAD_BINARY_FTP are explicitly false.
#
# DPL_UPLOAD_BINARY=1 applies to all targets, but you can negate the target by setting 0.
# DPL_UPLOAD_BINARY=0 can be overridden by setting the target to 1.
# An empty target setting means "do not override".

if [[ ("$__DPL_TRIGGERED_OR_FORCED" == "1" &&
        ("$DPL_UPLOAD_BINARY_GITHUB" == "1" ||
        "$DPL_UPLOAD_BINARY_FTP" == "1" || 
          ("$DPL_UPLOAD_BINARY" == "1" &&
            ! ("$DPL_UPLOAD_BINARY_GITHUB" == "0" && "$DPL_UPLOAD_BINARY_FTP" == "0")
          )
        )
      ) ]]; then
  __DPL_UPLOAD_BINARY=1;
  if [[ ("$DPL_UPLOAD_BINARY_GITHUB" == "1" ||
          ("$DPL_UPLOAD_BINARY" == "1" && "$DPL_UPLOAD_BINARY_GITHUB" != "0")
        ) ]]; then
    __DPL_UPLOAD_BINARY_GITHUB=1;
  fi;
  if [[ ("$DPL_UPLOAD_BINARY_FTP" == "1" ||
          ("$DPL_UPLOAD_BINARY" == "1" && "$DPL_UPLOAD_BINARY_FTP" != "0")
        ) ]]; then
    __DPL_UPLOAD_BINARY_FTP=1;
  fi;
fi;

if [[ ("$__DPL_TRIGGERED_OR_FORCED" == "1" &&
        ("$DPL_UPLOAD_ASSETS_GITHUB" == "1" ||
        "$DPL_UPLOAD_ASSETS_FTP" == "1" || 
          ("$DPL_UPLOAD_ASSETS" == "1" &&
            ! ("$DPL_UPLOAD_ASSETS_GITHUB" == "0" && "$DPL_UPLOAD_ASSETS_FTP" == "0")
          )
        )
      ) ]]; then
  __DPL_UPLOAD_ASSETS=1;
  if [[ ("$DPL_UPLOAD_ASSETS_GITHUB" == "1" ||
          ("$DPL_UPLOAD_ASSETS" == "1" && "$DPL_UPLOAD_ASSETS_GITHUB" != "0")
        ) ]]; then
    __DPL_UPLOAD_ASSETS_GITHUB=1;
  fi;
  if [[ ("$DPL_UPLOAD_ASSETS_FTP" == "1" ||
          ("$DPL_UPLOAD_ASSETS" == "1" && "$DPL_UPLOAD_ASSETS_FTP" != "0")
        ) ]]; then
    __DPL_UPLOAD_ASSETS_FTP=1;
  fi;
fi;

if [[ ("$__DPL_TRIGGERED_OR_FORCED" == "1" &&
        ("$DPL_UPLOAD_INSTALLER_GITHUB" == "1" ||
        "$DPL_UPLOAD_INSTALLER_FTP" == "1" || 
          ("$DPL_UPLOAD_INSTALLER" == "1" &&
            ! ("$DPL_UPLOAD_INSTALLER_GITHUB" == "0" && "$DPL_UPLOAD_INSTALLER_FTP" == "0")
          ) 
        )
      ) ]]; then
  __DPL_UPLOAD_INSTALLER=1;
  if [[ ("$DPL_UPLOAD_INSTALLER_GITHUB" == "1" ||
          ("$DPL_UPLOAD_INSTALLER" == "1" && "$DPL_UPLOAD_INSTALLER_GITHUB" != "0")
        ) ]]; then
    __DPL_UPLOAD_INSTALLER_GITHUB=1;
  fi;
  if [[ ("$DPL_UPLOAD_INSTALLER_FTP" == "1" ||
          ("$DPL_UPLOAD_INSTALLER" == "1" && "$DPL_UPLOAD_INSTALLER_FTP" != "0")
        ) ]]; then
    __DPL_UPLOAD_INSTALLER_FTP=1;
  fi;
fi;

########################################
# Procedure
########################################

# On OSX, use sudo to make packages
if [[ "$TRAVIS_OS_NAME" == "osx" ]]; then
  __MAKE="sudo make";
  __RM="sudo rm";
  __MV="sudo mv";
  __CP="sudo cp";
else
  __RM="rm";
  __MV="mv";
  __CP="cp";
fi;

mkdir deploy-github;
mkdir deploy-ftp;

# Construct our package name
if [[ "$DPL_PACKAGE_NAME" == "" ]]; then
  if [[ "$TRAVIS_TAG" != "" ]]; then
    DPL_PACKAGE_NAME="$TRAVIS_TAG-$TRAVIS_OS_NAME";
  else
    DPL_PACKAGE_NAME="$PROGRAM_FILENAME-$TRAVIS_OS_NAME-$TRAVIS_BRANCH-${TRAVIS_COMMIT:0:8}-${TRAVIS_JOB_ID}";
  fi;
fi;

# As-is tradition, generate 7Z archives for Windows.
if [[ "$TRAVIS_OS_NAME" == "windows" ]]; then
  __CPACK_GENERATOR="7Z";
fi;

# Use CMAKE to build no-asset and asset packages.
# NOTE: When specifying an archive generator (ZIP, TGZ, 7Z), the filename
# should be specified by CPACK_ARCHIVE_FILE_NAME, but currently the effective
# variable is CPACK_PACKAGE_FILE_NAME. Per
# https://gitlab.kitware.com/cmake/cmake/-/issues/20419

# Build no-asset package
if [[ "$__DPL_UPLOAD_BINARY" == "1" ]]; then
  cmake .. -DSRB2_ASSET_INSTALL=OFF -DSRB2_DEBUG_INSTALL=ON -DSRB2_CPACK_GENERATOR="${__CPACK_GENERATOR}" -DCPACK_PACKAGE_FILE_NAME="${DPL_PACKAGE_NAME}-no-assets";
  $__MAKE -k package;
  if [ -d "package/_CPack_Packages" ]; then
    $__RM -r package/_CPack_Packages;
  fi;
  if [[ "$__DPL_UPLOAD_BINARY_GITHUB" == "1" ]]; then
    $__CP package/* deploy-github/;
  fi;
  if [[ "$__DPL_UPLOAD_BINARY_FTP" == "1" ]]; then
    $__CP package/* deploy-ftp/;
  fi;
  $__RM -r package;
fi;

# Build package including binary and assets
if [[ "$__DPL_UPLOAD_ASSETS" == "1" ]]; then
  cmake .. -DSRB2_ASSET_INSTALL=ON -DSRB2_DEBUG_INSTALL=ON -DSRB2_CPACK_GENERATOR="${__CPACK_GENERATOR}" -DCPACK_PACKAGE_FILE_NAME="${DPL_PACKAGE_NAME}";
  $__MAKE -k package;
  if [ -d "package/_CPack_Packages" ]; then
    $__RM -r package/_CPack_Packages;
  fi;
  if [[ "$__DPL_UPLOAD_ASSETS_GITHUB" == "1" ]]; then
    $__CP package/* deploy-github/;
  fi;
  if [[ "$__DPL_UPLOAD_ASSETS_FTP" == "1" ]]; then
    $__CP package/* deploy-ftp/;
  fi;
  $__RM -r package;
fi;

# Build installer
# We expect the filetype to be different than the asset package, above.
# Windows TODO: Support NSIS. For now, we'll just do 7-zip SFX.
if [[ "$__DPL_UPLOAD_INSTALLER" == "1" ]]; then
  if [[ "$TRAVIS_OS_NAME" == "osx" ]]; then
    cmake .. -DSRB2_ASSET_INSTALL=ON -DSRB2_DEBUG_INSTALL=OFF -DSRB2_CPACK_GENERATOR=DragNDrop -DCPACK_PACKAGE_FILE_NAME="${DPL_PACKAGE_NAME}";
    $__MAKE -k package;
    if [ -d "package/_CPack_Packages" ]; then
      $__RM -r package/_CPack_Packages;
    fi;
  elif [[ "$TRAVIS_OS_NAME" == "windows" ]]; then
    cmake .. -DSRB2_ASSET_INSTALL=ON -DSRB2_DEBUG_INSTALL=OFF -DCMAKE_INSTALL_PREFIX="${PWD}/staging";
    $__MAKE -k install;
    mkdir package;
    cd staging;
    7z a -sfx7z.sfx "../package/${DPL_PACKAGE_NAME}.exe" ./*;
    cd ..;
  else
    echo "Building an installer is not supported on $TRAVIS_OS_NAME.";
  fi;
  if [ -d "package" ]; then
    if [[ "$__DPL_UPLOAD_INSTALLER_GITHUB" == "1" ]]; then
      $__CP package/* deploy-github/;
    fi;
    if [[ "$__DPL_UPLOAD_INSTALLER_FTP" == "1" ]]; then
      $__CP package/* deploy-ftp/;
    fi;
    $__RM -r package;
  fi;
fi;
