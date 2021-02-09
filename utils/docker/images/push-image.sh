#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2016-2021, Intel Corporation

#
# push-image.sh - pushes the Docker image tagged as described in
#		./build-image.sh, to the ${CONTAINER_REG}.
#
# The script utilizes ${CONTAINER_REG_USER} and ${CONTAINER_REG_PASS} variables to
# log in to the ${CONTAINER_REG}. The variables can be set in the CI's configuration
# for automated builds.
#

set -e
IMG_VER=${IMG_VER:-devel}
TAG="${OS}-${OS_VER}-${IMG_VER}"

if [[ -z "${OS}" ]]; then
	echo "OS environment variable is not set"
	exit 1
fi

if [[ -z "${OS_VER}" ]]; then
	echo "OS_VER environment variable is not set"
	exit 1
fi

if [[ -z "${CONTAINER_REG}" ]]; then
	echo "CONTAINER_REG environment variable is not set"
	exit 1
fi

if [[ -z "${CONTAINER_REG_USER}" || -z "${CONTAINER_REG_PASS}" ]]; then
	echo "ERROR: variables CONTAINER_REG_USER=\"${CONTAINER_REG_USER}\" and CONTAINER_REG_PASS=\"${CONTAINER_REG_PASS}\"" \
		"have to be set properly to allow login to the Container Registry."
	exit 1
fi

# Check if the image tagged with ${CONTAINER_REG}:${TAG} exists locally
if [[ ! $(docker images -a | awk -v pattern="^${CONTAINER_REG}:${TAG}\$" \
	'$1":"$2 ~ pattern') ]]
then
	echo "ERROR: Docker image tagged ${CONTAINER_REG}:${TAG} does not exists locally."
	exit 1
fi

echo "Log in to the container registry: ${CONTAINER_REG}"
echo "${CONTAINER_REG_PASS}" | docker login ghcr.io -u="${CONTAINER_REG_USER}" --password-stdin

echo "Push the image to the repository"
docker push ${CONTAINER_REG}:${TAG}
