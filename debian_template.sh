#!/bin/bash

# Deployer for Travis-CI
# Debian package templating
#
# Call this script BEFORE running debuild!
# source ./debian_template.sh [clean] [main/asset]
#
# Before running this script,
# you should also set PACKAGE_NAME_EMAIL="John Doe <jdoe@example.com>" to match
# the identity of the key you will use to sign the package.
#

# Get script's actual path
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null && pwd )"

# Recursive function for directory crawling
# $1 = Directory root to crawl
# $2 = Code to eval on file
# $3 = Code to eval on directory
# Exposes $dirtails, $dirlevel, and $dirtailname
dirlevel=0 # initialize
dirtails=()

# Utility function to make dira/dirb/dirc string
makedirtailname () {
	dirtailname=""
	for tail in $dirtails; do
		if [[ "$dirtailname" == "" ]]; then
			dirtailname="/$tail";
		else
			dirtailname="$dirtailname/$tail";
		fi;
	done;
}

evaldirectory () {
	if [ -d "$1" ]; then
		# Set contextual variables
		# dirtails is an array of directory basenames after the crawl root
		if (( $dirlevel > 0 )); then
			dirtails+=( "$(basename $1)" );
		else
			dirtails=();
		fi;
		dirlevel=$((dirlevel+1));

		# Generate directory path after the crawl root
		makedirtailname;

		# Eval our directory with the latest contextual info
		# Don't eval on root
		if (( $dirlevel > 1 )) && [[ "$3" != "" ]]; then
			eval "$3";
		fi;

		# Iterate entries
		for name in $1/*; do
			if [ -d "$name" ]; then
				# Name is a directory, but don't eval yet
				# Recurse so our vars are updated
				evaldirectory "$name" "$2" "$3";

				# Decrement our directory level and remove a dirtail
				unset 'dirtails[ ${#dirtails[@]}-1 ]';
				dirlevel=$((dirlevel-1));
				makedirtailname;
			else
				# Name is a file
				if [ -f "$name" ] && [[ "$2" != "" ]]; then
					eval "$2";
				fi;
			fi;
		done;

		# Reset our variables; we're done iterating
		if (( $dirlevel == 1 )); then
			dirlevel=0;
		fi;
	fi;
}

#
# Initialize package parameter defaults
#
if [[ "$__DEBIAN_PARAMETERS_INITIALIZED" != "1" ]]; then
	. ${DIR}/deployer/travis/deployer_defaults.sh;
fi;

# Clean up after ourselves; we only expect to run this script once
# during buildboting
__DEBIAN_PARAMETERS_INITIALIZED=0

# for envsubst
export __PACKAGE_DATETIME="$(date '+%a, %d %b %Y %H:%M:%S %z')"
export __PACKAGE_DATETIME_DIGIT="$(date -u '+%Y%m%d%H%M%S')"

if [[ "$PACKAGE_REVISION" == "" ]]; then
	PACKAGE_REVISION="-$__PACKAGE_DATETIME_DIGIT";
	__PACKAGE_REVISION_BY_DATE=1;
	export PACKAGE_REVISION=${PACKAGE_REVISION}; # for envsubst
fi;

#
# Clean the old debian/ directories
#
if [[ "$1" == "clean" ]]; then
	toclean=$2;
else
	toclean=$1;
fi;

if [[ "$toclean" == "" ]] || [[ "$toclean" == "main" ]]; then
	echo "Cleaning main package scripts";
	if [[ ! -f ${DIR}/debian ]]; then
		rm -rf ${DIR}/debian;
	fi;
fi;
if [[ "$toclean" == "" ]] || [[ "$toclean" == "asset" ]]; then
	echo "Cleaning asset package scripts";
	if [[ ! -f ${DIR}/assets/debian ]]; then
		rm -rf ${DIR}/assets/debian;
	fi;
fi;

#
# Make new templates
#
if [[ "$1" != "clean" ]]; then
	totemplate=$1;

	# HACK: ${shlibs:Depends} in the templates make the templating fail
	# So just define replacemment variables
	export SHLIBS_DEPENDS=${SHLIBS_DEPENDS};
	export MISC_DEPENDS=${MISC_DEPENDS};
	export DEBFILEVAR='$$file'; # used in assets/debian/rules

	# Package parameters are exported for envsubst in deployer_defaults.sh

	if [[ "$totemplate" == "" ]] || [[ "$totemplate" == "main" ]]; then
		echo "Generating main package scripts";
		fromroot=${DIR}/debian-template;
		toroot=${DIR}/debian;
		mkdir ${toroot};

		evaldirectory ${fromroot} \
			"cat \$name | envsubst > ${toroot}\${dirtailname}/\$( basename \$name )" \
			"mkdir \"${toroot}\${dirtailname}\"";
	fi;

	if [[ "$totemplate" == "" ]] || [[ "$totemplate" == "asset" ]]; then
		echo "Generating asset package scripts";
		fromroot=${DIR}/assets/debian-template;
		toroot=${DIR}/assets/debian;
		mkdir ${toroot};

		# Root dir to crawl; file eval; directory eval
		evaldirectory ${fromroot} \
			"cat \$name | envsubst > ${toroot}\${dirtailname}/\$( basename \$name )" \
			"mkdir \"${toroot}\${dirtailname}\"";
	fi;
fi;

if [[ "$__DPL_ACTIVE" != "1" ]] && [[ "$__PACKAGE_REVISION_BY_DATE" == "1" ]]; then
	unset PACKAGE_REVISION; # so we can reset the date on subsequent runs
fi;
