#include "kvdk_status.h"

namespace pmem
{
namespace kv
{

status map_kvdk_status(kvdk::Status s)
{
	switch (s) {
		case kvdk::Status::Ok:
			return status::OK;
		case kvdk::Status::NotFound:
			return status::NOT_FOUND;
		case kvdk::Status::MemoryOverflow:
			return status::OUT_OF_MEMORY;
		case kvdk::Status::PmemOverflow:
			return status::OUT_OF_MEMORY;
		case kvdk::Status::NotSupported:
			return status::NOT_SUPPORTED;
		default:
			return status::UNKNOWN_ERROR;
	}
}

} /* namespace kv */
} /* namespace pmem */
