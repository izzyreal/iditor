#include "Preproc.h"
#include "IditorUtil.h"

#include <sstream>

extern "C" {
TSLanguage *tree_sitter_cpp();
}

Preproc::Preproc()
{
  ts_parser_set_language(parser, tree_sitter_cpp());
}

std::string Preproc::getPreprocessed(std::string unprocessed, std::string source_dir)
{
  auto tree = ts_parser_parse_string(parser, nullptr, unprocessed.c_str(), strlen(unprocessed.c_str()));
  std::string result = unprocessed;
  int added = 0;

  auto proc_include = [&](TSNode n) {
    auto p = ts_node_parent(n);
    result = result.substr(0, ts_node_start_byte(p) + added);

    for (auto &i: IditorUtil::getFirstIdentifier(n, unprocessed)) {
      auto trimmed = i.length() >= 2 ? i.substr(1, i.length() - 2) : i;
      auto path = source_dir;
      path = path.append("/").append(trimmed);

      if (!exists(std::filesystem::path(path))) {
        auto candidate = IditorUtil::findIncludeFileInIncludeDirs(trimmed);
        if (!candidate.empty())
          path = candidate[0];
      }

      if (exists(std::filesystem::path(path))) {
        auto original_length = ts_node_end_byte(p) - ts_node_start_byte(p);
        auto toAdd = getPreprocessedFromFile(path);
        result += toAdd;
        added += toAdd.size() - original_length;
      }
    }
    result += unprocessed.substr(ts_node_end_byte(p));
  };

  auto proc_define = [&](TSNode n) {
    auto id_node = ts_node_child(n, 1);
    auto id = IditorUtil::getNodeText(id_node, unprocessed);

    if (ts_node_child_count(n) >= 4)
    {
      auto value_node = ts_node_child(n, 2);
      auto value = IditorUtil::getNodeText(value_node, unprocessed);
      Globals::definitions[id] = value;
      printf("Registered preprocessor definition '%s %s'\n", id.c_str(), value.c_str());
    } else {
      Globals::definitions[id] = "";
      printf("Registered preprocessor definition '%s'\n", id.c_str());
    }
    IditorUtil::removeNodeFromText(n, result, added );
  };

  auto proc_replace_macro_invocation = [&](TSNode n) {
    auto m = Globals::macros.find(IditorUtil::getNodeText(n, unprocessed));

    if (m != Globals::macros.end())
    {
      auto ctx = ts_node_parent(n);
      auto parent_t = ts_node_type(ctx);

      if (strcmp(parent_t, "ERROR") == 0) return;

      if (strcmp(parent_t, "preproc_function_def") == 0) return;

      printf("call expression text: %s\n", IditorUtil::getNodeText(ctx, unprocessed).c_str());
      printf("Found macro %s\n", (*m).first.c_str());

      auto parameters_node = ts_node_child(ctx, 1);
      std::vector<std::string> params;
      for (int i = 0; i < ts_node_child_count(parameters_node); i++)
      {
        auto nt = IditorUtil::getNodeText(ts_node_child(parameters_node, i), unprocessed);
        if (nt == "(" || nt == "," || nt == ")") continue;
        params.emplace_back(nt);
      }

      auto expanded = (*m).second.apply(params);
      result = result.substr(0, ts_node_start_byte(n) + added);
      auto original_length = ts_node_end_byte(n) - ts_node_start_byte(n);
      result += expanded;
      added += expanded.size() - original_length;
      result += unprocessed.substr(ts_node_end_byte(n));
    }
  };

  auto proc_replace_macro_definition = [&](TSNode n) {
    auto id = IditorUtil::getNodeText(ts_node_child(n, 1), unprocessed);
    auto body = IditorUtil::getNodeText(ts_node_child(n, 3), unprocessed);

    auto args_node = ts_node_child(n, 2);
    std::vector<std::string> args;

    for (int i = 0; i < ts_node_child_count(args_node); i++)
    {
      auto nt = IditorUtil::getNodeText(ts_node_child(args_node, i), unprocessed);
      if (nt == "(" || nt == "," || nt == ")") continue;
      args.emplace_back(nt);
    }

    Globals::macros[id] = Macro { id, args, body };

    std::string args_concatenated_again;
    for (int i = 0; i < args.size(); i++)
    {
      args_concatenated_again += args[i];
      if (i != args.size() - 1) args_concatenated_again += ",";
    }
    printf("Registered preprocessor function definition '%s %s'\n", id.c_str(), args_concatenated_again.c_str());
    IditorUtil::removeNodeFromText(n, result, added);
  };

  auto f = [&](TSNode n) {
    auto t = ts_node_type(n);
    auto text = IditorUtil::getNodeText(n, unprocessed);

    if (strcmp(t, "preproc_function_def") == 0)
    {
      proc_replace_macro_definition(n);
    }
    else if (strcmp(t, "preproc_call") == 0)
    {
//      printf("");
    }
    else if (strcmp(t, "#include") == 0)
    {
      proc_include(n);
    }
    else if (strcmp(t, "preproc_def") == 0)
    {
      proc_define(n);
    }
    else if (strcmp(t, "identifier") == 0)
    {
      proc_replace_macro_invocation(n);
    }
    else
    {
//      printf("unprocessed node type: %s\n", ts_node_type(n));
    }
  };

  IditorUtil::traverse(ts_tree_root_node(tree), unprocessed, f);
  printf("Preproc result:\n%s\n", result.c_str());
  return result;
}

std::string Preproc::getPreprocessedFromFile(const std::string &filePath)
{
  auto source_dir_terminator = filePath.find_last_of('/');

  std::string source_dir;

  if (source_dir.empty() && source_dir_terminator != std::string::npos) {
    source_dir = filePath.substr(0, source_dir_terminator);
  }
  return getPreprocessed(IditorUtil::readFile(filePath), source_dir);
}
