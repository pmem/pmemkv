#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2018-2021, Intel Corporation

#
# run-doc-update.sh - is called inside a Docker container,
#		to build docs for 'valid branches' and to create a pull request
#		with and update of doxygen and manpage files (on gh-pages).
#

set -e

if [[ -z "${DOC_UPDATE_GITHUB_TOKEN}" || -z "${DOC_UPDATE_BOT_NAME}" || -z "${DOC_REPO_OWNER}" ]]; then
	echo "To build documentation and upload it as a Github pull request, variables " \
		"'DOC_UPDATE_BOT_NAME', 'DOC_REPO_OWNER' and 'DOC_UPDATE_GITHUB_TOKEN' have to " \
		"be provided. For more details please read CONTRIBUTING.md"
	exit 0
fi

# Set up required variables
BOT_NAME=${DOC_UPDATE_BOT_NAME}
DOC_REPO_OWNER=${DOC_REPO_OWNER}
REPO_NAME=${REPO:-"pmemkv"}
export GITHUB_TOKEN=${DOC_UPDATE_GITHUB_TOKEN} # export for hub command
REPO_DIR=$(mktemp -d -t pmemkv-XXX)
ARTIFACTS_DIR=$(mktemp -d -t ARTIFACTS-XXX)

# Only 'master' or 'stable-*' branches are valid; determine docs location dir on gh-pages branch
TARGET_BRANCH=${CI_BRANCH}
if [[ "${TARGET_BRANCH}" == "master" ]]; then
	TARGET_DOCS_DIR="master"
elif [[ ${TARGET_BRANCH} == stable-* ]]; then
	TARGET_DOCS_DIR=v$(echo ${TARGET_BRANCH} | cut -d"-" -f2 -s)
else
	echo "Skipping docs build, this script should be run only on master or stable-* branches."
	echo "TARGET_BRANCH is set to: \'${TARGET_BRANCH}\'."
	exit 0
fi
if [ -z "${TARGET_DOCS_DIR}" ]; then
	echo "ERROR: Target docs location for branch: ${TARGET_BRANCH} is not set."
	exit 1
fi

ORIGIN="https://${GITHUB_TOKEN}@github.com/${BOT_NAME}/${REPO_NAME}"
UPSTREAM="https://github.com/${DOC_REPO_OWNER}/${REPO_NAME}"

pushd ${REPO_DIR}
echo "Clone repo:"
git clone ${ORIGIN} ${REPO_DIR}
cd ${REPO_DIR}
git remote add upstream ${UPSTREAM}

git config --local user.name ${BOT_NAME}
git config --local user.email "${BOT_NAME}@intel.com"
hub config --global hub.protocol https

git remote update
git checkout -B ${TARGET_BRANCH} upstream/${TARGET_BRANCH}

echo "Build docs:"
mkdir -p ${REPO_DIR}/build
cd ${REPO_DIR}/build

cmake .. -DBUILD_TESTS=OFF -DBUILD_EXAMPLES=OFF
make -j$(nproc) doc
cp ${REPO_DIR}/build/doc/man/tmp/*.md ${REPO_DIR}/doc/
cp -r ${REPO_DIR}/doc ${ARTIFACTS_DIR}/
cp -r ${REPO_DIR}/build/doc/cpp_html ${ARTIFACTS_DIR}/

cd ${REPO_DIR}

# Checkout gh-pages and copy docs
GH_PAGES_NAME="${TARGET_DOCS_DIR}-gh-pages-update"
git checkout -B ${GH_PAGES_NAME} upstream/gh-pages
git clean -dfx

# Clean old content, since some files might have been deleted
rm -rf ./${TARGET_DOCS_DIR}
mkdir -p ./${TARGET_DOCS_DIR}/manpages/
mkdir -p ./${TARGET_DOCS_DIR}/doxygen/

# copy all manpages (with format like <manpage>.<section>.md)
cp -f ${ARTIFACTS_DIR}/doc/*.*.md ./${TARGET_DOCS_DIR}/manpages/
cp -fr ${ARTIFACTS_DIR}/cpp_html/* ./${TARGET_DOCS_DIR}/doxygen/

# Fix the title tag in manpages:
# get rid of _MP macro, it changes e.g. "_MP(PMEMKV, 7)" to "PMEMKV"
sed -i 's/^title:\ _MP(*\([A-Za-z_-]*\).*$/title:\ \1/g' ./${TARGET_DOCS_DIR}/manpages/*.md

echo "Add and push changes:"
# git commit command may fail if there is nothing to commit.
# In that case we want to force push anyway (there might be open pull request with
# changes which were reverted).
git add -A
git commit -m "doc: automatic gh-pages docs update" && true
git push -f ${ORIGIN} ${GH_PAGES_NAME}

# Makes pull request.
# When there is already an open PR or there are no changes an error is thrown, which we ignore.
hub pull-request -f -b ${DOC_REPO_OWNER}:gh-pages -h ${BOT_NAME}:${GH_PAGES_NAME} \
	-m "doc: automatic gh-pages docs update" && true

popd
