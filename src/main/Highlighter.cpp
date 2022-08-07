#include "Highlighter.h"

#include "Globals.h"

#include <FL/Fl_Text_Buffer.H>

#include <tree_sitter/api.h>
#include <tree_sitter/highlight.h>
#include <cstring>

extern "C" {
TSLanguage *tree_sitter_cpp();
}

void Highlighter::do_highlighting(TSTree* old_tree, TSTree* new_tree, const char* text, Fl_Text_Buffer* style_buffer)
{
  const char* highlight_names[5] =  { "function", "type", "constant", "keyword", "string" };
  const char *ATTRIBUTE_STRINGS[5] = { "", "", "", "", "" };
  const auto hl = ts_highlighter_new(highlight_names, ATTRIBUTE_STRINGS, 5);

  ts_highlighter_add_language(hl, "source.c++", "", tree_sitter_cpp(), Globals::highlight_query, "", "", strlen(Globals::highlight_query), 0, 0);

  auto buf = ts_highlight_buffer_new();
  auto root = ts_tree_root_node(new_tree);
  uint32_t change_count = 0;
  auto changed_ranges = ts_tree_get_changed_ranges(old_tree, new_tree, &change_count);
  std::vector<std::pair<unsigned int, unsigned int>> changed_ranges_as_pairs;

  for (int i = 0; i < change_count; i++)
  {
    auto st = changed_ranges->start_byte;
    auto end = changed_ranges->end_byte;
    changed_ranges_as_pairs.emplace_back(std::pair{st, end});
    changed_ranges++;
  }

  auto slice = ts_highlighter_return_highlights(hl, "source.c++", text, strlen(text), &root, buf);
  int last_idx = -2;
  for (uint32_t i = 0; i < slice.len; i++)
  {
    auto ev = slice.arr[i];
    auto idx = ev.index;
    auto st = ev.start;
    auto end = ev.end;

//    if (idx  == -1) printf("Pos event: %d, %d\n", st, end);
//    if (idx  == -2) printf("Unknown event: %d, %d\n", st, end);
//    if (idx >= 0) printf("Known event %s %d, %d\n", highlight_names[idx], st, end);

    if (idx >= 0) { last_idx = idx; continue; }
    if (idx == -2) { last_idx = -2; continue; }

    if (idx == -1)
    {
      std::string style = "A";
      if (last_idx >= 0) style[0] += (last_idx + 1);
      for (auto &r: changed_ranges_as_pairs)
      {
        auto n1 = ts_node_first_child_for_byte(root, r.first);
        auto change_st = ts_node_start_byte(n1);

        auto n2 = ts_node_first_child_for_byte(root, r.second);
        auto change_end = n2.id == nullptr ? r.second : ts_node_end_byte(n1);

        for (uint32_t j = st; j < end; j++)
        {
          if (j >= change_st && j <= change_end)
          {
            style_buffer->replace(j, j + 1, style.c_str());
          }
        }
      }
    }
  }

  ts_highlighter_free_highlights(slice);
}
