#!/bin/sh

WORKSPACE="subprojects/west"
if [ ! -d ${WORKSPACE}/../.west ]; then
    echo "Initializing west in ${WORKSPACE}"
    mkdir -p ${WORKSPACE}
    cd ${WORKSPACE}
    west init --local --manifest-file ../../west.yml
    cd ../..
fi
