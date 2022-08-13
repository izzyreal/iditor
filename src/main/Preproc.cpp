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

  auto proc_include = [&](TSNode& n) {
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

  auto proc_comment = [&](TSNode& n) {
    auto st = ts_node_start_byte(n);
    auto end = ts_node_end_byte(n);
    IditorUtil::removeNodeFromText(n, result, added);
  };

  auto proc_define = [&](TSNode& n) {
    auto id_node = ts_node_child(n, 1);
    auto id = IditorUtil::getNodeText(id_node, unprocessed.c_str());

    if (ts_node_child_count(n) >= 4)
    {
      auto value_node = ts_node_child(n, 2);
      auto value = IditorUtil::getNodeText(value_node, unprocessed.c_str());
      Globals::definitions[id] = value;
      printf("Registered preprocessor definition '%s %s'\n", id.c_str(), value.c_str());
    } else {
      Globals::definitions[id] = "";
      printf("Registered preprocessor definition '%s'\n", id.c_str());
    }
    IditorUtil::removeNodeFromText(n, result, added);
  };

  auto proc_replace_macro_invocation = [&](TSNode& n) {
    auto m = Globals::macros.find(IditorUtil::getNodeText(n, unprocessed.c_str()));

    if (m != Globals::macros.end())
    {
      auto ctx = ts_node_parent(n);
      auto parent_t = ts_node_type(ctx);

      if (strcmp(parent_t, "ERROR") == 0) return;

      if (strcmp(parent_t, "preproc_function_def") == 0) return;

      printf("call expression text: %s\n", IditorUtil::getNodeText(ctx, unprocessed.c_str()).c_str());
      printf("Found macro %s\n", (*m).first.c_str());

      auto parameters_node = ts_node_child(ctx, 1);
      std::vector<std::string> params;
      for (int i = 0; i < ts_node_child_count(parameters_node); i++)
      {
        auto nt = IditorUtil::getNodeText(ts_node_child(parameters_node, i), unprocessed.c_str());
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

  auto proc_replace_macro_definition = [&](TSNode& n) {
    auto id = IditorUtil::getNodeText(ts_node_child(n, 1), unprocessed.c_str());
    auto body = IditorUtil::getNodeText(ts_node_child(n, 3), unprocessed.c_str());

    auto args_node = ts_node_child(n, 2);
    std::vector<std::string> args;

    for (int i = 0; i < ts_node_child_count(args_node); i++)
    {
      auto nt = IditorUtil::getNodeText(ts_node_child(args_node, i), unprocessed.c_str());
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
//    printf("Registered preprocessor function definition '%s %s'\n", id.c_str(), args_concatenated_again.c_str());
    IditorUtil::removeNodeFromText(n, result, added);
  };

  auto proc_preproc_ifdef = [&](TSNode& n){
    auto id_node = ts_node_child(n, 1);
    auto def_to_find = unprocessed.substr(ts_node_start_byte(id_node), ts_node_end_byte(id_node) - ts_node_start_byte(id_node));

    if (Globals::definitions.find(def_to_find) == Globals::definitions.end())
    {
//        printf("Definition %s was not found\n", def_to_find.c_str());

      // Remove all lines between #ifdef and #endif
      // Do not traverse further into the scope
      IditorUtil::removeNodeFromText(n, result, added);
      return false;
    } else {
      auto remainingNode = ts_node_child(n, 2);
      auto st = ts_node_start_byte(remainingNode);
      auto end = ts_node_end_byte(remainingNode);
      auto remainingCode = unprocessed.substr(st, end - st);
      IditorUtil::removeNodeFromText(n, result, added);
      result = remainingCode + result;
      added += remainingCode.length();
//        printf("Definition %s was found, continueing processing\n", def_to_find.c_str());
    }
    return true;
  };

  auto f = [&](TSNode& n) {
    auto t = ts_node_type(n);
    auto text = IditorUtil::getNodeText(n, unprocessed.c_str());

    if (strcmp(t, "comment") == 0)
    {
      proc_comment(n);
    }
    else if (strcmp(t, "preproc_ifdef") == 0)
    {
      if (!proc_preproc_ifdef(n))
      {
        return false;
      }
    }
    else if (strcmp(t, "preproc_function_def") == 0)
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
    return true;
  };

  auto root = ts_tree_root_node(tree);

  printf("AST: %s\n", ts_node_string(root));

  IditorUtil::traverse(root, result.c_str(), f);
//  printf("Preproc result:\n%s\n", result.c_str());
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

Preproc* Preproc::_instance = nullptr;

Preproc *Preproc::get()
{
  if (_instance == nullptr)
  {
    _instance = new Preproc;
  }
  return _instance;
}
