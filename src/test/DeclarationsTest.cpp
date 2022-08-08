#include <catch2/catch_test_macros.hpp>

#include "../main/Declarations.h"

bool contains(const std::vector<Declaration> &declarations,
              const std::string &name,
              const std::string &name_space)
{
  return std::any_of(declarations.begin(), declarations.end(),
              [&](Declaration declaration) {
                return (declaration.name == name && declaration.name_space == name_space);
              });
}

TEST_CASE("Declarations", "[declarations]")
{
  const std::string no_namespace;

  std::string code = "class Foo{};";
  auto res = Declarations::get(code);
  REQUIRE(contains(res, "Foo", no_namespace));

  code = "namespace foo { class Bar{}; }";
  res = Declarations::get(code);
  REQUIRE(contains(res, "Bar", "foo::"));

  code = "namespace foo { namespace bar { class Fizz{}; } }";
  res = Declarations::get(code);
  REQUIRE(contains(res, "Fizz", "foo::bar::"));

  code = "namespace foo::bar { class Fizz{}; }";
  res = Declarations::get(code);
  REQUIRE(contains(res, "Fizz", "foo::bar::"));

  code = "void foo(){}";
  res = Declarations::get(code);
  REQUIRE(contains(res, "foo", no_namespace));

  code = "namespace foo { void bar(){} }";
  res = Declarations::get(code);
  REQUIRE(contains(res, "bar", "foo::"));
}
