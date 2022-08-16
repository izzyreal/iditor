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
//  printf("AST: %s\n", ts_node_string(ts_tree_root_node(tree)));

  std::string result;
  int added = 0;



  auto proc_preproc_include = [&](TSNode& n) {
    result = result.substr(0, ts_node_start_byte(n) + added);

    for (auto &i : IditorUtil::getFirstIdentifier(ts_node_child(n, 0), unprocessed)) {
      auto trimmed = i.length() >= 2 ? i.substr(1, i.length() - 2) : i;
      auto path = source_dir;
      path = path.append("/").append(trimmed);

      if (!exists(std::filesystem::path(path)))
      {
        auto candidate = IditorUtil::findIncludeFileInIncludeDirs(trimmed);
        if (!candidate.empty())
          path = candidate[0];
      }

      if (exists(std::filesystem::path(path)))
      {
        printf("\n==== Including preprocessed copy of file %s ====\n", path.c_str());
        result = result.append(getPreprocessedFromFile(path));
      }
    }
  };

  auto proc_define = [&](TSNode& n) {
    auto id_node = ts_node_child(n, 1);
    auto id = IditorUtil::getNodeText(id_node, unprocessed.c_str());

    if (ts_node_child_count(n) >= 4)
    {
      auto preproc_arg_node = ts_node_child(n, 2);
      auto preproc_arg = IditorUtil::getNodeText(preproc_arg_node, unprocessed.c_str()).substr(1);
      Globals::definitions[id] = preproc_arg;
      printf("Registered preprocessor definition: '%s, with arg: %s'\n", id.c_str(), preproc_arg.c_str());
    } else {
      Globals::definitions[id] = "";
      printf("Registered preprocessor definition '%s'\n", id.c_str());
    }
//    result = result.append(unprocessed.substr(ts_node_end_byte(n)));
    printf("");
//    IditorUtil::removeNodeFromText(n, result, added);
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
//      result = result.substr(0, ts_node_start_byte(n) + added);
      auto original_length = ts_node_end_byte(n) - ts_node_start_byte(n);
//      result += expanded;
//      added += expanded.size() - original_length;
//      result += unprocessed.substr(ts_node_end_byte(n));
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
//    IditorUtil::removeNodeFromText(n, result, added);
  };

  auto proc_preproc_ifdef = [&](TSNode& n) {
    std::string ifdefOrIfndef = IditorUtil::getNodeText(ts_node_child(n, 0), unprocessed.c_str());
    std::string::iterator end_pos = std::remove(ifdefOrIfndef.begin(), ifdefOrIfndef.end(), ' ');
    ifdefOrIfndef.erase(end_pos, ifdefOrIfndef.end());
    bool isIfdef = ifdefOrIfndef == "#ifdef"; // We assume it's #ifndef otherwise
    auto id_node = ts_node_child(n, 1);
    auto def_to_find = IditorUtil::getNodeText(id_node, unprocessed.c_str());

    if ((isIfdef && Globals::definitions.find(def_to_find) != Globals::definitions.end()) ||
        (!isIfdef && Globals::definitions.find(def_to_find) == Globals::definitions.end()))
    {
      auto st = ts_node_start_byte(ts_node_child(n, 2));
      auto end = ts_node_end_byte(ts_node_child(n, ts_node_child_count(n) - 2));
      auto body = unprocessed.substr(st, end - st);
      result = result.append(getPreprocessed(body, source_dir)).append("\n");
    }
  };

  std::function<bool(TSNode&)> resolve_condition;
  resolve_condition = [&](TSNode& condition_node)->bool {
    auto condition_type = ts_node_type(condition_node);
    if (strcmp(condition_type, "binary_expression") == 0)
    {
      auto left_node = ts_node_child(condition_node, 0);
      auto left_condition_is_met = resolve_condition(left_node);
      auto binary_operator = ts_node_type(ts_node_child(condition_node, 1));

      if (strcmp(binary_operator, "||") == 0 && left_condition_is_met)
      {
        return true;
      }

      auto right_node = ts_node_child(condition_node, 2);
      auto right_condition_is_met = resolve_condition(right_node);

      if (strcmp(binary_operator, "||") == 0)
      {
        return right_condition_is_met;
      }

      return left_condition_is_met && right_condition_is_met;
    }
    else if (strcmp(condition_type, "unary_expression") == 0)
    {
      auto c2 = ts_node_child(condition_node, 1);
      return !resolve_condition(c2);
    }
    else if (strcmp(condition_type, "preproc_defined") == 0)
    {
      auto id_node = ts_node_child(condition_node, 2);
      auto id_text = IditorUtil::getNodeText(id_node, unprocessed.c_str());
      return Globals::definitions.find(id_text) != Globals::definitions.end();
    }
    return false;
  };

  auto proc_preproc_if = [&](TSNode& n) {
    auto condition_node = ts_node_child(n, 1);
    auto condition_is_met = resolve_condition(condition_node);
    if (condition_is_met)
    {
      auto body = IditorUtil::getNodeText(ts_node_child(n, 3), unprocessed.c_str());
      result = result.append(getPreprocessed(body, source_dir));
    }
  };

  auto f = [&](TSNode& n) -> bool {
    auto t = ts_node_type(n);
    if (strcmp(t, "preproc_ifdef") == 0)
    {
      proc_preproc_ifdef(n);
      return false;
    }
    else if (strcmp(t, "preproc_function_def") == 0)
    {
      proc_replace_macro_definition(n);
    }
    else if (strcmp(t, "preproc_call") == 0)
    {
//      printf("");
    }
    else if (strcmp(t, "preproc_include") == 0)
    {
      proc_preproc_include(n);
    }
    else if (strcmp(t, "preproc_def") == 0)
    {
      proc_define(n);
    }
    else if (strcmp(t, "identifier") == 0)
    {
//      proc_replace_macro_invocation(n);
    }
    else if (strcmp(t, "preproc_if") == 0)
    {
        proc_preproc_if(n);
        return false;
    }
    else
    {
      std::vector<std::string> wanted {"function_definition"};
      bool isInIfDef = false;
      auto p = ts_node_parent(n);

      while (!ts_node_is_null(p))
      {
        if (strcmp(ts_node_type(p), "preproc_ifdef") == 0)
        {
          isInIfDef = true;
          break;
        }
        p = ts_node_parent(p);
      }

      if (!isInIfDef && std::find(wanted.begin(), wanted.end(), std::string(ts_node_type(n))) != wanted.end())
      {
        result = result.append(IditorUtil::getNodeText(n, unprocessed.c_str()));
      }
    }
    return true;
  };

  auto root = ts_tree_root_node(tree);

  IditorUtil::traverse(root, f);
//  printf("Preproc result:\n%s\n", result.c_str());
  return result;
}

std::string Preproc::getPreprocessedFromFile(const std::string &filePath)
{
  printf("==== getting preprocessed from filePath %s\n", filePath.c_str());
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
