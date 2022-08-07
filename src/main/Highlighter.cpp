#include "Highlighter.h"

#include "Globals.h"

#include <FL/Fl_Text_Buffer.H>

#include <tree_sitter/api.h>
#include <tree_sitter/highlight.h>
#include <cstring>

extern "C" {
TSLanguage *tree_sitter_cpp();
}

TSNode find_common_ancestor(TSNode n1, TSNode n2)
{
  TSNode result;
  std::vector<TSNode> n1_parents;
  TSNode parent = ts_node_parent(n1);

  while (parent.id != nullptr)
  {
    n1_parents.emplace_back(parent);
    parent = ts_node_parent(parent);
  }

  parent = ts_node_parent(n2);

  while (parent.id != nullptr)
  {
    for (auto& p : n1_parents)
    {
      if (parent.id == p.id)
      {
        result = p;
        break;
      }
    }

    parent = ts_node_parent(parent);
  }

  return result;
}

void Highlighter::do_highlighting(TSTree* old_tree, TSTree* new_tree, const char* text, Fl_Text_Buffer* style_buffer)
{
  style_buffer->remove(0, style_buffer->length());

  for (int i = 0; i < strlen(text); i++)
  {
    style_buffer->insert(i, "A");
  }

  const char* highlight_names[5] =  { "function", "type", "constant", "keyword", "string" };
  const char *ATTRIBUTE_STRINGS[5] = { "class=function\0", "class=type\0", "class=constant\0", "class=keyword\0", "class=string\0" };
  const auto hl = ts_highlighter_new(highlight_names, ATTRIBUTE_STRINGS, 5);

  ts_highlighter_add_language(hl, "source.c++", "", tree_sitter_cpp(), Globals::highlight_query, "", "", strlen(Globals::highlight_query), 0, 0);

  auto buf = ts_highlight_buffer_new();
  auto root = ts_tree_root_node(new_tree);
  uint32_t change_count = 0;

//  auto changed_ranges = ts_tree_get_changed_ranges(old_tree, new_tree, &change_count);
//  printf("Changed ranges st: %d, end: %d\n", changed_ranges->start_byte, changed_ranges->end_byte);
//
//  auto st_node = ts_node_first_child_for_byte(root, changed_ranges->start_byte);
//  while (ts_node_first_child_for_byte(st_node, changed_ranges->start_byte).id != nullptr)
//  {
//    st_node = ts_node_first_child_for_byte(st_node, changed_ranges->start_byte);
//  }
//
//  auto last_text_char_index = changed_ranges->end_byte;
//
//  while (last_text_char_index >= strlen(text) ||
//         text[last_text_char_index] == '\n' ||
//         text[last_text_char_index] == '\0')
//  {
//    last_text_char_index--;
//  }
//
//  auto end_node = ts_node_first_child_for_byte(root, last_text_char_index);
//
//  auto common_ancestor = find_common_ancestor(st_node, end_node);

  auto slice = ts_highlighter_return_highlights(hl, "source.c++", text, strlen(text), &root, buf);

  for (uint32_t i = 0; i < slice.len; i++)
  {
    if (slice.arr[i].index >= 0)
    {
      auto name = highlight_names[slice.arr[i].index];
      printf("Event highlight name: %s\n", name);

      if (strcmp(name, "keyword") == 0 || strcmp(name, "function") == 0)
      {
        i++;
        auto next_event_st = slice.arr[i].start;
        auto next_event_end = slice.arr[i].end;
        for (unsigned int j = next_event_st; j < next_event_end; j++)
        {
          style_buffer->replace(j, j+1, "B\0");
        }
      }
    }
  }

  ts_highlighter_free_highlights(slice);
}
