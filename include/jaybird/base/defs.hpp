// This file is part of https://github.com/KurtBoehm/jaybird.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

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
