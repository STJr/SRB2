#!/bin/bash

# Travis-CI Deployer
# Debian package templating

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null && pwd )"

# Initialize package parameter defaults
if [[ "$__DEBIAN_PARAMETERS_INITIALIZED" != "1" ]]; then
    . ${DIR}/travis/deployer.sh;
fi;

__PACKAGE_DATETIME="$(date '+%a, %d %b %Y %H:%M:%S %z')"
__PACKAGE_DATETIME_DIGIT="$(date '+%Y%m%d%H%M%S')"

# TODO: Script arguments
# Build main, or assets
