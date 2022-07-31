#include "Preproc.h"
#include "IditorUtil.h"

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
        auto toAdd = getPreprocessedFromFile(path);
        result += toAdd;
        added += toAdd.size();
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
  };

  auto f = [&](TSNode n) {
    if (strcmp(ts_node_type(n), "#include") == 0) {
      proc_include(n);
    } else if (strcmp(ts_node_type(n), "preproc_def") == 0) {
      proc_define(n);
    } else {
//      printf("unprocessed node type: %s\n", ts_node_type(n));
    }
  };

  IditorUtil::traverse(ts_tree_root_node(tree), unprocessed, f);
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
