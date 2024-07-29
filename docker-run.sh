#!/bin/bash

DOCKER_IMAGE_NAME=sim_bootsec

DOCKER_CMD=docker
if [ -z $(which $DOCKER_CMD) ]; then
    DOCKER_CMD=podman
fi

if [ -z "$($DOCKER_CMD image ls -a | grep localhost/$DOCKER_IMAGE_NAME )" ]; then
    $DOCKER_CMD build -t sim_bootsec:latest . 
fi
mkdir -p ./ndnSIM 
if ! [ -e "./ndnSIM/ns-3/scratch/sim_bootsec" ]; then
    # create ns-3 dir, if it doesnt exist
    $DOCKER_CMD run --rm -v ./ndnSIM:/ndnSIM localhost/$DOCKER_IMAGE_NAME
    # copy files to scratch
    chmod 755 -R .
    GIT_CURRENT_BRANCH=$(git rev-parse --abbrev-ref HEAD)
    git clone -b "$GIT_CURRENT_BRANCH" git@github.com:andre-romano/ndn-secure-bootstrap.git "./ndnSIM/ns-3/scratch/sim_bootsec/"    
fi
$DOCKER_CMD run --name $DOCKER_IMAGE_NAME -it -v ./ndnSIM:/ndnSIM localhost/$DOCKER_IMAGE_NAME ||
$DOCKER_CMD start -at $DOCKER_IMAGE_NAME
