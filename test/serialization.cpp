#include <iostream>
#include <sstream>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

#include "nlohmann/json.hpp"
#include "thesauros/macropolis.hpp"
#include "thesauros/test.hpp"

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

    thes::test::string_eq(R"({"a":0.0,"b":[2.0,3],"c":1})", jay::to_json(test));
    std::cout << std::endl;
  }

  {
    constexpr Test1 test1{0.0, {2.0, 3}, 1};
    constexpr TestTwo test2{5.0, test1};
    thes::test::string_eq(R"({"a":5.0,"b":{"a":0.0,"b":[2.0,3],"c":1},"c":0})",
                          jay::to_json(test2));
    std::cout << std::endl;
  }

  {
    using Var = std::variant<Test1, TestTwo>;

    constexpr Test1 test1{0.0, {2.0, 3}, 1};
    constexpr TestTwo test2{5.0, test1};
    constexpr Var var1{test1};
    constexpr Var var2{test2};

    {
      auto json = jay::to_json(var1);
      thes::test::string_eq(R"({"Test1":{"a":0.0,"b":[2.0,3],"c":1}})", json);

      auto json_out = Json::parse((std::stringstream{} << json).str());
      auto test_out = jay::from_json<Var>(json_out);
      thes::test::string_eq(R"({"Test1":{"a":0.0,"b":[2.0,3],"c":1}})", jay::to_json(test_out));
      std::cout << std::endl;
    }

    {
      auto json = jay::to_json(var2);
      thes::test::string_eq(R"({"test_two":{"a":5.0,"b":{"a":0.0,"b":[2.0,3],"c":1},"c":0}})",
                            json);

      auto json_out = Json::parse((std::stringstream{} << json).str());
      auto test_out = jay::from_json<Var>(json_out);
      thes::test::string_eq(R"({"test_two":{"a":5.0,"b":{"a":0.0,"b":[2.0,3],"c":1},"c":0}})",
                            jay::to_json(test_out));
    }
  }
  std::cout << std::endl;

  {
    using Var = std::variant<Test1, TestTwo, Test3>;
    constexpr Var var{Test3{}};

    auto json = jay::to_json(var);
    thes::test::string_eq(R"({"test_3":{"a":0.0,"b":{"Test1":{"a":0.0,"b":[2.0,3],"c":3}}}})",
                          json);

    auto json_out = Json::parse((std::stringstream{} << json).str());
    auto test_out = jay::from_json<Var>(json_out);
    thes::test::string_eq(R"({"test_3":{"a":0.0,"b":{"Test1":{"a":0.0,"b":[2.0,3],"c":3}}}})",
                          jay::to_json(test_out));
  }
  std::cout << std::endl;

  {
    using namespace jay;

    const Test5 value0a{3};
    thes::test::string_eq(R"({"a":3,"type":"f32","value":"forward"})", to_json(value0a));

    Json json0{};
    const auto value0b = from_json<Test5>(to_json(value0a));
    if (value0a != value0b) {
      return 1;
    }

    using Type = std::variant<Templ5<Direction::FORWARD, float>, Templ5<Direction::BACKWARD, int>>;

    auto json1 = R"({"templ5":{"type":"f32","value":"forward","a":3}})"_json;
    const auto value1 = from_json<Type>(json1);
    if (value1.index() != 0) {
      return 1;
    }

    auto json2 = R"({"templ5":{"type":"i32","value":"backward","a":5}})"_json;
    const auto value2 = from_json<Type>(json2);
    if (value2.index() != 1) {
      return 1;
    }
  }
}