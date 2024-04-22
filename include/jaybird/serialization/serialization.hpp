#ifndef INCLUDE_JAYBIRD_SERIALIZATION_SERIALIZATION_HPP
#define INCLUDE_JAYBIRD_SERIALIZATION_SERIALIZATION_HPP

#include <cassert>
#include <cstddef>
#include <exception>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "nlohmann/json.hpp"
#include "thesauros/concepts.hpp"
#include "thesauros/containers.hpp"
#include "thesauros/format.hpp"
#include "thesauros/macropolis.hpp"
#include "thesauros/ranges.hpp"
#include "thesauros/utility.hpp"

#include "jaybird/base.hpp"

namespace jay {
template<typename T>
struct JsonConverter;
template<typename T>
concept HasJsonConverter = thes::CompleteType<JsonConverter<T>>;

template<IsJsonCompatible T>
inline Json to_json(const T& value) {
  return value;
}

template<IsJsonCompatible T>
inline T from_json(const Json& value) {
  return value.get<T>();
}

template<typename T>
struct JsonFetcher {
  static T fetch(const Json& value, const std::string& key) {
    return from_json<T>(value.at(key));
  }
};
template<typename T>
struct JsonFetcher<std::optional<T>> {
  static std::optional<T> fetch(const Json& value, const std::string& key) {
    auto it = value.find(key);
    if (it == value.end()) {
      return std::nullopt;
    }
    return from_json<std::optional<T>>(*it);
  }
};
template<typename T>
inline T json_fetch(const Json& value, const std::string& key) {
  return JsonFetcher<T>::fetch(value, key);
}

template<HasJsonMembers T>
struct JsonConverter<T> {
  static Json to(const T& value) {
    return value.to_json();
  }

  static T from(const Json& json) {
    return T::from_json(json);
  }
};

struct StaticError final {
  StaticError(std::string_view key, const Json& value, const Json& ref_value)
      : what_{
          fmt::format("The value of key {} is {}, not {}!", key, value.dump(), ref_value.dump())} {}
  explicit StaticError(std::string what) : what_{std::move(what)} {}

  [[nodiscard]] const char* what() const noexcept {
    return what_.c_str();
  }

  [[nodiscard]] auto exception() const {
    return std::runtime_error{what_};
  }

private:
  std::string what_;
};

template<IsJsonCompatible T>
inline std::optional<StaticError> static_check(const Json& json) {
  if constexpr (requires { JsonConverter<T>::static_check(json); }) {
    return JsonConverter<T>::static_check(json);
  } else {
    try {
      json.get<T>();
      return std::nullopt;
    } catch (const std::exception& ex) {
      return StaticError{ex.what()};
    }
  }
}

template<HasTypeInfo T>
struct JsonConverter<T> {
  using Info = thes::TypeInfo<T>;

  static std::optional<StaticError> static_check(const Json& json) {
    if constexpr (std::tuple_size_v<decltype(Info::static_members)> == 0) {
      return std::nullopt;
    } else {
      return std::apply(
        [&](const auto&... members) {
          auto impl = [&](auto& rec, const auto& head,
                          const auto&... tail) -> std::optional<StaticError> {
            const auto key = head.serial_name.view();
            const Json& value = json.at(key);
            const Json ref_value = thes::serial_value(head.value);
            if (value != ref_value) {
              return StaticError{key, value, ref_value};
            }
            if constexpr (sizeof...(tail) > 0) {
              return rec(rec, tail...);
            } else {
              return std::nullopt;
            }
          };
          return impl(impl, members...);
        },
        Info::static_members);
    }
  }

  static Json to(const T& value) {
    auto json = Json::object();

    auto static_member_impl = [&]<typename... TMembers>(TMembers... /*members*/) {
      ((json[TMembers::serial_name.view()] = thes::serial_value(TMembers::value)), ...);
    };
    std::apply(static_member_impl, Info::static_members);

    auto member_impl = [&]<typename... TMembers>(TMembers... /*members*/) {
      ((json[TMembers::serial_name.view()] = to_json(value.*TMembers::pointer)), ...);
    };
    std::apply(member_impl, Info::members);

    return json;
  }

  static T from(const Json& json) {
    if (const auto err = static_check(json); err.has_value()) {
      throw err->exception();
    }
    return std::apply(
      [&]<typename... TMembers>(TMembers... /*members*/) {
        return T(
          json_fetch<typename TMembers::Type>(json, std::string{TMembers::serial_name.view()})...);
      },
      Info::members);
  }
};

template<HasEnumInfo T>
struct JsonConverter<T> {
  using EnumInfo = thes::EnumInfo<T>;

  static Json to(const T& value) {
    auto impl = [&]<std::size_t tHead, std::size_t... tTail>(
                  auto rec, std::index_sequence<tHead, tTail...>) THES_ALWAYS_INLINE -> Json {
      constexpr auto value_info = std::get<tHead>(EnumInfo::values);
      if (value_info.value == value) {
        return value_info.serial_name.view();
      }
      if constexpr (sizeof...(tTail) > 0) {
        return rec(rec, std::index_sequence<tTail...>{});
      } else {
        throw std::invalid_argument{
          fmt::format("The value {} is not a valid value for the enum {}!",
                      static_cast<std::underlying_type_t<T>>(value), EnumInfo::name.view())};
      }
    };
    return impl(impl, std::make_index_sequence<std::tuple_size_v<decltype(EnumInfo::values)>>{});
  }

