#pragma once

#include <sqlite3.h>

#include <string>
#include <vector>

class Db {
private:
  Db();
  ~Db();

  static Db* _instance;
  sqlite3 *db{};

public:
  static Db* instance();

  void insert_declaration(const std::string& name, const std::string& file_path);
  std::vector<std::string> get_declarations_starting_with(const std::string& st);
  std::vector<std::string> get_declarations_containing(const std::string&);
  void print_declarations();

};