#!/usr/bin/env bash
#
# Copyright 2018-2019, Intel Corporation
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

set -e

ORIGIN="https://${GITHUB_TOKEN}@github.com/pmem-bot/pmemkv"
UPSTREAM="https://github.com/pmem/pmemkv"

# Clone repo
git clone ${ORIGIN}
cd pmemkv
git remote add upstream ${UPSTREAM}

git config --local user.name "pmem-bot"
git config --local user.email "pmem-bot@intel.com"

git checkout master
git remote update
git rebase upstream/master

# Build docs
mkdir build
cd build

cmake -DBUILD_TESTING=OFF -DBUILD_EXAMPLES=OFF ..
make doc
cp man/tmp/*.md ../doc/
cp -r ../doc ../..
cp -r doc/cpp_html ../..

cd ..

# Checkout gh-pages and copy docs
git checkout -fb gh-pages upstream/gh-pages
git clean -df
cp -f ../doc/*.md manpages/master/
cp -fr ../cpp_html/* master/doxygen/

# fix the title tag
sed -i 's/^title:\ _MP(*\([A-Za-z_-]*\).*$/title:\ \1/g' manpages/master/*.md

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
