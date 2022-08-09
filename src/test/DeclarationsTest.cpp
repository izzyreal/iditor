#include <catch2/catch_test_macros.hpp>

#include "../main/Declarations.h"

#include <filesystem>
#include <fstream>

bool contains(const std::vector<Declaration> &declarations,
              const std::string& name,
              const std::string& name_space,
              const std::string& declaration_type)
{
  return std::any_of(declarations.begin(), declarations.end(),
              [&](const Declaration& declaration) {
                return (
                    declaration.name == name &&
                    declaration.name_space == name_space &&
                    declaration.declaration_type == declaration_type);
              });
}

TEST_CASE("Declarations", "[declarations]")
{
  const std::string no_namespace;
  const std::string CLASS_SPECIFIER = "class_specifier";
  const std::string FUNCTION_DECLARATOR = "function_declarator";

  std::string code = "class Foo{};";
  auto res = Declarations::get(code);
  REQUIRE(contains(res, "Foo", no_namespace, CLASS_SPECIFIER));

  code = "namespace foo { class Bar{}; }";
  res = Declarations::get(code);
  REQUIRE(contains(res, "Bar", "foo::", CLASS_SPECIFIER));

  code = "namespace foo { namespace bar { class Fizz{}; } }";
  res = Declarations::get(code);
  REQUIRE(contains(res, "Fizz", "foo::bar::", CLASS_SPECIFIER));

  code = "namespace foo::bar { class Fizz{}; }";
  res = Declarations::get(code);
  REQUIRE(contains(res, "Fizz", "foo::bar::", CLASS_SPECIFIER));

  code = "void foo(){}";
  res = Declarations::get(code);
  REQUIRE(contains(res, "foo", no_namespace, FUNCTION_DECLARATOR));

  code = "namespace foo { void bar(){} }";
  res = Declarations::get(code);
  REQUIRE(contains(res, "bar", "foo::", FUNCTION_DECLARATOR));
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
  REQUIRE(contains(res, "Foo", "", "class_specifier"));
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
  REQUIRE(contains(res, "Foo", "", "class_specifier"));
}
