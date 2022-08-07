#include "Highlighter.h"

#include "Globals.h"

#include <FL/Fl_Text_Buffer.H>

#include <tree_sitter/api.h>
#include <cstring>

extern "C" {
TSLanguage *tree_sitter_cpp();
}

void Highlighter::do_highlighting(TSTree* tree, const char* text, Fl_Text_Buffer* style_buffer)
{
  style_buffer->remove(0, style_buffer->length());

  for (int i = 0; i < strlen(text); i++)
  {
    style_buffer->insert(i, "A");
  }

  uint32_t error_offset;
  TSQueryError error;
  auto query = ts_query_new(tree_sitter_cpp(), Globals::highlight_query, strlen(Globals::highlight_query), &error_offset, &error);

  TSQueryCursor *cursor = ts_query_cursor_new();
  ts_query_cursor_exec(cursor, query, ts_tree_root_node(tree));

  TSQueryMatch match;
  uint32_t capture_index;

  char style[2];
  style[1] = '\0';

  while (ts_query_cursor_next_capture(cursor, &match, &capture_index))
  {
    for (int i = 0; i < match.capture_count; i++)
    {
      style[0] = 'A';

      auto capture = match.captures[i];
      auto pattern_index = match.pattern_index;

      auto st = ts_node_start_byte(capture.node);
      auto end = ts_node_end_byte(capture.node);

      switch (pattern_index)
      {
        case 0: case 1: case 2: case 3: case 4: case 5: case 6:
          // identifier
          style[0]++;
          break;
        case 7: case 8:
          // type
          style[0] += 2;
          break;
        case 9: case 10:
          // constant
          style[0] += 3;
          break;
        case 11:
          // keyword
          style[0] += 4;
          break;
        default:
          break;
      }

      for (int j = 0; j < end - st; j++)
        style_buffer->replace((int)st + j, (int)st + 1 + j, style);
    }
  }

  ts_query_cursor_delete(cursor);
}
