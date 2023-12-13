#ifndef INCLUDE_JAYBIRD_BASE_DEFS_HPP
#define INCLUDE_JAYBIRD_BASE_DEFS_HPP

#include "nlohmann/json.hpp"

namespace jay {
using Json = nlohmann::json;
using Real = Json::number_float_t;
using Int = Json::number_integer_t;
using UInt = Json::number_unsigned_t;
} // namespace jay

#endif // INCLUDE_JAYBIRD_BASE_DEFS_HPP
