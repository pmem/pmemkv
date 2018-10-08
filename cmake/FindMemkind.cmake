include(FindPackageHandleStandardArgs)

find_path(MEMKIND_INCLUDE_DIR pmem_allocator.h)
find_library(MEMKIND_LIBRARY NAMES memkind libmemkind)

find_package_handle_standard_args(MEMKIND
	DEFAULT_MSG
	MEMKIND_INCLUDE_DIR
	MEMKIND_LIBRARY
	)

mark_as_advanced(MEMKIND_LIBRARY MEMKIND_INCLUDE_DIR)

if(MEMKIND_FOUND)
	set(MEMKIND_LIBRARIES ${MEMKIND_LIBRARY})
	set(MEMKIND_INCLUDE_DIRS ${MEMKIND_INCLUDE_DIR})
endif()
