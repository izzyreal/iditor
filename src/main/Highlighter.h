#pragma once

#include <thread>

class TSTree;
class TSHighlighter;
class TSHighlightBuffer;

class Fl_Text_Buffer;

class Highlighter
{
public:
  void highlight(TSTree* old_tree, TSTree* new_tree, const char* text, Fl_Text_Buffer* style_buffer);

  Highlighter();
  ~Highlighter();

  bool isRunning();
  void stop();

private:
  std::thread hl_thread;
  TSHighlighter* hl;
  TSHighlightBuffer* buf;
  bool running = false;
  void do_highlight(TSTree* old_tree, TSTree* new_tree, const char* text, Fl_Text_Buffer* style_buffer);

};
