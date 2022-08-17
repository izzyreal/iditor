#pragma once
#include <tree_sitter/api.h>

#include <string>
#include <set>
#include <filesystem>

class Preproc {
public:
  std::string getPreprocessed(const std::string& unprocessed, std::string source_dir, std::set<std::string>& alreadyIncluded);
  std::string getPreprocessedFromFile(const std::string& filePath, std::set<std::string>& alreadyIncluded);
  static Preproc* get();

private:
  Preproc();
  TSParser *parser = ts_parser_new();
  static Preproc* _instance;

};
