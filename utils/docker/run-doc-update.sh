#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2018-2019, Intel Corporation

set -e

ORIGIN="https://${GITHUB_TOKEN}@github.com/pmem-bot/pmemkv"
UPSTREAM="https://github.com/pmem/pmemkv"
CURR_DIR=$(pwd)

# Clone repo
git clone ${ORIGIN}
cd $CURR_DIR/pmemkv
git remote add upstream ${UPSTREAM}

git config --local user.name "pmem-bot"
git config --local user.email "pmem-bot@intel.com"

git checkout master
git remote update
git rebase upstream/master

# Build docs
mkdir $CURR_DIR/pmemkv/build
cd $CURR_DIR/pmemkv/build

cmake .. -DBUILD_TESTS=OFF -DBUILD_EXAMPLES=OFF
make -j$(nproc) doc
cp $CURR_DIR/pmemkv/build/man/tmp/*.md $CURR_DIR/pmemkv/doc/
cp -r $CURR_DIR/pmemkv/doc $CURR_DIR/
cp -r $CURR_DIR/pmemkv/build/doc/cpp_html $CURR_DIR/

cd $CURR_DIR/pmemkv

# Checkout gh-pages and copy docs
git checkout -fb gh-pages upstream/gh-pages
git clean -df
cp -f $CURR_DIR/doc/*.md master/manpages/
cp -fr $CURR_DIR/cpp_html/* master/doxygen/

# Fix the title tag:
# get rid of _MP macro, it changes e.g. "_MP(PMEMKV, 7)" to "PMEMKV"
sed -i 's/^title:\ _MP(*\([A-Za-z_-]*\).*$/title:\ \1/g' master/manpages/*.md

# Add and push changes.
# git commit command may fail if there is nothing to commit.
# In that case we want to force push anyway (there might be open pull request with
# changes which were reverted).
git add -A
git commit -m "doc: automatic gh-pages docs update" && true
git push -f ${ORIGIN} gh-pages

# Makes pull request.
# When there is already an open PR or there are no changes an error is thrown, which we ignore.
hub pull-request -f -b pmem:gh-pages -h pmem-bot:gh-pages -m "doc: automatic gh-pages docs update" && true

exit 0
