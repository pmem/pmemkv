#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2019-2020, Intel Corporation

# common.sh - contains bash functions used in all jenkins pipelines.

set -o pipefail

scriptdir=$(readlink -f $(dirname ${BASH_SOURCE[0]}))

function system_info {
	echo "********** system-info **********"
	cat /etc/os-release | grep -oP "PRETTY_NAME=\K.*"
	uname -r
	echo "libndctl: $(pkg-config --modversion libndctl)"
	echo "libfabric: $(pkg-config --modversion libfabric)"
	echo "**********memory-info**********"
	sudo ipmctl show -dimm
	sudo ipmctl show -topology
	echo "**********list-existing-namespaces**********"
	sudo ndctl list -M -N
	echo "**********installed-packages**********"
	zypper se --installed-only 2>/dev/null || true
	apt list --installed 2>/dev/null || true
	yum list installed 2>/dev/null || true
	echo "**********/proc/cmdline**********"
	cat /proc/cmdline
	echo "**********/proc/modules**********"
	cat /proc/modules
	echo "**********/proc/cpuinfo**********"
	cat /proc/cpuinfo
	echo "**********/proc/meminfo**********"
	cat /proc/meminfo
	eco "**********/proc/swaps**********"
	cat /proc/swaps
	echo "**********/proc/version**********"
	cat /proc/version
	echo "**********check-updates**********"
	sudo zypper list-updates 2>/dev/null || true
	sudo apt-get update 2>/dev/null || true ; apt upgrade --dry-run 2>/dev/null || true
	sudo dnf check-update 2>/dev/null || true
	echo "**********list-enviroment**********"
	env
}

function set_warning_message {
	echo $scriptdir
	cd $scriptdir &&  sudo bash -c 'cat banner >> /etc/motd'
}

function disable_warning_message {
	sudo rm /etc/motd || true
}

# Check host linux distribution and return distro name 
function check_distro {
	distro=$(cat /etc/os-release | grep -e ^NAME= | cut -c6-) && echo "${distro//\"}"
}
