#include "Db.h"

#include <sqlite3.h>
#include <sqlite3ext.h>

#include <set>

void tokenizer_xDelete(Fts5Tokenizer *tokenizer)
{

}

int tokenizer_xTokenize(Fts5Tokenizer *,
                        void *pCtx,
                        int flags,            /* Mask of FTS5_TOKENIZE_* flags */
                        const char *pText, int nText,
                        int (*xToken)(
                            void *pCtx,         /* Copy of 2nd argument to xTokenize() */
                            int tflags,         /* Mask of FTS5_TOKEN_* flags */
                            const char *pToken, /* Pointer to buffer containing token */
                            int nToken,         /* Size of token in bytes */
                            int iStart,         /* Byte offset of token within input text */
                            int iEnd            /* Byte offset of end of token within input text */
                        )
)
{
  if (flags == FTS5_TOKENIZE_QUERY)
  {
    xToken(pCtx, 0, pText, nText, 0, nText);
    return SQLITE_OK;
  }

  for (int i = 0; i < nText; i++)
  {
    char c = tolower(pText[i]);
    xToken(pCtx, 0, &c, 1, 0, 1);
  }
  return SQLITE_OK;
}

int tokenizer_xCreate(void *, const char **azArg, int nArg, Fts5Tokenizer **ppOut)
{
  auto* t = new fts5_tokenizer();
  t->xTokenize = tokenizer_xTokenize;
  *ppOut = (Fts5Tokenizer*)t;
  return 0;
}

fts5_api *fts5_api_from_db(sqlite3 *db){
  fts5_api *pRet = 0;
  sqlite3_stmt *pStmt = 0;

  if( SQLITE_OK==sqlite3_prepare(db, "SELECT fts5(?1)", -1, &pStmt, 0) ){
    sqlite3_bind_pointer(pStmt, 1, (void*)&pRet, "fts5_api_ptr", nullptr);
    sqlite3_step(pStmt);
  }
  sqlite3_finalize(pStmt);
  return pRet;
}

Db::Db()
{
  sqlite3_open(":memory:", &db);

  fts5_api* api = fts5_api_from_db(db);
  fts5_tokenizer tokenizer;
  tokenizer.xCreate = tokenizer_xCreate;
  tokenizer.xDelete = tokenizer_xDelete;
  tokenizer.xTokenize = tokenizer_xTokenize;
  api->xCreateTokenizer(api, "iditor", nullptr, &tokenizer, nullptr);

//  auto sql = std::string("CREATE VIRTUAL TABLE declaration USING FTS5(name, file_name, tokenize='iditor');");
  auto sql = std::string("CREATE VIRTUAL TABLE declaration USING FTS5(name, file_name, tokenize='iditor');");
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
//  auto sql = std::string("SELECT * FROM declaration('")
//      .append(st).append("');");
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
