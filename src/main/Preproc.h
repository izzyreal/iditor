#pragma once
#include <tree_sitter/api.h>

#include <string>
#include <filesystem>

class Preproc {
public:
  std::string getPreprocessed(std::string unprocessed, std::string source_dir);
  std::string getPreprocessedFromFile(const std::string& filePath);
  static Preproc* get();

private:
  Preproc();
  TSParser *parser = ts_parser_new();
  static Preproc* _instance;

};
