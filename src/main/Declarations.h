#pragma once

#include <vector>
#include <string>
#include <utility>

struct Declaration {
  const std::string declaration_type;
  const std::string name_space;
  const std::string name;
  const std::string parameters;
  const std::string return_type;
};

class Declarations
{
public:
  static std::vector<Declaration> get(const std::string& code);
};
