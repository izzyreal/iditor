#pragma once

#include <string>
#include <filesystem>
#include <vector>

class Project
{
public:
  std::vector<std::filesystem::path> getHeaderFiles();
  void setRootPath(const std::filesystem::path &_root_path);
  std::filesystem::path& getRootPath();

private:
  std::filesystem::path root_path;

};
