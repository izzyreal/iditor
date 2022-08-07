#include "Declarations.h"

#include <tree_sitter/api.h>

extern "C" {
TSLanguage *tree_sitter_cpp();
}

std::vector<std::pair<std::string, std::string>> Declarations::get(const std::string &code)
{
  std::vector<std::pair<std::string, std::string>> result;

  auto parser = ts_parser_new();
  ts_parser_set_language(parser, tree_sitter_cpp());
  auto tree = ts_parser_parse_string(parser, nullptr, code.c_str(), code.length());
  auto root = ts_tree_root_node(tree);

  auto query = "(type_identifier) @type\n";

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
      auto n = m.captures[i].node;
      auto st = ts_node_start_byte(n);
      auto end = ts_node_end_byte(n);
      auto name = code.substr(st, end - st);
      auto p = ts_node_parent(n);

      std::string type_ = ts_node_type(p);

      p = ts_node_parent(p);

      while (!ts_node_is_null(p))
      {
        auto pt = ts_node_type(p);

        if (pt == "namespace_definition")
        {
          auto id = ts_node_child(p, 1);
          auto ns_st = ts_node_start_byte(id);
          auto ns_end = ts_node_end_byte(id);
          auto ns_name = code.substr(ns_st, ns_end - ns_st);
          name = ns_name.append("::").append(name);
        }

        p = ts_node_parent(p);
      }

      result.emplace_back(std::pair{name, type_});
    }
  }

  return result;
}