#pragma once

#include <set>
#include <map>
#include <string>
#include <vector>
#include <regex>

struct Macro {
  std::string name;
  std::vector<std::string> args;
  std::string body;
  std::string apply(const std::vector<std::string> params)
  {
    if (params.size() != args.size()) return body;
    auto result = body;
    for (int i = 0; i < args.size(); i++)
      result = std::regex_replace(result, std::regex(args[i]), params[i]);
    return result;
  }
};

class Globals {
public:
  static std::set<std::string> includeDirectories;
  static std::map<std::string, std::string> definitions;
  static std::map<std::string, Macro> macros;

  static  const char* highlight_query;
};