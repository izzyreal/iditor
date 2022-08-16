#pragma once

#include <string>
#include <fstream>
#include <vector>
#include <tree_sitter/api.h>

#include "Globals.h"

class IditorUtil {
public:
  static std::vector<std::filesystem::path> getFilesFromDir(const std::string &dirPath)
  {
    std::vector<std::filesystem::path> result;
    for (const auto &entry: std::filesystem::directory_iterator(dirPath)) {
      if (entry.is_directory())
        continue;

      result.push_back(entry.path());
    }
    return result;
  }

  static std::string readFile(const std::filesystem::path &path)
  {
    return readFile(path.string());
  }

  static std::string readFile(const std::string &fileName)
  {
    std::ifstream ifs(fileName.c_str(), std::ios::in | std::ios::binary | std::ios::ate);

    std::ifstream::pos_type fileSize = ifs.tellg();
    ifs.seekg(0, std::ios::beg);

    std::vector<char> bytes(fileSize);
    ifs.read(bytes.data(), fileSize);

    auto result = std::string(bytes.data(), fileSize);
    return result;
  }

  static void traverse(TSNode n, std::function<bool(TSNode&)> f)
  {
    bool continueTraversal = f(n);

    if (!continueTraversal)
    {
      return;
    }

    for (int i = 0; i < ts_node_child_count(n); i++)
    {
      traverse(ts_node_child(n, i), f);
    }
  }

  static void collectLeafNodes(TSNode node, std::vector<TSNode> &leafNodes)
  {
    if (ts_node_child_count(node) == 0) {
      leafNodes.emplace_back(node);
      return;
    }

    for (int i = 0; i < ts_node_child_count(node); i++) {
      collectLeafNodes(ts_node_child(node, i), leafNodes);
    }
  }

  static void removeNodeFromText(TSNode n, std::string& text, int& added)
  {
    auto st = ts_node_start_byte(n) + added;
    auto end = ts_node_end_byte(n) + added;
    text = text.substr(0, st) + text.substr(end);
    added -= end - st;
  }

  static std::string getNodeText(TSNode node, const char* text)
  {
    auto st = ts_node_start_byte(node);
    auto end = ts_node_end_byte(node);
    return std::string(text).substr(st, end - st);
  }

  static std::vector<std::string> getFirstIdentifier(TSNode n, std::string &text)
  {
    auto sibling = ts_node_next_sibling(n);
    auto nodeText = getNodeText(n, text.c_str());

    while (!ts_node_is_null(sibling)) {
      auto sibling_type = ts_node_type(sibling);

      if (strcmp(sibling_type, "type_identifier") == 0 ||
          strcmp(sibling_type, "identifier") == 0 ||
          strcmp(sibling_type, "string_literal") == 0 ||
          strcmp(sibling_type, "system_lib_string") == 0) {
        auto candidate = getNodeText(sibling, text.c_str());
        return {candidate};
      }

      sibling = ts_node_next_sibling(sibling);
    }

    return {};
  }

  static std::vector<std::filesystem::path> findFileInDir(const std::string& filename, std::filesystem::path dir)
  {
    for (const auto &entry: std::filesystem::directory_iterator(dir)) {
      if (entry.is_directory())
        continue;

      if (entry.path().filename() == filename)
      {
        return {entry.path()};
      }
    }
    return {};
  }

  static std::vector<std::filesystem::path> findIncludeFile(const std::string& relativePath, std::filesystem::path dir)
  {
    if (!std::filesystem::exists(dir))
    {
      return {};
    }

    auto candidate = dir;
    candidate.append(relativePath);

    for (const auto &entry: std::filesystem::directory_iterator(dir)) {
      if (entry.is_directory())
        continue;

      if (entry.path() == candidate)
      {
        return {entry.path()};
      }
    }
    return {};
  }

  static std::vector<std::filesystem::path> findIncludeFileInIncludeDirs(const std::string& relPath)
  {
//    printf("Looking for relPath %s\n", relPath.c_str());

    if (relPath == "endian.h") return {};

    for (auto& dir : Globals::includeDirectories)
    {
      auto candidate = findIncludeFile(relPath, dir);
      if (!candidate.empty())
        return candidate;
    }
    return {};
  }

  static void cleanIncludeFilename(std::string &includeFilename)
  {
    if (!includeFilename.empty())
    {
      if (includeFilename[0] == '<' || includeFilename[0] == '"')
      {
        includeFilename = includeFilename.substr(1);
      }
      if (!includeFilename.empty())
      {
        if (includeFilename[includeFilename.length() - 1] == '>' || includeFilename[includeFilename.length() - 1] == '"')
        {
          includeFilename = includeFilename.substr(0, includeFilename.length() - 1);
        }
      }
    }
  }
};
