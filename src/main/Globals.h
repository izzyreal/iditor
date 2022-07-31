#pragma once

#include <set>
#include <map>
#include <string>

class Globals {
public:
  static std::set<std::string> includeDirectories;
  static std::map<std::string, std::string> definitions;
};