#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2019-2020, Intel Corporation

# removeNamespaces.sh - clear all existing namespaces.
set -e

MOUNT_POINT="/mnt/pmem*"

echo "Clearing all existing namespaces"
sudo umount $MOUNT_POINT || true

namespace_names=$(ndctl list -X | jq -r '.[].dev')

for n in $namespace_names
do
	sudo ndctl clear-errors $n -v
done
sudo ndctl disable-namespace all || true
sudo ndctl destroy-namespace all || true
