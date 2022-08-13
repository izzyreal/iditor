#include "TreeDiff.h"

#include "Declarations.h"
#include "IditorUtil.h"
#include "Db.h"

std::vector<std::string> TreeDiff::getNewIncludes(TSTree *t1, TSTree *t2, const char* text, Project& p)
{
  uint32_t change_count;
  auto ranges = ts_tree_get_changed_ranges(t1, t2, &change_count);
  std::vector<std::string> result;
  std::vector<TSNode> newIncludes;

  auto f = [&](TSNode n) {
    for (int i = 0; i < change_count; i++)
    {
      auto r = ranges[i];
      auto rst = r.start_byte;
      auto rend = r.end_byte;
      auto nst = ts_node_start_byte(n);
      auto nend = ts_node_end_byte(n);

      if ((nst >= rst && nst <= rend) ||
          (nend <= rend && nend >= rst))
      {
        if (strcmp(ts_node_type(n), "preproc_include") == 0 ||
            strcmp(ts_node_type(n), "system_lib_string") == 0)
        {
          newIncludes.emplace_back(n);
        }
      }
    }
    return true;
  };

  IditorUtil::traverse(ts_tree_root_node(t2), text, f);

  for (auto& n : newIncludes)
  {
    auto t = ts_node_type(n);
    auto c = ts_node_child(n, 1);
    auto file_name = IditorUtil::getNodeText(c, text);
    IditorUtil::cleanIncludeFilename(file_name);

    auto file = IditorUtil::findIncludeFileInIncludeDirs(file_name);
    if (!file.empty())
    {
      for (auto &d: Declarations::getFromFile(file[0]))
      {
        Db::instance()->insert_declaration(d->getName(), file[0]);
      }
    }
    else
    {
    }
  }
  return {};
}
