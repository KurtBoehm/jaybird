#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

#include "nlohmann/json.hpp"
#include "thesauros/thesauros.hpp"

#include "jaybird/jaybird.hpp"

struct Test1 {
  THES_DEFINE_TYPE(KEEP(Test1), CONSTEXPR_CONSTRUCTOR, (KEEP(a), float),
                   (KEEP(b), (std::pair<float, int>)), (KEEP(c), int))
};
struct TestTwo {
  THES_DEFINE_TYPE(SNAKE_CASE(TestTwo), CONSTEXPR_CONSTRUCTOR, (KEEP(a), float, {}),
                   (KEEP(b), Test1, (Test1{0.0, {2.0, 3}, 1})), (KEEP(c), int, {}))
};
struct Test3 {
  THES_DEFINE_TYPE(NAMED(Test3, "test_3"), CONSTEXPR_CONSTRUCTOR, (KEEP(a), float, {}),
                   (KEEP(b), (std::variant<Test1, TestTwo>), ({Test1{0.0, {2.0, 3}, 3}})))
};
struct Test4 {
  THES_DEFINE_TYPE(NAMED(Test4, "test_4"), CONSTEXPR_CONSTRUCTOR)
};

THES_DEFINE_ENUM_SIMPLE(SNAKE_CASE(Direction), bool, LOWERCASE(FORWARD), LOWERCASE(BACKWARD));
template<Direction tVal, typename TType>
struct Templ5 {
  THES_DEFINE_TYPE_EX(SNAKE_CASE(Templ5), CONSTEXPR_CONSTRUCTOR,
                      STATIC_MEMBERS(("value", tVal), ("type", thes::type_tag<TType>)),
                      MEMBERS((KEEP(a), int)))

  constexpr bool operator==(const Templ5&) const = default;
};
using Test5 = Templ5<Direction::FORWARD, float>;

using Members = std::decay_t<decltype(thes::TypeInfo<Test1>::members)>;
using Member0 = std::tuple_element_t<0, Members>;
inline constexpr auto name = Member0::name;
inline constexpr auto ptr = Member0::pointer;

int main() {
  using jay::Json;

  {
    constexpr Test1 test{0.0, {2.0, 3}, 1};
    constexpr auto a = test.*ptr;
    static_assert(a == 0.0F);

    THES_ASSERT(thes::test::string_eq(R"({"a":0.0,"b":[2.0,3],"c":1})", jay::to_json(test).dump()));
    fmt::print("\n");
  }

  {
    constexpr Test1 test1{0.0, {2.0, 3}, 1};
    constexpr TestTwo test2{5.0, test1};
    THES_ASSERT(thes::test::string_eq(R"({"a":5.0,"b":{"a":0.0,"b":[2.0,3],"c":1},"c":0})",
                                      jay::to_json(test2).dump()));
    fmt::print("\n");
  }

  {
    using Var = std::variant<Test1, TestTwo>;

    constexpr Test1 test1{0.0, {2.0, 3}, 1};
    constexpr TestTwo test2{5.0, test1};
    constexpr Var var1{test1};
    constexpr Var var2{test2};

    {
      auto json = jay::to_json(var1);
      THES_ASSERT(thes::test::string_eq(R"({"Test1":{"a":0.0,"b":[2.0,3],"c":1}})", json.dump()));

      auto json_out = Json::parse(json.dump());
      auto test_out = jay::from_json<Var>(json_out);
      THES_ASSERT(thes::test::string_eq(R"({"Test1":{"a":0.0,"b":[2.0,3],"c":1}})",
                                        jay::to_json(test_out).dump()));
      fmt::print("\n");
    }

    {
      auto json = jay::to_json(var2);
      THES_ASSERT(thes::test::string_eq(
        R"({"test_two":{"a":5.0,"b":{"a":0.0,"b":[2.0,3],"c":1},"c":0}})", json.dump()));

      auto json_out = Json::parse(json.dump());
      auto test_out = jay::from_json<Var>(json_out);
      THES_ASSERT(
        thes::test::string_eq(R"({"test_two":{"a":5.0,"b":{"a":0.0,"b":[2.0,3],"c":1},"c":0}})",
                              jay::to_json(test_out).dump()));
    }
  }
  fmt::print("\n");

  {
    using Var = std::variant<Test1, TestTwo, Test3>;
    constexpr Var var{Test3{}};

    auto json = jay::to_json(var);
    THES_ASSERT(thes::test::string_eq(
      R"({"test_3":{"a":0.0,"b":{"Test1":{"a":0.0,"b":[2.0,3],"c":3}}}})", json.dump()));

    auto json_out = Json::parse(json.dump());
    auto test_out = jay::from_json<Var>(json_out);
    THES_ASSERT(
      thes::test::string_eq(R"({"test_3":{"a":0.0,"b":{"Test1":{"a":0.0,"b":[2.0,3],"c":3}}}})",
                            jay::to_json(test_out).dump()));
  }
  fmt::print("\n");

  {
    const Test5 value0a{3};
    THES_ASSERT(thes::test::string_eq(R"({"a":3,"type":"f32","value":"forward"})",
                                      jay::to_json(value0a).dump()));

    Json json0{};
    const auto value0b = jay::from_json<Test5>(jay::to_json(value0a));
    if (value0a != value0b) {
      return 1;
    }

    using Type = std::variant<Templ5<Direction::FORWARD, float>, Templ5<Direction::BACKWARD, int>>;

    auto json1 = R"({"templ5":{"type":"f32","value":"forward","a":3}})"_json;
    const auto value1 = jay::from_json<Type>(json1);
    if (value1.index() != 0) {
      return 1;
    }

    auto json2 = R"({"templ5":{"type":"i32","value":"backward","a":5}})"_json;
    const auto value2 = jay::from_json<Type>(json2);
    if (value2.index() != 1) {
      return 1;
    }

    thes::LimitedArray<double, 4> arr{1.0, 5.0, 3.0};
    const auto value3 = jay::from_json<decltype(arr)>(jay::to_json(arr));
    if (value3 != arr) {
      return 1;
    }
  }
  fmt::print("\n");

  {
    using Type = std::variant<thes::i32, thes::f32>;
    auto json = R"({"i64":1})"_json;
    THES_ASSERT(thes::test::string_eq(R"({"i64":1})", json.dump()));
    try {
      jay::from_json<Type>(json);
      return 1;
    } catch (const std::invalid_argument& ex) {
      THES_ASSERT(
        thes::test::string_eq(ex.what(), "This is not a known variant! Keys: [i32, f32]"));
    }
  }
  fmt::print("\n");

  {
    using Type = std::variant<thes::i32, thes::f32>;
    auto json = R"({"i32":"a"})"_json;
    THES_ASSERT(thes::test::string_eq(R"({"i32":"a"})", json.dump()));
    try {
      jay::from_json<Type>(json);
      return 1;
    } catch (const std::invalid_argument& ex) {
      THES_ASSERT(thes::test::string_eq(
        ex.what(), "This is not a known variant! Keys: [i32, f32]\n"
                   "Errors that occurred when checking variants with the same name: "
                   "[\"[json.exception.type_error.302] type must be number, but is string\"]"));
    }
  }
}
