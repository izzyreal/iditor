#include "Project.h"

void Project::setRootPath(const std::filesystem::path& _root_path)
{
  this->root_path = _root_path;
}

std::vector<std::filesystem::path> Project::getHeaderFiles()
{
  std::vector<std::filesystem::path> result;

  for (auto& f : std::filesystem::directory_iterator(root_path))
  {
    if (f.is_regular_file() && f.path().extension() == ".h")
    {
      result.emplace_back(f.path());
    }
  }

  return result;
}
std::filesystem::path& Project::getRootPath()
{
  return root_path;
}
