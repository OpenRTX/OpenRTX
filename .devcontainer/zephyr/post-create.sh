#!/bin/sh

if [ ! -d ../.west ]; then
    west init --local --manifest-file west.yml
fi
