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
