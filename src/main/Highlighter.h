#pragma once

class TSTree;

class Fl_Text_Buffer;

class Highlighter
{
public:
  static void do_highlighting(TSTree* old_tree, TSTree* new_tree, const char* text, Fl_Text_Buffer* style_buffer);

};