  static T from(const Json& json) {
    const std::string& value = json.get<std::string>();

    auto impl = [&]<std::size_t tHead, std::size_t... tTail>(
                  auto rec, std::index_sequence<tHead, tTail...>) THES_ALWAYS_INLINE -> T {
      constexpr auto value_info = std::get<tHead>(EnumInfo::values);
      if (value_info.serial_name.view() == value) {
        return value_info.value;
      }
      if constexpr (sizeof...(tTail) > 0) {
        return rec(rec, std::index_sequence<tTail...>{});
      } else {
        throw std::invalid_argument{
          fmt::format("The value {} is not a valid value for the enum {}!",
                      static_cast<std::underlying_type_t<T>>(value), EnumInfo::name.view())};
      }
    };
    return impl(impl, std::make_index_sequence<std::tuple_size_v<decltype(EnumInfo::values)>>{});
  }
};

template<IsJsonCompatible T>
struct JsonConverter<std::optional<T>> {
  static Json to(const std::optional<T>& value) {
    if (value.has_value()) {
      return to_json(*value);
    }
    return {};
  }

  static std::optional<T> from(const Json& json) {
    if (json.is_null()) {
      return std::nullopt;
    }
    return from_json<T>(json);
  }
};

template<typename... Ts>
requires(sizeof...(Ts) > 0 && (... && thes::HasSerialName<Ts>))
struct JsonConverter<std::variant<Ts...>> {
  using Var = std::variant<Ts...>;

  static std::string error_msg(const std::vector<StaticError>& errors) {
    auto msg = fmt::format("This is not a known variant! Keys: {}",
                           thes::Tuple{thes::serial_name_of<Ts>()...} | thes::star::format);
    if (!errors.empty()) {
      msg += fmt::format(
        "\nErrors that occurred when checking variants with the same name: {}",
        thes::transform_range([](const StaticError& err) { return err.what(); }, errors));
    }
    return msg;
  }

  static Json to(const Var& value) {
    return std::visit(
      []<typename T>(const T& var) {
        using Info = thes::TypeInfo<T>;
        return Json{{Info::serial_name.view(), to_json(var)}};
      },
      value);
  }

  static Var from(const Json& json) {
    if (json.size() != 1) {
      throw std::invalid_argument("A variant JSON needs to be an object with a single entry!");
    }
    auto it = json.begin();
    const auto& key = it.key();
    const auto& value = it.value();

    std::vector<StaticError> errors{};
    auto impl = [&]<typename THead, typename... TTail>(auto rec, const THead& /*head*/,
                                                       const TTail&... tail) -> Var {
      using Type = typename THead::Type;

      if (thes::serial_name_of<Type>().view() == key) {
        if (auto err = static_check<Type>(value); err.has_value()) {
          errors.push_back(std::move(*err));
        } else {
          // TODO Always returns the first hit!
          return from_json<Type>(value);
        }
      }
      if constexpr (sizeof...(TTail) > 0) {
        return rec(rec, tail...);
      } else {
        throw std::invalid_argument{error_msg(errors)};
      }
    };
    return impl(impl, thes::type_tag<Ts>...);
  }
};

template<typename... Ts>
requires(sizeof...(Ts) > 0 && (... && HasTypeInfo<Ts>))
struct JsonConverter<UniVariant<Ts...>> {
  using Var = UniVariant<Ts...>;

  static std::string error_msg(const std::vector<StaticError>& errors) {
    auto msg = fmt::format("This is not a known variant! Keys: {}",
                           thes::Tuple{thes::serial_name_of<Ts>()...} | thes::star::format);
    if (!errors.empty()) {
      msg += fmt::format(
        "\nErrors that occurred when checking variants with the same name: {}",
        thes::transform_range([](const StaticError& err) { return err.what(); }, errors));
    }
    return msg;
  }

  static Json to(const Var& value) {
    return std::visit([]<typename T>(const T& var) { return to_json(var); }, value);
  }

  static Var from(const Json& json) {
    std::vector<StaticError> errors{};
    auto impl = [&]<typename THead, typename... TTail>(auto rec, const THead& /*head*/,
                                                       const TTail&... tail) -> Var {
      using Type = typename THead::Type;

      if (auto err = static_check<Type>(json); err.has_value()) {
        errors.push_back(std::move(*err));
      } else {
        // TODO Always returns the first hit!
        return from_json<Type>(json);
      }

      if constexpr (sizeof...(TTail) > 0) {
        return rec(rec, tail...);
      } else {
        throw std::invalid_argument{error_msg(errors)};
      }
    };
    return impl(impl, thes::type_tag<Ts>...);
  }
};

template<IsJsonCompatible T, std::size_t tCapacity>
struct JsonConverter<thes::LimitedArray<T, tCapacity>> {
  using Arr = thes::LimitedArray<T, tCapacity>;

  static Json to(const Arr& arr) {
    auto out = Json::array();
    for (const auto& v : arr) {
      out.push_back(to_json(v));
    }
    return out;
  }

  static Arr from(const Json& json) {
    assert(json.size() <= tCapacity);
    auto trans = thes::transform_range([](auto v) { return from_json<T>(v); }, json);
    return Arr{trans.begin(), trans.end()};
  }
};
} // namespace jay

namespace nlohmann {
template<jay::HasJsonConverter T>
struct adl_serializer<T> {
  using Converter = jay::JsonConverter<T>;

  static T from_json(const json& j) {
    return Converter::from(j);
  }

  static void to_json(json& j, const T& value) {
    j = Converter::to(value);
  }
};
} // namespace nlohmann

#endif // INCLUDE_JAYBIRD_SERIALIZATION_SERIALIZATION_HPP
