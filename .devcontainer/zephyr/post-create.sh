#!/bin/bash

source ${HOME}/venv/bin/activate

if [ ! -d ../.west ]; then
    west init --local --manifest-file west.yml
fi
