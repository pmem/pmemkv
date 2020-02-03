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
# build-image.sh <OS-VER> - prepares a Docker image with <OS>-based
#                           environment for testing pmemkv, according
#                           to the Dockerfile.<OS-VER> file located
#                           in the same directory.
#
# The script can be run locally.
#

set -e

function usage {
	echo "Usage:"
	echo "    build-image.sh <DOCKERHUB_REPO> <OS-VER>"
	echo "where <OS-VER>, for example, can be 'fedora-30', provided " \
		"a Dockerfile named 'Dockerfile.fedora-30' exists in the " \
		"current directory."
}

# Check if the first and second argument is nonempty
if [[ -z "$1" || -z "$2" ]]; then
	usage
	exit 1
fi

# Check if the file Dockerfile.OS-VER exists
if [[ ! -f "Dockerfile.$2" ]]; then
	echo "ERROR: wrong argument."
	usage
	exit 1
fi

# Build a Docker image tagged with ${DOCKERHUB_REPO}:1.2-OS-VER
docker build -t $1:1.2-$2 \
	--build-arg http_proxy=$http_proxy \
	--build-arg https_proxy=$https_proxy \
	-f Dockerfile.$2 .
