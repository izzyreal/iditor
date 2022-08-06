#pragma once

#include <string>

class TSParser;
class Fl_Text_Buffer;

class Highlighter
{
public:
  static void do_highlighting(std::string& text, Fl_Text_Buffer* style_buffer, TSParser* parser);

};
