#include <catch2/catch_test_macros.hpp>

#include "../main/Declarations.h"

bool contains(const std::vector<std::pair<std::string, std::string>>& declarations, const std::string& v)
{
  for (auto& declaration : declarations)
  {
    if (declaration.first == v)
    {
      return true;
    }
  }
  return false;
}

TEST_CASE("Declarations", "[declarations]")
{
  std::string code = "class Foo{};";
  auto res = Declarations::get(code);
  REQUIRE(contains(res, "Foo"));

  code = "namespace foo { class Bar{}; }";
  res = Declarations::get(code);
  REQUIRE(contains(res, "foo::Bar"));

  code = "namespace foo { namespace bar { class Fizz{}; } }";
  res = Declarations::get(code);
  REQUIRE(contains(res, "foo::bar::Fizz"));

  code = "namespace foo::bar { class Fizz{}; }";
  res = Declarations::get(code);
  REQUIRE(contains(res, "foo::bar::Fizz"));
}