#!/usr/bin/env bash

LANGUAGES="$(ls languages/)"
PYTHON=$(command -v python3)

#
# $1 is the language to build
#
function BuildLanguage()
{
    (cd languages/$1 && ${PYTHON} ../../OpenRTXVoicePromptsBuilder.py $PROMPTS_OPTIONS -c config.csv) || return 1 && return 0
}

#
#
#
if [ "$#" -ne 0 ]; then # One or more language has been specified on the command line
    for l in "$*"; do
	echo "Build $l..."
	
	BuildLanguage $l
	ret=$?
	
	if [ $ret -eq 1 ]; then
	    echo "An error occured while building language \"$l\"."
	    exit 1
	fi
	
	echo "Done"
    done
else # No command line argument, build the whole language list
    echo "Build all languages..."
    
    for l in $LANGUAGES; do
	BuildLanguage $l
	ret=$?

	if [ $ret -eq 1 ]; then
	    echo "An error occured while building language \"$l\"."
	    exit 1
	fi
    done

    echo "Done."
fi

exit 0
