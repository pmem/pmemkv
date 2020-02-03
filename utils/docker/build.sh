#!/usr/bin/env bash
#
# Copyright 2017-2020, Intel Corporation
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
# build.sh - runs a Docker container from a Docker image with environment
#            prepared for running pmemkv build and tests.
#
#
# Notes:
# - run this script from its location or set the variable 'HOST_WORKDIR' to
#   where the root of this project is on the host machine,
# - set variables 'OS' and 'OS_VER' properly to a system you want to build this
#   repo on (for proper values take a look on the list of Dockerfiles at the
#   utils/docker/images directory), eg. OS=ubuntu, OS_VER=16.04.
#

set -e

if [[ -z "$OS" || -z "$OS_VER" ]]; then
	echo "ERROR: The variables OS and OS_VER have to be set " \
		"(eg. OS=fedora, OS_VER=30)."
	exit 1
fi

if [[ -z "$HOST_WORKDIR" ]]; then
	HOST_WORKDIR=$(readlink -f ../..)
fi

if [[ "$TRAVIS_EVENT_TYPE" == "cron" || "$TRAVIS_BRANCH" == "coverity_scan" ]]; then
	if [[ "$TYPE" != "coverity" ]]; then
		echo "Skipping non-Coverity job for cron/Coverity build"
		exit 0
	fi
else
	if [[ "$TYPE" == "coverity" ]]; then
		echo "Skipping Coverity job for non cron/Coverity build"
		exit 0
	fi
fi

imageName=${DOCKERHUB_REPO}:1.2-${OS}-${OS_VER}
containerName=pmemkv-${OS}-${OS_VER}

if [[ "$command" == "" ]]; then
	case $TYPE in
		normal)
			command="./run-build.sh";
			;;
		compatibility)
			command="./run-compatibility.sh";
			;;
		building)
			command="./run-test-building.sh";
			;;
		coverity)
			command="./run-coverity.sh";
			;;
		bindings)
			command="./run-bindings.sh";
			;;
	esac
fi

if [ "$COVERAGE" == "1" ]; then
	docker_opts="${docker_opts} `bash <(curl -s https://codecov.io/env)`";
	ci_env=`bash <(curl -s https://codecov.io/env)`
fi

if [ -n "$DNS_SERVER" ]; then DNS_SETTING=" --dns=$DNS_SERVER "; fi


# Only run doc update on pmem/pmemkv master branch
if [[ "$TRAVIS_BRANCH" != "master" || "$TRAVIS_PULL_REQUEST" != "false" || "$TRAVIS_REPO_SLUG" != "${GITHUB_REPO}" ]]; then
	AUTO_DOC_UPDATE=0
fi

WORKDIR=/pmemkv
SCRIPTSDIR=$WORKDIR/utils/docker

# check if we are running on a CI (Travis or GitHub Actions)
[ -n "$GITHUB_ACTIONS" -o -n "$TRAVIS" ] && CI_RUN="YES" || CI_RUN="NO"

# do not allocate a pseudo-TTY if we are running on GitHub Actions
[ ! $GITHUB_ACTIONS ] && TTY='-t' || TTY=''

echo Building ${OS}-${OS_VER}

# Run a container with
#  - environment variables set (--env)
#  - host directory containing source mounted (-v)
#  - working directory set (-w)
docker run --privileged=true --name=$containerName -i $TTY \
	$DNS_SETTING \
	${docker_opts} \
	$ci_env \
	--env http_proxy=$http_proxy \
	--env https_proxy=$https_proxy \
	--env AUTO_DOC_UPDATE=$AUTO_DOC_UPDATE \
	--env GITHUB_TOKEN=$GITHUB_TOKEN \
	--env WORKDIR=$WORKDIR \
	--env SCRIPTSDIR=$SCRIPTSDIR \
	--env COVERAGE=$COVERAGE \
	--env TRAVIS_REPO_SLUG=$TRAVIS_REPO_SLUG \
	--env TRAVIS_BRANCH=$TRAVIS_BRANCH \
	--env TRAVIS_EVENT_TYPE=$TRAVIS_EVENT_TYPE \
	--env COVERITY_SCAN_TOKEN=$COVERITY_SCAN_TOKEN \
	--env COVERITY_SCAN_NOTIFICATION_EMAIL=$COVERITY_SCAN_NOTIFICATION_EMAIL \
	--env TEST_BUILD=$TEST_BUILD \
	--env DEFAULT_TEST_DIR=/dev/shm \
	--env TEST_PACKAGES=${TEST_PACKAGES:-ON} \
	--env BUILD_JSON_CONFIG=${BUILD_JSON_CONFIG:-ON} \
	--env CHECK_CPP_STYLE=${CHECK_CPP_STYLE:-ON} \
	--env CI_RUN=$CI_RUN \
	--shm-size=4G \
	-v $HOST_WORKDIR:$WORKDIR \
	-v /etc/localtime:/etc/localtime \
	-w $SCRIPTSDIR \
	$imageName $command
