#pragma once

#include <vector>
#include <string>
#include <map>
#include <utility>
#include <filesystem>
#include <tree_sitter/api.h>
#include "Project.h"
#include "decl/Decl.h"

#include <memory>

class Declarations
{
public:
  // Leave file_path empty if you want to parse some code that doesn't come from a file
  static std::vector<std::shared_ptr<Decl>> get(const std::string& code, const std::filesystem::path& file_path = "");
  static std::vector<std::shared_ptr<Decl>> getFromFile(const std::filesystem::path& filePath);
  static std::vector<std::shared_ptr<Decl>> getFromProject(Project& p);

  static bool contains(const std::vector<std::shared_ptr<Decl>> &declarations,
                const std::string& name,
                const std::string& name_space,
                DeclType declaration_type);
private:
  static std::string getNamespaceForNode(TSNode& n, const std::string& code);
  static bool evaluate_condition(TSNode &condition_node, const std::string &text, const std::map<std::string, std::string>& defs);

};
