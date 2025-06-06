// This file is part of https://github.com/KurtBoehm/jaybird.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef INCLUDE_JAYBIRD_BASE_UNI_VARIANT_HPP
#define INCLUDE_JAYBIRD_BASE_UNI_VARIANT_HPP

#include <type_traits>
#include <variant>

#include "thesauros/macropolis.hpp"

namespace jay {
template<typename THead, typename... TTail>
struct HaveSameSerialNameTrait
    : public std::bool_constant<(... && (thes::TypeInfo<THead>::serial_name.view() ==
                                         thes::TypeInfo<TTail>::serial_name.view()))> {};

template<typename... Ts>
requires HaveSameSerialNameTrait<Ts...>::value
struct UniVariant : public std::variant<Ts...> {
  using Parent = std::variant<Ts...>;
  using Parent::Parent;
};
} // namespace jay

template<typename... Ts>
struct thes::FlattenType<jay::UniVariant<Ts...>> {
  static constexpr auto flatten(std::variant<Ts...>&& value) {
    return flatten_variant(std::move(value));
  }
};

#endif // INCLUDE_JAYBIRD_BASE_UNI_VARIANT_HPP
