#
# Copyright 2019, Intel Corporation
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

#
# packages.cmake - CPack configuration for rpm and deb generation
#

# Converts list of ${dep_list} dependencies to the ${str} string of debian format
function(parse_dep_list_to_deb_format dep_list str)
	set(res_list "")
	foreach(dep ${dep_list})
		string(REPLACE " " ";" words_list ${dep})
		list(LENGTH words_list LEN)
		# reformat if version requirement was set
		if (${LEN} EQUAL 3)
			list(GET words_list 0 LIBRARY)
			list(GET words_list 1 OPERATOR)
			list(GET words_list 2 VERSION)
			list(APPEND res_list "${LIBRARY} (${OPERATOR} ${VERSION})")
			continue()
		endif()
		list(APPEND res_list ${words_list})
	endforeach(dep)
	string(REPLACE ";" ", " DEB_PACKAGE_REQUIRES "${res_list}")
	set(${str} "${DEB_PACKAGE_REQUIRES}" PARENT_SCOPE)
endfunction(parse_dep_list_to_deb_format)

string(TOUPPER "${CPACK_GENERATOR}" CPACK_GENERATOR)

if(NOT ("${CPACK_GENERATOR}" STREQUAL "DPKG" OR
	"${CPACK_GENERATOR}" STREQUAL "RPM"))
	message(FATAL_ERROR "Wrong CPACK_GENERATOR value, valid generators are: DPKG, RPM")
endif()

set(CPACK_PACKAGING_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")
set(CMAKE_INSTALL_TMPDIR /tmp CACHE PATH "Output dir for tmp")
set(CPACK_COMPONENTS_ALL_IN_ONE)

# Filter out some of directories from %dir section, which are expected
# to exist in filesystem. Leaving them might lead to conflicts with other
# packages (for example with 'filesystem' package on fedora which specify
# /usr, /usr/local, etc.)
set(CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION
	${CPACK_PACKAGING_INSTALL_PREFIX}
	${CPACK_PACKAGING_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}
	${CPACK_PACKAGING_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/pkgconfig
	${CPACK_PACKAGING_INSTALL_PREFIX}/${CMAKE_INSTALL_INCDIR}
	${CPACK_PACKAGING_INSTALL_PREFIX}/share
	${CPACK_PACKAGING_INSTALL_PREFIX}/share/doc)

set(CPACK_PACKAGE_NAME "libpmemkv")
set(CPACK_PACKAGE_VERSION ${VERSION})
set(CPACK_PACKAGE_VERSION_MAJOR ${VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${VERSION_MINOR})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Key/Value Datastore for Persistent Memory")
set(CPACK_PACKAGE_VENDOR "Intel")

set(CPACK_RPM_PACKAGE_NAME "libpmemkv-devel")
set(CPACK_RPM_PACKAGE_GROUP "Development/Libraries")
set(CPACK_RPM_PACKAGE_LICENSE "BSD")
set(CPACK_RPM_PACKAGE_ARCHITECTURE x86_64)

string(REPLACE ";" ", " RPM_PACKAGE_REQUIRES "${DEPENDENCY_LIST}")
string(REPLACE "libpmemobj++" "libpmemobj++-devel" RPM_PACKAGE_REQUIRES ${RPM_PACKAGE_REQUIRES})
set(CPACK_RPM_PACKAGE_REQUIRES ${RPM_PACKAGE_REQUIRES})
#set(CPACK_RPM_CHANGELOG_FILE ${CMAKE_SOURCE_DIR}/ChangeLog)

set(CPACK_DEBIAN_PACKAGE_NAME "libpmemkv-dev")
set(CPACK_DEBIAN_PACKAGE_VERSION ${CPACK_PACKAGE_VERSION})
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE amd64)
parse_dep_list_to_deb_format("${DEPENDENCY_LIST}" DEB_PACKAGE_REQUIRES)
string(REPLACE "libpmemobj++" "libpmemobj++-dev" DEB_PACKAGE_REQUIRES ${DEB_PACKAGE_REQUIRES})
set(CPACK_DEBIAN_PACKAGE_DEPENDS ${DEB_PACKAGE_REQUIRES})
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "marcin.slusarz@intel.com")

if("${CPACK_GENERATOR}" STREQUAL "RPM")
	set(CPACK_PACKAGE_FILE_NAME
		${CPACK_RPM_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}.${CPACK_RPM_PACKAGE_ARCHITECTURE})
elseif("${CPACK_GENERATOR}" STREQUAL "DPKG")
	# We are using "gnutar" to avoid this bug:
	# https://gitlab.kitware.com/cmake/cmake/issues/14332
	set(CPACK_DEBIAN_ARCHIVE_TYPE "gnutar")
	set(CPACK_PACKAGE_FILE_NAME
		${CPACK_DEBIAN_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}_${CPACK_DEBIAN_PACKAGE_ARCHITECTURE})
endif()

set(targetDestDir ${CMAKE_INSTALL_TMPDIR})
include(CPack)
