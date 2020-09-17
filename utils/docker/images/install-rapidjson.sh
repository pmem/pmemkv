#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2020, Intel Corporation

#
# install-rapidjson.sh - installs rapidjson from sources
#

set -e

git clone https://github.com/Tencent/rapidjson
cd rapidjson
# master: Merge pull request #1760 from escherstair/fix_ce6_support, 07.08.2020
git checkout "ce81bc9edfe773667a7a4454ba81dac72ed4364c"

mkdir build
cd build
cmake ..
make -j$(nproc)
sudo make -j$(nproc) install

cd ../..
rm -r rapidjson
