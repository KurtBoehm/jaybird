#ifndef INCLUDE_JAYBIRD_SERIALIZATION_SERIALIZATION_HPP
#define INCLUDE_JAYBIRD_SERIALIZATION_SERIALIZATION_HPP

#include <cstddef>
#include <exception>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "nlohmann/json.hpp"
#include "thesauros/concepts.hpp"
#include "thesauros/macropolis.hpp"
#include "thesauros/utility.hpp"

#include "jaybird/base.hpp"

namespace jay {
template<typename T>
struct JsonConverter;
template<typename T>
concept HasJsonConverter = thes::CompleteType<JsonConverter<T>>;

template<typename T>
requires IsJsonCompatible<T>
inline Json to_json(const T& value) {
  return value;
}

template<typename T>
requires IsJsonCompatible<T>
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

template<typename T>
requires HasJsonMembers<T>
struct JsonConverter<T> {
  static Json to(const T& value) {
    return value.to_json();
  }

  static T from(const Json& json) {
    return T::from_json(json);
  }
};

struct StaticError final : public std::exception {
  StaticError(std::string_view key, const Json& value, const Json& ref_value)
      : what_((std::stringstream{} << "The value of key " << key << " is " << value << ", not "
                                   << ref_value << "!")
                .str()) {}

  [[nodiscard]] const char* what() const noexcept override {
    return what_.c_str();
  }

private:
  std::string what_;
};

template<typename T>
requires HasTypeInfo<T>
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
      throw *err;
    }
    return std::apply(
      [&]<typename... TMembers>(TMembers... /*members*/) {
        return T(
          json_fetch<typename TMembers::Type>(json, std::string{TMembers::serial_name.view()})...);
      },
      Info::members);
  }
};

template<typename T>
requires HasEnumInfo<T>
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
        throw std::invalid_argument(
          (std::stringstream{} << "The value " << static_cast<std::underlying_type_t<T>>(value)
                               << " is not a valid value for the enum " << EnumInfo::name.view()
                               << "!")
            .str());
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
        throw std::invalid_argument((std::stringstream{} << "The value " << value
                                                         << " is not a valid value for the enum "
                                                         << EnumInfo::name.view() << "!")
                                      .str());
      }
    };
    return impl(impl, std::make_index_sequence<std::tuple_size_v<decltype(EnumInfo::values)>>{});
  }
};

template<typename T>
requires IsJsonCompatible<T>
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
requires(sizeof...(Ts) > 0 && (... && HasTypeInfo<Ts>))
struct JsonConverter<std::variant<Ts...>> {
  using Var = std::variant<Ts...>;

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
      using Info = thes::TypeInfo<Type>;

      if (Info::serial_name.view() == key) {
        if (auto err = JsonConverter<Type>::static_check(value); err.has_value()) {
          errors.push_back(std::move(*err));
        } else {
          // TODO Always returns the first hit!
          return from_json<Type>(value);
        }
      }
      if constexpr (sizeof...(TTail) > 0) {
        return rec(rec, tail...);
      } else {
        using namespace thes::literals;

        std::stringstream stream{};
        stream << "This is not a known variant! Keys:";
        (stream << ... << (" “"_sstr + thes::serial_name_of<Ts>() + "”"_sstr).view());
        if (!errors.empty()) {
          stream << "\nErrors that occurred when checking variants with the same name:";
          for (const auto& err : errors) {
            stream << " “" << err.what() << "”";
          }
        }

        throw std::invalid_argument(stream.str());
      }
    };
    return impl(impl, thes::type_tag<Ts>...);
  }
};

template<typename... Ts>
requires(sizeof...(Ts) > 0 && (... && HasTypeInfo<Ts>))
struct JsonConverter<UniVariant<Ts...>> {
  using Var = UniVariant<Ts...>;

  static Json to(const Var& value) {
    return std::visit([]<typename T>(const T& var) { return to_json(var); }, value);
  }

  static Var from(const Json& json) {
    std::vector<StaticError> errors{};
    auto impl = [&]<typename THead, typename... TTail>(auto rec, const THead& /*head*/,
                                                       const TTail&... tail) -> Var {
      using Type = typename THead::Type;

      if (auto err = JsonConverter<Type>::static_check(json); err.has_value()) {
        errors.push_back(std::move(*err));
      } else {
        // TODO Always returns the first hit!
        return from_json<Type>(json);
      }

      if constexpr (sizeof...(TTail) > 0) {
        return rec(rec, tail...);
      } else {
        using namespace thes::literals;

        std::stringstream stream{};
        stream << "This is not a known variant! Keys:";
        (stream << ... << (" “"_sstr + thes::serial_name_of<Ts>() + "”"_sstr).view());
        if (!errors.empty()) {
          stream << "\nErrors that occurred when checking variants with the same name:";
          for (const auto& err : errors) {
            stream << " “" << err.what() << "”";
          }
        }

        throw std::invalid_argument(stream.str());
      }
    };
    return impl(impl, thes::type_tag<Ts>...);
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
