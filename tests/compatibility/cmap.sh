#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2020, Intel Corporation

#
# cmap.sh -- runs cmap compatibility test
#

set -e

binary1=$1
binary2=$2
testfile=$3

rm -f $testfile
$binary1 $testfile create
$binary2 $testfile open

rm -f $testfile
$binary1 $testfile create_ungraceful
$binary2 $testfile open

rm -f $testfile
$binary2 $testfile create
$binary1 $testfile open

rm -f $testfile
$binary2 $testfile create_ungraceful
$binary1 $testfile open

rm -f $testfile
