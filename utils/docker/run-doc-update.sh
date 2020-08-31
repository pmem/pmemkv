#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2018-2020, Intel Corporation

#
# run-doc-update.sh - is called inside a Docker container,
#                     build docs and automatically update manpages
#                     and doxygen files on gh-pages
#

set -e

source `dirname $0`/valid-branches.sh

BOT_NAME=${DOC_UPDATE_BOT_NAME:-"pmem-bot"}
DOC_REPO_OWNER="${DOC_REPO_OWNER:-"pmem"}"
REPO_NAME="pmemkv"
ARTIFACTS_DIR=$(mktemp -d -t ARTIFACTS-XXX)

ORIGIN="https://${DOC_UPDATE_GITHUB_TOKEN}@github.com/${BOT_NAME}/${REPO_NAME}"
UPSTREAM="https://github.com/${DOC_REPO_OWNER}/${REPO_NAME}"
# master or stable-* branch
TARGET_BRANCH=${CI_BRANCH}
VERSION=${TARGET_BRANCHES[$TARGET_BRANCH]}
export GITHUB_TOKEN=${DOC_UPDATE_GITHUB_TOKEN}

if [ -z $VERSION ]; then
	echo "Target location for branch ${TARGET_BRANCH} is not defined."
	exit 1
fi
REPO_DIR=$(mktemp -d -t pmemkv-XXX)
pushd ${REPO_DIR}
# Clone repo
git clone ${ORIGIN} ${REPO_DIR}
cd ${REPO_DIR}
git remote add upstream ${UPSTREAM}

git config --local user.name ${BOT_NAME}
git config --local user.email "${BOT_NAME}@intel.com"
hub config --global hub.protocol https

git remote update
git checkout -B ${TARGET_BRANCH} upstream/${TARGET_BRANCH}

# Build docs
mkdir -p ${REPO_DIR}/build
cd ${REPO_DIR}/build

cmake .. -DBUILD_TESTS=OFF -DBUILD_EXAMPLES=OFF
make -j$(nproc) doc
cp ${REPO_DIR}/build/man/tmp/*.md ${REPO_DIR}/doc/
cp -r ${REPO_DIR}/doc ${ARTIFACTS_DIR}/
cp -r ${REPO_DIR}/build/doc/cpp_html ${ARTIFACTS_DIR}/

cd ${REPO_DIR}

# Checkout gh-pages and copy docs
GH_PAGES_NAME="gh-pages-for-${TARGET_BRANCH}"
git checkout -B ${GH_PAGES_NAME} upstream/gh-pages
git clean -dfx

# Clean old content, since some files might have been deleted
rm -r ./${VERSION}
mkdir -p ./${VERSION}/manpages/
mkdir -p ./${VERSION}/doxygen/

# copy all manpages (with format like <manpage>.<section>.md)
cp -f ${ARTIFACTS_DIR}/doc/*.*.md ./${VERSION}/manpages/
cp -fr ${ARTIFACTS_DIR}/cpp_html/* ./${VERSION}/doxygen/

# Fix the title tag:
# get rid of _MP macro, it changes e.g. "_MP(PMEMKV, 7)" to "PMEMKV"
sed -i 's/^title:\ _MP(*\([A-Za-z_-]*\).*$/title:\ \1/g' ./${VERSION}/manpages/*.md

# Add and push changes.
# git commit command may fail if there is nothing to commit.
# In that case we want to force push anyway (there might be open pull request with
# changes which were reverted).
git add -A
git commit -m "doc: automatic gh-pages docs update" && true
git push -f ${ORIGIN} ${GH_PAGES_NAME}

# Makes pull request.
# When there is already an open PR or there are no changes an error is thrown, which we ignore.
hub pull-request -f -b ${DOC_REPO_OWNER}:gh-pages -h ${BOT_NAME}:${GH_PAGES_NAME} -m "doc: automatic gh-pages docs update" && true

popd
exit 0
