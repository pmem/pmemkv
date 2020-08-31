#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2017-2020, Intel Corporation

#
# build.sh - runs a Docker container from a Docker image with environment
#            prepared for running pmemkv build and tests.
#
#
# Notes:
# - run this script from its location or set the variable 'HOST_WORKDIR' to
#   where the root of this project is on the host machine,
# - set variables 'OS' and 'OS_VER' properly to a system you want to build this
#   repo on (for proper values take a look at the list of Dockerfiles at the
#   utils/docker/images directory), eg. OS=ubuntu, OS_VER=19.10.
#

set -e

source $(dirname $0)/set-ci-vars.sh
source $(dirname $0)/set-vars.sh
source $(dirname $0)/valid-branches.sh

doc_varialbes_error="To build documentation and upload it as github pull request \
variables 'DOC_UPDATE_BOT_NAME' and 'DOC_UPDATE_GITHUB_TOKEN' have to be provided. \
for more details please read CONTRIBUTION.md"

if [[ "$CI_EVENT_TYPE" != "cron" && "$CI_BRANCH" != "coverity_scan" \
	&& "$TYPE" == "coverity" ]]; then
	echo "INFO: Skip Coverity scan job if build is triggered neither by " \
		"'cron' nor by a push to 'coverity_scan' branch"
	exit 0
fi

if [[ ( "$CI_EVENT_TYPE" == "cron" || "$CI_BRANCH" == "coverity_scan" )\
	&& "$TYPE" != "coverity" ]]; then
	echo "INFO: Skip regular jobs if build is triggered either by 'cron'" \
		" or by a push to 'coverity_scan' branch"
	exit 0
fi

if [[ -z "$OS" || -z "$OS_VER" ]]; then
	echo "ERROR: The variables OS and OS_VER have to be set " \
		"(eg. OS=fedora, OS_VER=31)."
	exit 1
fi

if [[ -z "$HOST_WORKDIR" ]]; then
	echo "ERROR: The variable HOST_WORKDIR has to contain a path to " \
		"the root of this project on the host machine"
	exit 1
fi


imageName=${DOCKERHUB_REPO}:1.3-${OS}-${OS_VER}
containerName=pmemkv-${OS}-${OS_VER}

if [[ "$command" == "" ]]; then
	case $TYPE in
		debug)
			builds=(tests_gcc_debug_cpp11
					tests_gcc_debug_cpp14)
			command="./run-build.sh ${builds[@]}";
			;;
		release)
			builds=(tests_clang_release_cpp20
					test_release_installation)
			command="./run-build.sh ${builds[@]}";
			;;
		valgrind)
			builds=(tests_gcc_debug_cpp14_valgrind_other)
			command="./run-build.sh ${builds[@]}";
			;;
		memcheck_drd)
			builds=(tests_gcc_debug_cpp14_valgrind_memcheck_drd)
			command="./run-build.sh ${builds[@]}";
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
		doc)
			if [[ -z "${DOC_UPDATE_BOT_NAME}" || -z "${DOC_UPDATE_GITHUB_TOKEN}" ]]; then
				echo "${doc_varialbes_error}"
				exit 0
			fi
			command="./run-doc-update.sh";
			;;
		*)
			echo "ERROR: wrong build TYPE"
			exit 1
			;;
	esac
fi

if [ "$COVERAGE" == "1" ]; then
	docker_opts="${docker_opts} `bash <(curl -s https://codecov.io/env)`";
fi

if [ -n "$DNS_SERVER" ]; then DNS_SETTING=" --dns=$DNS_SERVER "; fi

# Check if we are running on a CI (Travis or GitHub Actions)
[ -n "$GITHUB_ACTIONS" -o -n "$TRAVIS" ] && CI_RUN="YES" || CI_RUN="NO"

# do not allocate a pseudo-TTY if we are running on GitHub Actions
[ ! $GITHUB_ACTIONS ] && TTY='-t' || TTY=''

WORKDIR=/pmemkv
SCRIPTSDIR=$WORKDIR/utils/docker

echo Building on ${OS}-${OS_VER}

# Run a container with
#  - environment variables set (--env)
#  - host directory containing source mounted (-v)
#  - working directory set (-w)
docker run --privileged=true --name=$containerName -i $TTY \
	$DNS_SETTING \
	${docker_opts} \
	--env http_proxy=$http_proxy \
	--env https_proxy=$https_proxy \
	--env TERM=xterm-256color \
	--env WORKDIR=$WORKDIR \
	--env SCRIPTSDIR=$SCRIPTSDIR \
	--env COVERAGE=$COVERAGE \
	--env AUTO_DOC_UPDATE=$AUTO_DOC_UPDATE \
	--env CI_RUN=$CI_RUN \
	--env TRAVIS=$TRAVIS \
	--env GITHUB_REPO=$GITHUB_REPO \
	--env CI_COMMIT_RANGE=$CI_COMMIT_RANGE \
	--env CI_COMMIT=$CI_COMMIT \
	--env CI_REPO_SLUG=$CI_REPO_SLUG \
	--env CI_BRANCH=$CI_BRANCH \
	--env CI_EVENT_TYPE=$CI_EVENT_TYPE \
	--env DOC_UPDATE_GITHUB_TOKEN=$DOC_UPDATE_GITHUB_TOKEN \
	--env DOC_UPDATE_BOT_NAME=$DOC_UPDATE_BOT_NAME \
	--env DOC_REPO_OWNER=$DOC_REPO_OWNER \
	--env GITHUB_TOKEN=$GITHUB_TOKEN \
	--env GITHUB_UPSTREAM_USER_NAME=$GITHUB_USER_NAME \
	--env COVERITY_SCAN_TOKEN=$COVERITY_SCAN_TOKEN \
	--env COVERITY_SCAN_NOTIFICATION_EMAIL=$COVERITY_SCAN_NOTIFICATION_EMAIL \
	--env TEST_PACKAGES=${TEST_PACKAGES:-ON} \
	--env TESTS_LONG=${TESTS_LONG:-OFF} \
	--env BUILD_JSON_CONFIG=${BUILD_JSON_CONFIG:-ON} \
	--env CHECK_CPP_STYLE=${CHECK_CPP_STYLE:-ON} \
	--env DEFAULT_TEST_DIR=/dev/shm \
	--shm-size=4G \
	-v $HOST_WORKDIR:$WORKDIR \
	-v /etc/localtime:/etc/localtime \
	-w $SCRIPTSDIR \
	$imageName $command
