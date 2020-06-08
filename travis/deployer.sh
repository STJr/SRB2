#!/bin/bash

# Deployer for Travis-CI
# Pre-Deployment Script

# Determine whether we are deploying this job.
if [[ ("$DPL_GITHUB_TOKEN" != "" &&
      "$TRAVIS_PULL_REQUEST" == "false" &&
      "$DPL_FORCE_OFF" != "1" &&
      !("$DPL_FORCE_OFF_GITHUB" == "1" &&
        "$DPL_FORCE_OFF_FTP" == "1") &&
      (
      "$DPL_FORCE_ON" = "1" ||
      "$DPL_FORCE_ON_GITHUB" = "1" ||
      "$DPL_FORCE_ON_FTP" = "1" ||
      "$TRAVIS_TAG" != "" ||
      "$TRAVIS_BRANCH" =~ ^.*$DPL_BRANCH_TRIGGER.*$ ||
      "$TRAVIS_COMMIT_MESSAGE" =~ ^.*\[$DPL_COMMIT_TRIGGER\].*$
      )) ]]; then
  __DPL_ENABLED=1;
fi;

# On OSX, use sudo to make packages
if [[ "$TRAVIS_OS_NAME" == "osx" ]]; then
  __MAKE="sudo make";
  __RM="sudo rm";
  __MV="sudo mv";
else
  __RM="rm";
  __MV="mv";
fi;

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
  __CPACK_GENERATOR="7Z"
fi;

mkdir deploy;

# Use CMAKE to build no-asset and asset packages.
# NOTE: When specifying an archive generator (ZIP, TGZ, 7Z), the filename
# should be specified by CPACK_ARCHIVE_FILE_NAME, but currently the effective
# variable is CPACK_PACKAGE_FILE_NAME. Per
# https://gitlab.kitware.com/cmake/cmake/-/issues/20419

# Build no-asset package
if [[ "$__DPL_ENABLED" == "1" ]] && [[ "$DPL_UPLOAD_BINARY" == "1" ]]; then
  cmake .. -DSRB2_ASSET_INSTALL=OFF -DSRB2_DEBUG_INSTALL=ON -DSRB2_CPACK_GENERATOR="${__CPACK_GENERATOR}" -DCPACK_PACKAGE_FILE_NAME="${DPL_PACKAGE_NAME}-no-assets";
  $__MAKE -k package;
  if [ -d "package/_CPack_Packages" ]; then
    $__RM -r package/_CPack_Packages;
  fi;
  $__MV package/* deploy/;
fi;

# Build package including binary and assets
if [[ "$__DPL_ENABLED" == "1" ]] && [[ "$DPL_UPLOAD_ASSETS" == "1" ]]; then
  cmake .. -DSRB2_ASSET_INSTALL=ON -DSRB2_DEBUG_INSTALL=ON -DSRB2_CPACK_GENERATOR="${__CPACK_GENERATOR}" -DCPACK_PACKAGE_FILE_NAME="${DPL_PACKAGE_NAME}";
  $__MAKE -k package;
  if [ -d "package/_CPack_Packages" ]; then
    $__RM -r package/_CPack_Packages;
  fi;
  $__MV package/* deploy/;
fi;

# Build installer
# We expect the filetype to be different than the asset package, above.
# For now, only build for OSX.
if [[ "$__DPL_ENABLED" == "1" ]] && [[ "$DPL_UPLOAD_INSTALLER" == "1" ]]; then
  if [[ "$TRAVIS_OS_NAME" == "osx" ]]; then
    cmake .. -DSRB2_ASSET_INSTALL=ON -DSRB2_DEBUG_INSTALL=OFF -DSRB2_CPACK_GENERATOR=DragNDrop -DCPACK_PACKAGE_FILE_NAME="${DPL_PACKAGE_NAME}";
    $__MAKE -k package;
    if [ -d "package/_CPack_Packages" ]; then
      $__RM -r package/_CPack_Packages;
    fi;
    $__MV package/* deploy/;
  else
    echo "Building an installer is not supported on $TRAVIS_OS_NAME.";
  fi;
fi;
