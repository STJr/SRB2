#!/bin/bash

# Travis-CI Deployer
# Debian package templating

# Initialize package parameter defaults
if [[ "$__DEBIAN_PARAMETERS_INITIALIZED" != "1" ]]; then
    . ./deployer.sh;
fi;

__PACKAGE_DATETIME="$(date '+%a, %d %b %Y %H:%M:%S %z')"
__PACKAGE_DATETIME_DIGIT="$(date '+%Y%m%d%H%M%S')"
