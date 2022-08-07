#pragma once

#include <vector>
#include <string>
#include <utility>

class Declarations
{
public:
  static std::vector<std::pair<std::string, std::string>> get(const std::string& code);
};
