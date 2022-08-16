#include <catch2/catch_test_macros.hpp>

#include "../main/Preproc.h"
#include "Globals.h"
#include "IditorUtil.h"

#include <string>

TEST_CASE("ifdefined", "[preprocessor]")
{
  Globals::definitions.clear();
  Globals::definitions["__cplusplus"] = "201703L";

  Globals::macros.clear();
  std::string code = "#define _MSC_VER\n"
                     "#if defined(_MSC_VER) && !defined(__clang__)\n"
  "#  if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)\n"
  "#    define _LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER\n"
  "#  endif\n"
  "#endif\n";
  auto res = Preproc::get()->getPreprocessed(code, "");
  REQUIRE(res.empty());
  REQUIRE(Globals::definitions.find("_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER") != Globals::definitions.end());
}

TEST_CASE("preprocvector", "[preprocessor]")
{
  Globals::definitions.clear();
  Globals::includeDirectories.clear();
  Globals::macros.clear();

  Globals::definitions["__cplusplus"] = "201703L";

  Globals::includeDirectories.emplace(
      "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/include/c++/v1");

//  auto file = IditorUtil::findIncludeFileInIncludeDirs("vector");
  std::string file = "/Users/izmar/git/iditor/inctest/vector.cpp";
  auto res = Preproc::get()->getPreprocessedFromFile(file);
  printf("");
}

TEST_CASE("ifdef", "[preprocessor]")
{
  Globals::definitions.clear();
  std::string code = "#define _FOO\n"
                     "#ifdef _FOO\n"
                     "void bar(){}\n"
                     "#endif\n"
                     "void fizz(){}";
  auto res = Preproc::get()->getPreprocessed(code, "");
  REQUIRE(res == "void bar(){}\nvoid fizz(){}");
}

TEST_CASE("ifndef", "[preprocessor]")
{
  Globals::definitions.clear();
  std::string code = "#ifndef _FOO\n"
                     "void bar(){}\n"
                     "#endif\n"
                     "void fizz(){}";
  auto res = Preproc::get()->getPreprocessed(code, "");
  REQUIRE(res == "void bar(){}\nvoid fizz(){}");
}