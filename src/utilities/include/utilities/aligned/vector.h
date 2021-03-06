#pragma once

#include "utilities/aligned/allocator.h"

#include <vector>

namespace util {
namespace aligned {

template <typename T>
using vector = std::vector<T, allocator<T>>;

}  // namespace aligned
}  // namespace util
