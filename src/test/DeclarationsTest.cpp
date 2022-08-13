#include <catch2/catch_test_macros.hpp>

#include "Declarations.h"

#include "decl/Decl.h"

#include <filesystem>
#include <fstream>

const std::string no_namespace;

TEST_CASE("Declarations", "[declarations]")
{
  std::string code = "class Foo{};";
  auto res = Declarations::get(code);
  REQUIRE(Declarations::contains(res, "Foo", no_namespace, CLASS_SPECIFIER));

  code = "namespace foo { class Bar{}; }";
  res = Declarations::get(code);
  REQUIRE(Declarations::contains(res, "Bar", "foo::", CLASS_SPECIFIER));

  code = "namespace foo { namespace bar { class Fizz{}; } }";
  res = Declarations::get(code);
  REQUIRE(Declarations::contains(res, "Fizz", "foo::bar::", CLASS_SPECIFIER));

  code = "namespace foo::bar { class Fizz{}; }";
  res = Declarations::get(code);
  REQUIRE(Declarations::contains(res, "Fizz", "foo::bar::", CLASS_SPECIFIER));

  code = "void foo(){}";
  res = Declarations::get(code);
  REQUIRE(Declarations::contains(res, "foo", no_namespace, FUNCTION_DECLARATOR));

  code = "namespace foo { void bar(){} }";
  res = Declarations::get(code);
  REQUIRE(Declarations::contains(res, "bar", "foo::", FUNCTION_DECLARATOR));
}

TEST_CASE("typedef", "[declarations]")
{
  auto code = "typedef __char16_t char16_t;";
  auto res = Declarations::get(code);
  REQUIRE(Declarations::contains(res, "__char16_t", no_namespace, TYPE_DEFINITION));
  REQUIRE(std::dynamic_pointer_cast<TypeDefDecl>(res[0])->getDeclaratorTypeID() == "char16_t");
}

Project createEmptyTestProject()
{
  std::filesystem::path path("test_project");
  Project p;
  p.setRootPath(path);

  std::filesystem::remove_all(path);
  std::filesystem::create_directories(path);
  return p;
}

TEST_CASE("Project declarations", "[declarations]")
{
  Project p = createEmptyTestProject();

  auto path = p.getRootPath();
  path /= "a.h";
  std::ofstream ofs(path);
  ofs << "class Foo{};\n";

  ofs.close();

  auto res = Declarations::getFromProject(p);
  REQUIRE(Declarations::contains(res, "Foo", no_namespace, CLASS_SPECIFIER));
}

TEST_CASE("Included declarations", "[declarations]")
{
  Project p = createEmptyTestProject();

  auto root_path = p.getRootPath();
  auto path1 = root_path / "a.h";
  auto path2 = root_path / "b.h";

  std::ofstream ofs(path1);
  ofs << "class Foo{};\n";
  ofs.close();

  ofs = std::ofstream(path2);
  ofs << "#include \"a.h\"\n";
  ofs.close();

  auto res = Declarations::getFromFile(path2);
  REQUIRE(Declarations::contains(res, "Foo", no_namespace, CLASS_SPECIFIER));
}
