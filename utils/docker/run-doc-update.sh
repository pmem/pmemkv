#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2018-2022, Intel Corporation

#
# run-doc-update.sh - is called inside a Docker container to build docs in the repository within ${WORKDIR}
#		and it creates a pull request with an update of our docs (on 'main' branch of pmem.github.io repo).
#

set -e

source $(dirname ${0})/prepare-for-build.sh

if [[ -z "${DOC_UPDATE_GITHUB_TOKEN}" || -z "${DOC_UPDATE_BOT_NAME}" || -z "${DOC_REPO_OWNER}" ]]; then
	echo "To build documentation and upload it as a Github pull request, variables " \
		"'DOC_UPDATE_BOT_NAME', 'DOC_REPO_OWNER' and 'DOC_UPDATE_GITHUB_TOKEN' have to " \
		"be provided. For more details please read CONTRIBUTING.md"
	exit 0
fi

if [[ -z "${WORKDIR}" ]]; then
	echo "ERROR: The variable WORKDIR has to contain a path to the root " \
		"of this project - 'build' sub-directory may be created there."
	exit 1
fi

# Set up required variables
BOT_NAME=${DOC_UPDATE_BOT_NAME}
DOC_REPO_OWNER=${DOC_REPO_OWNER}
DOC_REPO_NAME=${DOC_REPO_NAME:-"${DOC_REPO_OWNER}.github.io"}
export GITHUB_TOKEN=${DOC_UPDATE_GITHUB_TOKEN} # export for hub command
DOC_REPO_DIR=$(mktemp -d -t pmem_io-XXX)
ARTIFACTS_DIR=$(mktemp -d -t ARTIFACTS-XXX)

ORIGIN="https://${GITHUB_TOKEN}@github.com/${BOT_NAME}/${DOC_REPO_NAME}"
UPSTREAM="https://github.com/${DOC_REPO_OWNER}/${DOC_REPO_NAME}"

# Determine docs location directory, based on the branch (for CI builds: 'master' or 'vX.Y')
if [[ ${CI_BRANCH} == stable-* ]]; then
	DOCS_TARGET_DIR=$(echo ${CI_BRANCH} | awk -F 'stable-' '/stable-/{printf "v"} {print $(NF)}')
else
	DOCS_TARGET_DIR=${CI_BRANCH}
fi

echo "Build docs"
pushd ${WORKDIR}
mkdir -p build
pushd build

cmake .. -DBUILD_DOC=ON -DBUILD_TESTS=OFF -DBUILD_EXAMPLES=OFF
make -j$(nproc) doc
cp man/tmp/*.md ${WORKDIR}/doc/
cp -r doc/cpp_html ${ARTIFACTS_DIR}/
popd
cp -r doc ${ARTIFACTS_DIR}/
popd

echo "Clone pmem.io repo (with web content and our docs):"
git clone --depth=1 ${ORIGIN} ${DOC_REPO_DIR}
pushd ${DOC_REPO_DIR}
git remote add upstream ${UPSTREAM}
git fetch upstream

git config --local user.name ${BOT_NAME}
git config --local user.email "${BOT_NAME}@intel.com"
hub config --global hub.protocol https

echo "Checkout new branch (based on 'main') for PR"
DOCS_BRANCH_NAME="pmemkv-${DOCS_TARGET_DIR}-docs-update"
git checkout -B ${DOCS_BRANCH_NAME} upstream/main
git clean -dfx

DOCS_CONTENT_DIR="./content/pmemkv/${DOCS_TARGET_DIR}/"
echo "Clean old content, since some files might have been deleted"
rm -rf ${DOCS_CONTENT_DIR}
mkdir -p ${DOCS_CONTENT_DIR}/manpages/
mkdir -p ${DOCS_CONTENT_DIR}/doxygen/

echo "Copy all content"
# manpages (.md files with format like <manpage>.<section>.md)
cp -f ${ARTIFACTS_DIR}/doc/*.*.md ${DOCS_CONTENT_DIR}/manpages/
cp -fr ${ARTIFACTS_DIR}/cpp_html/* ${DOCS_CONTENT_DIR}/doxygen/

echo "Add and push changes:"
# git commit command may fail if there is nothing to commit.
# In that case we want to force push anyway (there might be open pull request with
# changes which were reverted).
git add -A
git commit -m "pmemkv: automatic docs update for '${CI_BRANCH}'" && true
git push -f ${ORIGIN} ${DOCS_BRANCH_NAME}

echo "Make a Pull Request"
# When there is already an open PR or there are no changes an error is thrown, which we ignore.
hub pull-request -f -b ${DOC_REPO_OWNER}:main -h ${BOT_NAME}:${DOCS_BRANCH_NAME} \
	-m "pmemkv: automatic docs update for '${CI_BRANCH}'" && true

popd
