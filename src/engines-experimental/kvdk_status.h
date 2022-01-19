#pragma once

#ifndef KVDK_STATUS_MAPPER_H
#define KVDK_STATUS_MAPPER_H

#include "../engine.h"

#include "kvdk/engine.hpp"

namespace pmem
{
namespace kv
{

status map_kvdk_status(kvdk::Status s);

} /* namespace kv */
} /* namespace pmem */

#endif /* KVDK_STATUS_MAPPER_H */