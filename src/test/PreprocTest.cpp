#include <catch2/catch_test_macros.hpp>

#include "../main/Preproc.h"
#include "Globals.h"
#include "IditorUtil.h"

#include <string>

TEST_CASE("preproc2", "[preproc]")
{
//  Globals::includeDirectories.emplace(
//      "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/include/c++/v1");
//
//  auto file = IditorUtil::findIncludeFileInIncludeDirs("vector");
//  auto res = Preproc::get()->getPreprocessedFromFile(file[0]);
//  printf("");
}

TEST_CASE("preproc", "[preproc]")
{
  std::string code = "//#define _FOO\n"
                     "#ifdef _FOO\n"
                     "void bar(){}\n"
                     "#endif\n"
                     "void fizz(){}";
  auto res = Preproc::get()->getPreprocessed(code, "");
  REQUIRE(res == "void bar(){}\n");
}