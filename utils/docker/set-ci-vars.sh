#!/usr/bin/env bash
#
# Copyright 2020, Intel Corporation
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
# set-ci-vars.sh -- set CI variables common for both:
#                   Travis and GitHub Actions CIs
#

set -e

function get_commit_range_from_last_merge {
	# get commit id of the last merge
	LAST_MERGE=$(git log --merges --pretty=%H -1)
	LAST_COMMIT=$(git log --pretty=%H -1)
	RANGE_END="HEAD"
	if [ -n "$GITHUB_ACTIONS" ] && [ "$GITHUB_EVENT_NAME" == "pull_request" ] && [ "$LAST_MERGE" == "$LAST_COMMIT" ]; then
		# GitHub Actions commits its own merge in case of pull requests
		# so the first merge commit has to be skipped.

		LAST_COMMIT=$(git log --pretty=%H -2 | tail -n1)
		LAST_MERGE=$(git log --merges --pretty=%H -2 | tail -n1)
		# If still the last commit is a merge commit it means we're manually
		# merging changes (probably back from stable branch). We have to use
		# left parent of the merge and the current commit for COMMIT_RANGE.
		if [ "$LAST_MERGE" == "$LAST_COMMIT" ]; then
			LAST_MERGE=$(git log --merges --pretty=%P -2 | tail -n1 | cut -d" " -f1)
			RANGE_END=$LAST_COMMIT
		fi
	elif [ "$LAST_MERGE" == "$LAST_COMMIT" ] &&
		([ "$TRAVIS_EVENT_TYPE" == "push" ] || [ "$GITHUB_EVENT_NAME" == "push" ]); then
		# Other case in which last commit equals last merge, is when commiting
		# a manual merge. Push events don't set proper COMMIT_RANGE.
		# It has to be then set: from merge's left parent to the current commit.
		LAST_MERGE=$(git log --merges --pretty=%P -1 | cut -d" " -f1)
	fi
	if [ "$LAST_MERGE" == "" ]; then
		# possible in case of shallow clones
		# or new repos with no merge commits yet
		# - pick up the first commit
		LAST_MERGE=$(git log --pretty=%H | tail -n1)
	fi
	COMMIT_RANGE="$LAST_MERGE..$RANGE_END"
	# make sure it works now
	if ! git rev-list $COMMIT_RANGE >/dev/null; then
		COMMIT_RANGE=""
	fi
	echo $COMMIT_RANGE
}

COMMIT_RANGE_FROM_LAST_MERGE=$(get_commit_range_from_last_merge)

if [ -n "$TRAVIS" ]; then
	CI_COMMIT=$TRAVIS_COMMIT
	CI_COMMIT_RANGE="${TRAVIS_COMMIT_RANGE/.../..}"
	CI_BRANCH=$TRAVIS_BRANCH
	CI_EVENT_TYPE=$TRAVIS_EVENT_TYPE
	CI_REPO_SLUG=$TRAVIS_REPO_SLUG

	# CI_COMMIT_RANGE is usually invalid for force pushes - fix it when used
	# with non-upstream repository
	if [ -n "$CI_COMMIT_RANGE" -a "$CI_REPO_SLUG" != "$GITHUB_REPO" ]; then
		if ! git rev-list $CI_COMMIT_RANGE; then
			CI_COMMIT_RANGE=$COMMIT_RANGE_FROM_LAST_MERGE
		fi
	fi

	case "$TRAVIS_CPU_ARCH" in
	"amd64")
		CI_CPU_ARCH="x86_64"
		;;
	*)
		CI_CPU_ARCH=$TRAVIS_CPU_ARCH
		;;
	esac

elif [ -n "$GITHUB_ACTIONS" ]; then
	CI_COMMIT=$GITHUB_SHA
	CI_COMMIT_RANGE=$COMMIT_RANGE_FROM_LAST_MERGE
	CI_BRANCH=$(echo $GITHUB_REF | cut -d'/' -f3)
	CI_REPO_SLUG=$GITHUB_REPOSITORY
	CI_CPU_ARCH="x86_64" # GitHub Actions supports only x86_64

	case "$GITHUB_EVENT_NAME" in
	"schedule")
		CI_EVENT_TYPE="cron"
		;;
	*)
		CI_EVENT_TYPE=$GITHUB_EVENT_NAME
		;;
	esac

else
	CI_COMMIT=$(git log --pretty=%H -1)
	CI_COMMIT_RANGE=$COMMIT_RANGE_FROM_LAST_MERGE
	CI_CPU_ARCH="x86_64"
fi

export CI_COMMIT=$CI_COMMIT
export CI_COMMIT_RANGE=$CI_COMMIT_RANGE
export CI_BRANCH=$CI_BRANCH
export CI_EVENT_TYPE=$CI_EVENT_TYPE
export CI_REPO_SLUG=$CI_REPO_SLUG
export CI_CPU_ARCH=$CI_CPU_ARCH

echo CI_COMMIT=$CI_COMMIT
echo CI_COMMIT_RANGE=$CI_COMMIT_RANGE
echo CI_BRANCH=$CI_BRANCH
echo CI_EVENT_TYPE=$CI_EVENT_TYPE
echo CI_REPO_SLUG=$CI_REPO_SLUG
echo CI_CPU_ARCH=$CI_CPU_ARCH
