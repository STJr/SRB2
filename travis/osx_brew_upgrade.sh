#!/bin/sh

build_from_source=""
package=$1
if [[ "$package" == "--build-from-source" ]]; then
    build_from_source="--build-from-source";
    package=$2;
    args=${a[@]:2};
else
    args=${a[@]:1};
fi
pkg_installed=false
pkg_updated=false
verbose=true

# TODO: ensure valid input

list_output=`brew list | grep $package`
outdated_output=`brew outdated | grep $package`

# now enable error checking
set -e

if [[ ! -z "$list_output" ]]; then
    pkg_installed=true
    $verbose && echo "package $package is installed"
    if [[ -z "$outdated_output" ]]; then
        pkg_updated=true
        $verbose && echo "package $package is up-to-date."
    else
        $verbose && echo "package $package is out-of-date. updating..."
        brew upgrade $build_from_source $package $args
    fi
else
    $verbose && echo "package $package is not installed. installing..."
    brew install $build_from_source $package $args
fi
