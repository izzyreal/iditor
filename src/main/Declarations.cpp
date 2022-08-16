#include "Declarations.h"

#include "IditorUtil.h"

#include <tree_sitter/api.h>

#include <fstream>

extern "C" {
TSLanguage *tree_sitter_cpp();
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "misc-no-recursion"
std::vector<std::shared_ptr<Decl>> Declarations::get(const std::string &code, const std::filesystem::path& file_path)
{
  std::vector<std::shared_ptr<Decl>> result;

  auto parser = ts_parser_new();
  ts_parser_set_language(parser, tree_sitter_cpp());
  auto tree = ts_parser_parse_string(parser, nullptr, code.c_str(), code.length());
  auto root = ts_tree_root_node(tree);

//  printf("%s\n", ts_node_string(root));

  auto query = "(type_definition) @type_definition\n"
               "(class_specifier) @class_specifier\n"
               "(function_declarator) @function_declarator\n"
               "(preproc_include) @preproc_include\n"
               "(preproc_ifdef) @preproc_ifdef";

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
        auto c_st = ts_node_start_byte(c);
        auto end_st = ts_node_end_byte(c);
        std::string include_file_name = code.substr(c_st, end_st - c_st);

        IditorUtil::cleanIncludeFilename(include_file_name);

        if (!include_file_name.empty())
        {
          auto includeFile = IditorUtil::findIncludeFileInIncludeDirs(include_file_name);

          if (!includeFile.empty())
          {
            for (auto &d: getFromFile(includeFile[0]))
            {
              result.emplace_back(d);
            }
          }
        }
      };

      auto process_type_definition = [&]() {
        auto typeTypeID = IditorUtil::getNodeText(ts_node_child(n, 1), code.c_str());
        auto declaratorTypeID = IditorUtil::getNodeText(ts_node_child(n, 2), code.c_str());
        auto ns = getNamespaceForNode(n, code);
        result.emplace_back(std::make_shared<TypeDefDecl>(ns, typeTypeID, declaratorTypeID));
      };

      auto process_class_specifier = [&]() {
        auto name = IditorUtil::getNodeText(ts_node_child(n, 1), code.c_str());
        auto ns = getNamespaceForNode(n, code);
        result.emplace_back(std::make_shared<ClassSpecDecl>(ns, name));
      };

      auto process_function_declarator = [&]() {
        auto name = IditorUtil::getNodeText(ts_node_child(n, 0), code.c_str());
        auto ns = getNamespaceForNode(n, code);
        result.emplace_back(std::make_shared<FuncDeclaratorDecl>(ns, name));
      };

      if (strcmp(t, "type_definition") == 0)
      {
        process_type_definition();
      }
      else if (strcmp(t, "class_specifier") == 0)
      {
        process_class_specifier();
      }
      else if (strcmp(t, "function_declarator") == 0)
      {
        process_function_declarator();
      }
      else if (strcmp(t, "preproc_include") == 0)
      {
        process_preproc_include();
      }
    }
  }

  return result;
}

std::vector<std::shared_ptr<Decl>> Declarations::getFromFile(const std::filesystem::path &path)
{
  std::ifstream file(path, std::ios::in | std::ios::binary);

  if (!file.is_open())
    return {};

  std::string content{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
  return get(content, path);
}

std::vector<std::shared_ptr<Decl>> Declarations::getFromProject(Project &p)
{
  std::vector<std::shared_ptr<Decl>> result;

  for (auto& f : p.getHeaderFiles())
  {
    for (auto& d : getFromFile(f))
    {
      result.emplace_back(d);
    }
  }

  return result;
}

std::string Declarations::getNamespaceForNode(TSNode& n, const std::string& code)
{
  std::string result;

  auto p = ts_node_parent(n);

  std::string type_ = ts_node_type(p);

  while (!ts_node_is_null(p))
  {
    if (strcmp(ts_node_type(p), "namespace_definition") == 0)
    {
      auto id = ts_node_child(p, 1);
      auto ns_st = ts_node_start_byte(id);
      auto ns_end = ts_node_end_byte(id);
      auto ns_name = code.substr(ns_st, ns_end - ns_st).append("::");
      result = ns_name.append(result);
    }

    p = ts_node_parent(p);
  }

  return result;
}

bool Declarations::contains(const std::vector<std::shared_ptr<Decl>> &declarations,
              const std::string& name,
              const std::string& name_space,
              DeclType declaration_type)
{
  return std::any_of(declarations.begin(), declarations.end(),
                     [&](const std::shared_ptr<Decl>& declaration) {
                       return (
                           declaration->getName() == name &&
                           declaration->getNamespace() == name_space &&
                           declaration->getType() == declaration_type);
                     });
}