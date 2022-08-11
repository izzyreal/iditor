#include "Declarations.h"

#include "IditorUtil.h"

#include <tree_sitter/api.h>

#include <fstream>

extern "C" {
TSLanguage *tree_sitter_cpp();
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "misc-no-recursion"
std::vector<Declaration> Declarations::get(const std::string &code, const std::filesystem::path& file_path)
{
  std::vector<Declaration> result;

  auto parser = ts_parser_new();
  ts_parser_set_language(parser, tree_sitter_cpp());
  auto tree = ts_parser_parse_string(parser, nullptr, code.c_str(), code.length());
  auto root = ts_tree_root_node(tree);

  printf("%s\n", ts_node_string(root));

  auto query = "(type_definition"
               "  type: (type_identifier) @type_definition)\n"
               "\n"
               "(function_declarator"
               "  declarator: (identifier) @function)\n"
               "(preproc_include) @include";

  TSQueryError e;
  uint32_t e_offset;
  auto q = ts_query_new(tree_sitter_cpp(), query, strlen(query), &e_offset, &e);
  auto cursor = ts_query_cursor_new();
  ts_query_cursor_exec(cursor, q, root);
  TSQueryMatch m;

  while (ts_query_cursor_next_match(cursor, &m))
  {
    for (int i = 0; i < m.capture_count; i++)
    {
      std::string name_space;
      auto n = m.captures[i].node;
      auto st = ts_node_start_byte(n);
      auto end = ts_node_end_byte(n);
      auto name = code.substr(st, end - st);
      auto t = ts_node_type(n);

      auto process_preproc_include = [&]() {
        auto c = ts_node_child(n, 1);
        auto ct = ts_node_type(c);
        auto c_st = ts_node_start_byte(c);
        auto end_st = ts_node_end_byte(c);
        std::string include_file_name = code.substr(c_st, end_st - c_st);

        IditorUtil::cleanIncludeFilename(include_file_name);

        if (!include_file_name.empty())
        {
          auto full_path = file_path.parent_path() / include_file_name;

          for (auto& d : getFromFile(full_path))
          {
            result.emplace_back(d);
          }
        }
      };

      auto process_type_definition = [&]() {

      };

      if (strcmp(t, "type_definition") == 0)
      {
        process_type_definition();
        continue;
      }
      else if (strcmp(t, "preproc_include") == 0)
      {
        process_preproc_include();
        continue;
      }

      auto p = ts_node_parent(n);

      std::string type_ = ts_node_type(p);

      while (!ts_node_is_null(p))
      {
        auto pt = ts_node_type(p);

        if (pt == "namespace_definition")
        {
          auto id = ts_node_child(p, 1);
          auto ns_st = ts_node_start_byte(id);
          auto ns_end = ts_node_end_byte(id);
          auto ns_name = code.substr(ns_st, ns_end - ns_st).append("::");
          name_space = ns_name.append(name_space);
        }

        p = ts_node_parent(p);
      }

      if (type_ != "translation_unit")
      {
        result.emplace_back(Declaration{type_, name_space, name, "", "", file_path});
      }
    }
  }

  return result;
}
#pragma clang diagnostic pop

std::vector<Declaration> Declarations::getFromFile(const std::filesystem::path &path)
{
  std::ifstream file(path, std::ios::in | std::ios::binary);

  if (!file.is_open())
    return {};

  std::string content{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
  return get(content, path);
}

std::vector<Declaration> Declarations::getFromProject(Project &p)
{
  std::vector<Declaration> result;
  for (auto& f : p.getHeaderFiles())
  {
    for (auto& d : getFromFile(f))
    {
      result.emplace_back(d);
    }
  }

  return result;
}
