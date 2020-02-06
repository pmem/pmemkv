#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2016-2020, Intel Corporation

#
# push-image.sh <OS-VER> - pushes the Docker image tagged with OS-VER
#                          to the Docker Hub.
#
# The script utilizes $DOCKERHUB_USER and $DOCKERHUB_PASSWORD variables to log in to
# Docker Hub. The variables can be set in the Travis project's configuration
# for automated builds.
#

set -e

function usage {
	echo "Usage:"
	echo "    push-image.sh <OS-VER>"
	echo "where <OS-VER>, for example, can be 'fedora-30', provided " \
		"a Docker image tagged with ${DOCKERHUB_REPO}:1.2-fedora-30 exists " \
		"locally."
}

# Check if the first argument is nonempty
if [[ -z "$1" ]]; then
	usage
	exit 1
fi

# Check if the image tagged with ${DOCKERHUB_REPO}:1.2-OS-VER exists locally
if [[ ! $(docker images -a | awk -v pattern="^${DOCKERHUB_REPO}:1.2-$1\$" \
	'$1":"$2 ~ pattern') ]]
then
	echo "ERROR: wrong argument."
	usage
	exit 1
fi

# Log in to the Docker Hub
docker login -u="$DOCKERHUB_USER" -p="$DOCKERHUB_PASSWORD"

# Push the image to the repository
docker push ${DOCKERHUB_REPO}:1.2-$1
