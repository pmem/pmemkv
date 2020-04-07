#!/usr/bin/env bash
#
# Copyright 2016-2020, Intel Corporation
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in
#       the documentation and/or other materials provided with the
#       distribution.
#
#     * Neither the name of the copyright holder nor the names of its
#       contributors may be used to endorse or promote products derived
#       from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#
# push-image.sh - pushes the Docker image tagged with OS-VER to the Docker Hub.
#
# The script utilizes $DOCKERHUB_USER and $DOCKERHUB_PASSWORD variables to
# log in to the Docker Hub. The variables can be set in the CI's configuration
# for automated builds.
#

set -e

if [[ -z "$OS" ]]; then
	echo "OS environment variable is not set"
	exit 1
fi

if [[ -z "$OS_VER" ]]; then
	echo "OS_VER environment variable is not set"
	exit 1
fi

if [[ -z "${DOCKERHUB_REPO}" ]]; then
	echo "DOCKERHUB_REPO environment variable is not set"
	exit 1
fi

TAG="1.1-${OS}-${OS_VER}"

# Check if the image tagged with ${DOCKERHUB_REPO}:${TAG} exists locally
if [[ ! $(docker images -a | awk -v pattern="^${DOCKERHUB_REPO}:${TAG}\$" \
	'$1":"$2 ~ pattern') ]]
then
	echo "ERROR: Docker image tagged ${DOCKERHUB_REPO}:${TAG} does not exists locally."
	exit 1
fi

# Log in to the Docker Hub
docker login -u="$DOCKERHUB_USER" -p="$DOCKERHUB_PASSWORD"

# Push the image to the repository
docker push ${DOCKERHUB_REPO}:${TAG}
