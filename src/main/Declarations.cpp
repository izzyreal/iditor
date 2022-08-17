#include "Declarations.h"

#include "IditorUtil.h"
#include "Preproc.h"

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
  std::set<std::string> alreadyIncluded;
  auto preprocessed = Preproc::get()->getPreprocessed(code, file_path.parent_path(), alreadyIncluded);
  auto tree = ts_parser_parse_string(parser, nullptr, preprocessed.c_str(), preprocessed.length());
  auto root = ts_tree_root_node(tree);

//  printf("%s\n", ts_node_string(root));

  auto query = "(type_definition) @type_definition\n"
               "(class_specifier) @class_specifier\n"
               "(function_declarator) @function_declarator\n"
               "(preproc_include) @preproc_include";

  TSQueryError e;
  uint32_t e_offset;
  auto q = ts_query_new(tree_sitter_cpp(), query, strlen(query), &e_offset, &e);
  auto cursor = ts_query_cursor_new();
  ts_query_cursor_exec(cursor, q, root);
  TSQueryMatch m;

  auto defs = Globals::definitions;

  auto is_included_after_preprocessing = [&](TSNode& n) -> bool {
    auto p = ts_node_parent(n);
    bool is_in_def = false;
    bool def_is_satisfied = true;
    while (!ts_node_is_null(p))
    {
      auto pt = ts_node_type(p);
      if (strcmp(pt, "translation_unit") == 0)
      {
        break;
      }
      else if (strcmp(pt, "preproc_if") == 0)
      {
        is_in_def = true;
        auto condition_node = ts_node_child(n, 1);
        auto condition_is_met = evaluate_condition(condition_node, preprocessed, defs);
        if (!condition_is_met)
        {
          def_is_satisfied = false;
          break;
        }
      }
      p = ts_node_parent(p);
    }
    return !is_in_def || def_is_satisfied;
  };

  while (ts_query_cursor_next_match(cursor, &m))
  {
    for (int i = 0; i < m.capture_count; i++)
    {
      std::string name_space;
      auto n = m.captures[i].node;

      if (!is_included_after_preprocessing(n))
      {
        continue;
      }

      auto st = ts_node_start_byte(n);
      auto end = ts_node_end_byte(n);
      auto name = preprocessed.substr(st, end - st);
      auto t = ts_node_type(n);

      auto process_type_definition = [&]() {
        auto typeTypeID = IditorUtil::getNodeText(ts_node_child(n, 1), preprocessed.c_str());
        auto declaratorTypeID = IditorUtil::getNodeText(ts_node_child(n, 2), preprocessed.c_str());
        auto ns = getNamespaceForNode(n, preprocessed);
        result.emplace_back(std::make_shared<TypeDefDecl>(ns, typeTypeID, declaratorTypeID));
      };

      auto process_class_specifier = [&]() {
        auto name = IditorUtil::getNodeText(ts_node_child(n, 1), preprocessed.c_str());
        auto ns = getNamespaceForNode(n, preprocessed);
        result.emplace_back(std::make_shared<ClassSpecDecl>(ns, name));
      };

      auto process_function_declarator = [&]() {
        auto name = IditorUtil::getNodeText(ts_node_child(n, 0), preprocessed.c_str());
        auto ns = getNamespaceForNode(n, preprocessed);
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

bool Declarations::evaluate_condition(TSNode& condition_node, const std::string& text, const std::map<std::string, std::string>& defs)
{
  auto condition_type = ts_node_type(condition_node);
  if (strcmp(condition_type, "binary_expression") == 0)
  {
    auto left_node = ts_node_child(condition_node, 0);
    auto left_condition_is_met = evaluate_condition(left_node, text, defs);
    auto binary_operator = ts_node_type(ts_node_child(condition_node, 1));

    if (strcmp(binary_operator, "||") == 0 && left_condition_is_met)
    {
      return true;
    }

    auto right_node = ts_node_child(condition_node, 2);
    auto right_condition_is_met = evaluate_condition(right_node, text, defs);

    if (strcmp(binary_operator, "||") == 0)
    {
      return right_condition_is_met;
    }

    return left_condition_is_met && right_condition_is_met;
  }
  else if (strcmp(condition_type, "unary_expression") == 0)
  {
    auto c2 = ts_node_child(condition_node, 1);
    return !evaluate_condition(c2, text, defs);
  }
  else if (strcmp(condition_type, "preproc_defined") == 0)
  {
    auto id_node = ts_node_child(condition_node, 2);
    auto id_text = IditorUtil::getNodeText(id_node, text.c_str());
    return defs.find(id_text) != defs.end();
  }
  return false;
}
