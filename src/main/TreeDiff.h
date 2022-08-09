#pragma once

#include <tree_sitter/api.h>

#include "Project.h"

#include <vector>
#include <string>

class TreeDiff
{
public:
  static std::vector<std::string> getNewIncludes(TSTree* t1, TSTree* t2, const char* text, Project& p);
};
