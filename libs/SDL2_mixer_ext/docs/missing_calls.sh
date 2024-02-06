#!/bin/bash

while read CMD; do
    if ! grep -Fxq "$CMD" *.texi*
    then
        echo "Documentation for call $CMD is missing"
    fi
done < "list.txt"

while read CMD; do
    if ! grep -Fxq "$CMD" *.texi*
    then
        echo "Documentation for enum $CMD is missing"
    fi
done < "list-enums.txt"

while read CMD; do
    if grep -Fxq "$CMD" *.texi*
    then
        echo "Deprecated call $CMD is still be documented"
    fi
done < "deprecated.txt"
