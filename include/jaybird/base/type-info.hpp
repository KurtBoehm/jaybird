// This file is part of https://github.com/KurtBoehm/jaybird.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef INCLUDE_JAYBIRD_BASE_TYPE_INFO_HPP
#define INCLUDE_JAYBIRD_BASE_TYPE_INFO_HPP

#include <concepts>

#include "thesauros/concepts.hpp"
#include "thesauros/macropolis.hpp"

#include "jaybird/base/defs.hpp"

namespace jay {
template<typename T>
concept HasJsonMembers = requires(const Json& json, const T& self) {
  { T::from_json(json) } -> std::same_as<T>;
  { self.to_json() } -> std::same_as<Json>;
};
template<typename T>
concept JsonCompatible = requires(const Json& json, const T& self) {
  { Json(self) } -> std::same_as<Json>;
  { json.get<T>() } -> std::same_as<T>;
};

template<typename T>
concept HasTypeInfo = thes::CompleteType<thes::TypeInfo<T>>;
template<typename T>
concept HasEnumInfo = thes::CompleteType<thes::EnumInfo<T>>;
} // namespace jay

#endif // INCLUDE_JAYBIRD_BASE_TYPE_INFO_HPP
