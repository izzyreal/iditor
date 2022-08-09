#pragma once

#include <vector>
#include <string>
#include <utility>
#include <filesystem>
#include "Project.h"

struct Declaration {
  const std::string declaration_type;
  const std::string name_space;
  const std::string name;
  const std::string parameters;
  const std::string return_type;
  const std::filesystem::path file_path;
};

class Declarations
{
public:
  // Leave file_path empty if you want to parse some code that doesn't come from a file
  static std::vector<Declaration> get(const std::string& code, const std::filesystem::path& file_path = "");
  static std::vector<Declaration> getFromFile(const std::filesystem::path& filePath);
  static std::vector<Declaration> getFromProject(Project& p);
};
