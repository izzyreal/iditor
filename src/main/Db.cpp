#include "Db.h"

#include <set>

Db::Db()
{
  sqlite3_open(":memory:", &db);
  auto sql = std::string("CREATE VIRTUAL TABLE declaration USING FTS5(name, file_name);");
  char *err_msg = 0;
  sqlite3_exec(db, sql.c_str(), 0, 0, &err_msg);
}

Db::~Db()
{
  sqlite3_close(db);
}

Db *Db::_instance = nullptr;
Db *Db::instance()
{
  if (_instance == nullptr) {
    Db::_instance = new Db();
  }
  return Db::_instance;
}

void Db::insert_declaration(const std::string &name, const std::string &file_path)
{
  auto existing = get_declarations_starting_with(name);

  if (find(existing.begin(), existing.end(), name) != existing.end()) return;

  auto sql = std::string("INSERT INTO declaration VALUES('")
      .append(name).append("', '").append(file_path).append("') ;'");

  char *err_msg = 0;
  sqlite3_exec(db, sql.c_str(), 0, 0, &err_msg);
}

std::vector<std::string> Db::get_declarations_starting_with(const std::string &st)
{
  sqlite3_stmt *res;
  auto sql = std::string("SELECT * FROM declaration WHERE name MATCH '")
      .append(st).append("*';");
  sqlite3_prepare_v2(db, sql.c_str(), -1, &res, 0);
  auto rc = sqlite3_step(res);

  std::vector<std::string> result;
  while (rc == SQLITE_ROW) {
    auto declaration_name = sqlite3_column_text(res, 0);
    auto str = std::string(reinterpret_cast<const char *>(declaration_name));
    result.emplace_back(str);
    rc = sqlite3_step(res);
  }

  sqlite3_finalize(res);
  return result;
}

std::vector<std::string> Db::get_declarations_containing(const std::string &st)
{
  sqlite3_stmt *res;
  auto sql = std::string("SELECT * FROM declaration WHERE name GLOB '*")
      .append(st).append("*';");
  sqlite3_prepare_v2(db, sql.c_str(), -1, &res, 0);
  auto rc = sqlite3_step(res);

  std::vector<std::string> result;
  while (rc == SQLITE_ROW) {
    auto declaration_name = sqlite3_column_text(res, 0);
    auto str = std::string(reinterpret_cast<const char *>(declaration_name));
    result.emplace_back(str);
    rc = sqlite3_step(res);
  }

  sqlite3_finalize(res);
  return result;
}

void Db::print_declarations()
{
  sqlite3_stmt *res;
  auto sql = std::string("SELECT * FROM declaration;");
  sqlite3_prepare_v2(db, sql.c_str(), -1, &res, 0);
  auto rc = sqlite3_step(res);

  std::vector<std::string> result;
  std::set<std::string> unique_results;

  while (rc == SQLITE_ROW) {
    auto declaration_name = sqlite3_column_text(res, 0);
    auto str = std::string(reinterpret_cast<const char *>(declaration_name));
    result.emplace_back(str);
    unique_results.emplace(result.back());
    rc = sqlite3_step(res);
  }

  printf("Found %lu duplicates\n", (result.size() - unique_results.size()));

  sqlite3_finalize(res);
}
