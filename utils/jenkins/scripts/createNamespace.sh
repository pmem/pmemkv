#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2019-2020, Intel Corporation

# createNamespace.sh - Remove old namespaces and create new for pmemkv tests.

set -e

# region used for dax namespaces.
DEV_DAX_R=0x0000
# region used for fsdax namespaces.
FS_DAX_R=0x0001
CREATE_DAX=false
CREATE_PMEM=false
MOUNT_POINT="/mnt/pmem0"
SIZE=100G

function usage()
{
	echo ""
	echo "Script for creating namespaces, mountpoint, and configuring file permissions."
	echo "Usage: $(basename $1) [-h|--help]  [-d|--dax] [-p|--pmem] [--size]"
	echo "-h, --help       Print help and exit"
	echo "-d, --dax        Create dax device."
	echo "-p, --pmem       Create fsdax device and create mountpoint."
	echo "--size           Set size for namespaces [default: $SIZE]"
}

function clear_namespaces() {
	scriptdir=$(readlink -f $(dirname ${BASH_SOURCE[0]}))
	$scriptdir/removeNamespaces.sh
}

function create_devdax() {
	echo "Creating devdax namespace"
	local align=$1
	local size=$2
	local cmd="sudo ndctl create-namespace --mode devdax -a ${align} -s ${size} -r ${DEV_DAX_R} -f"
	result=$(${cmd})
	if [ $? -ne 0 ]; then
		exit 1;
	fi
	jq -r '.daxregion.devices[].chardev' <<< $result
}

function create_fsdax() {
	echo "Creating fsdax namespace"
	local size=$1
	local cmd="sudo ndctl create-namespace --mode fsdax -s ${size} -r ${FS_DAX_R} -f"
	result=$(${cmd})
	if [ $? -ne 0 ]; then
		exit 1;
	fi
	jq -r '.blockdev' <<< $result
}

while getopts ":dhp-:" optchar; do
	case "${optchar}" in
		-)
		case "$OPTARG" in
			help) usage $0 && exit 0 ;;
			dax) CREATE_DAX=true ;;
			pmem) CREATE_PMEM=true ;;
			size=*) SIZE="${OPTARG#*=}" ;;
			*) echo "Invalid argument '$OPTARG'"; usage $0 && exit 1 ;;
		esac
		;;
		p) CREATE_PMEM=true ;;
		d) CREATE_DAX=true ;;
		h) usage $0 && exit 0 ;;
		*) echo "Invalid argument '$OPTARG'"; usage $0 && exit 1 ;;
	esac
done

# There is no default test cofiguration in this script. Configurations has to be specified.
if ! $CREATE_DAX && ! $CREATE_PMEM; then
	echo ""
	echo "ERROR: No config type selected. Please select one or more config types."
	exit 1
fi

# Remove existing namespaces.
clear_namespaces

# Creating namespaces.
trap 'echo "ERROR: Failed to create namespaces"; clear_namespaces; exit 1' ERR SIGTERM SIGABRT

if $CREATE_DAX; then
	create_devdax 4k $SIZE
fi

if $CREATE_PMEM; then
	pmem_name=$(create_fsdax $SIZE)
fi

# Creating mountpoint.
trap 'echo "ERROR: Failed to create mountpoint"; clear_namespaces; exit 1' ERR SIGTERM SIGABRT
if $CREATE_PMEM; then
	if [ ! -d "$MOUNT_POINT" ]; then
		sudo mkdir $MOUNT_POINT
	fi

	if ! grep -qs "$MOUNT_POINT " /proc/mounts; then
		sudo mkfs.ext4 -F /dev/$pmem_name
		sudo mount -o dax /dev/$pmem_name $MOUNT_POINT
	fi
	echo "Mount point: ${MOUNT_POINT}"
fi

echo "Changing file permissions"
sudo chmod 777 $MOUNT_POINT || true

sudo chmod 777 /dev/dax* || true
sudo chmod a+rw /sys/bus/nd/devices/region*/deep_flush
sudo chmod +r /sys/bus/nd/devices/ndbus*/region*/resource
sudo chmod +r  /sys/bus/nd/devices/ndbus*/region*/dax*/resource

echo "Print created namespaces:"
ndctl list -X | jq -r '.[] | select(.mode=="devdax") | [.daxregion.devices[].chardev, "align: "+(.daxregion.align/1024|tostring+"k"), "size: "+(.size/1024/1024/1024|tostring+"G") ]'
ndctl list | jq -r '.[] | select(.mode=="fsdax") | [.blockdev, "size: "+(.size/1024/1024/1024|tostring+"G") ]'
