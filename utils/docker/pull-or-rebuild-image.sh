#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2016-2020, Intel Corporation

#
# pull-or-rebuild-image.sh - rebuilds the Docker image used in the
#                            current Travis build if necessary.
#
# The script rebuilds the Docker image if:
# 1. the Dockerfile for the current OS version (Dockerfile.${OS}-${OS_VER})
#    or any .sh script in the Dockerfiles directory were modified and committed, or
# 2. "rebuild" param was passed as first argument to this script.
#
# If the Travis build is not of the "pull_request" type (i.e. in case of
# merge after pull_request) and it succeed, the Docker image should be pushed
# to the Docker Hub repository. An empty file is created to signal that to
# further scripts.
#
# If the Docker image does not have to be rebuilt, it will be pulled from
# Docker Hub.
#

set -e

source $(dirname $0)/set-ci-vars.sh
source $(dirname $0)/set-vars.sh

if [[ -z "$OS" || -z "$OS_VER" ]]; then
	echo "ERROR: The variables OS and OS_VER have to be set properly " \
             "(eg. OS=fedora, OS_VER=31)."
	exit 1
fi

if [[ -z "$HOST_WORKDIR" ]]; then
	echo "ERROR: The variable HOST_WORKDIR has to contain a path to " \
		"the root of this project on the host machine"
	exit 1
fi

# Path to directory with Dockerfiles and image building scripts
images_dir_name=images
base_dir=utils/docker/$images_dir_name

# If "rebuild" param is passed to script, force rebuild
if [[ "$1" == "rebuild" ]]; then
	pushd $images_dir_name
	./build-image.sh ${OS}-${OS_VER}
	popd
	exit 0
fi

# Find all the commits for the current build
if [ -n "$CI_COMMIT_RANGE" ]; then
	commits=$(git rev-list $CI_COMMIT_RANGE)
else
	commits=$CI_COMMIT
fi

echo "Commits in the commit range:"
for commit in $commits; do echo $commit; done

# Get the list of files modified by the commits
files=$(for commit in $commits; do git diff-tree --no-commit-id --name-only \
	-r $commit; done | sort -u)
echo "Files modified within the commit range:"
for file in $files; do echo $file; done

# Check if committed file modifications require the Docker image to be rebuilt
for file in $files; do
	# Check if modified files are relevant to the current build
	if [[ $file =~ ^($base_dir)\/Dockerfile\.($OS)-($OS_VER)$ ]] \
		|| [[ $file =~ ^($base_dir)\/.*\.sh$ ]]
	then
		# Rebuild Docker image for the current OS version
		echo "Rebuilding the Docker image for the Dockerfile.$OS-$OS_VER"
		pushd $images_dir_name
		./build-image.sh ${OS}-${OS_VER}
		popd

		# Check if the image has to be pushed to Docker Hub
		# (i.e. the build is triggered by commits to the $GITHUB_REPO
		# repository's stable-* or master branch, and the Travis build is not
		# of the "pull_request" type). In that case, create the empty
		# file.
		if [[ "$CI_REPO_SLUG" == "$GITHUB_REPO" \
			&& ($CI_BRANCH == stable-* || $CI_BRANCH == master) \
			&& $CI_EVENT_TYPE != "pull_request" \
			&& $PUSH_IMAGE == "1" ]]
		then
			echo "The image will be pushed to Docker Hub"
			touch $CI_FILE_PUSH_IMAGE_TO_REPO
		else
			echo "Skip pushing the image to Docker Hub"
		fi

		exit 0
	fi
done

# Getting here means rebuilding the Docker image is not required.
# Pull the image from Docker Hub.
docker pull ${DOCKERHUB_REPO}:1.4-${OS}-${OS_VER}
