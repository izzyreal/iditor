#pragma once

#include <set>
#include <string>

class Globals {
public:
  static std::set<std::string> includeDirectories;
  static std::set<std::string> definitions;
};