#include "Editor.h"

#include "IditorUtil.h"
#include "Preproc.h"
#include "Db.h"

extern "C" {
TSLanguage *tree_sitter_cpp();
}

const char *tsinput_read_function(void *payload, uint32_t byte_index, TSPoint /*position*/, uint32_t *bytes_read)
{
  auto tbuff = (Fl_Text_Buffer *) payload;
  auto res = tbuff->text_range((int) byte_index, (int) tbuff->length());
  *bytes_read = tbuff->length() - byte_index;
  return res;
}

Editor::Editor(int X, int Y, int W, int H)
    : EditorDraw(X, Y, W, H), browser(nullptr)
{
  load_font();
  mVScrollBar->color(FL_BLACK);
  mHScrollBar->color(FL_BLACK);

  ts_parser_set_language(parser, tree_sitter_cpp());

  static const Fl_Text_Editor::Style_Table_Entry stable[] = {
      {FL_GRAY,         test_font, 12, ATTR_BGCOLOR},
      {FL_DARK_GREEN,   test_font, 12, ATTR_BGCOLOR},
      {FL_DARK_CYAN,    test_font, 12, ATTR_BGCOLOR},
      {FL_DARK_YELLOW,  test_font, 12, ATTR_BGCOLOR},
      {FL_DARK_MAGENTA, test_font, 12, ATTR_BGCOLOR},
  };

  mStyleBuffer = new Fl_Text_Buffer();
  buffer(new Fl_Text_Buffer());

  int stable_size = sizeof(stable) / sizeof(stable[0]);
  highlight_data(style_buffer(), stable, stable_size, 'A', nullptr, nullptr);
  buffer()->add_modify_callback(ModifyCallback_STATIC, (void *) this);

  color(FL_BLACK);
  color2(FL_YELLOW);
  cursor_color(FL_LIGHT1);
  cursor_style(SIMPLE_CURSOR);
  selection_color(FL_DARK_CYAN);

  Fl::add_timeout(0.5, Editor::blinkCursor, this);

  tree = ts_parser_parse_string(parser, nullptr, "", 0);
}


void Editor::blinkCursor(void *data)
{
  auto editor = (Editor *) data;
  editor->mCursorOn = editor->mCursorOn == 0 ? 1 : 0;

  editor->redraw();
  editor->browser->redraw();
  Fl::repeat_timeout(0.5, blinkCursor, data);
}

Editor::~Editor()
{
  unload_font();
  ts_tree_delete(tree);
  ts_parser_delete(parser);
}

int Editor::handle(int event)
{
  buffer()->primary_selection()->position(&sel_st, &sel_end);

  if (event == FL_KEYBOARD && Fl::event_ctrl() && Fl::event_key() == FL_BackSpace)
  {
    for (int i = 0; i < buffer()->length(); i++)
    {
      while (buffer()->is_word_separator(i) && i < buffer()->length())
      {
        i++;
      }

      if (i >= buffer()->length())
      {
        break;
      }

      auto st = buffer()->word_start(i);
      auto end = buffer()->word_end(i);
      auto word = buffer()->text_range(st, end);
      Db::instance()->insert_declaration(word, "");
      i = end;
    }
    Db::instance()->print_declarations();
    return 1;
  }

  if (event == FL_ENTER)
  {
    // Maybe on Windows we need to show_cursor and therefor also browser->redraw.
    // See Fl_Text_Editor.cxx implementation for the origin of this comment.
    // Try it on Windows without and check if either statement is really necessary.
    show_cursor(mCursorOn);
    browser->redraw();
    return 1;
  }

  if (!browser_items.empty() > 0 && event == FL_KEYBOARD)
  {
    if (Fl::event_key() == FL_Up || Fl::event_key() == FL_Down)
    {
      return browser->handle(event);
    }

    if (Fl::event_key() == FL_Left || Fl::event_key() == FL_Right)
    {
      hide_browser();
    }

    if (Fl::event_key() == FL_Enter)
    {
      auto word_st = buffer()->word_start(insert_position());
      auto word_end = buffer()->word_end(insert_position());

      if (word_st == word_end || word_st > word_end)
      {
        word_st = buffer()->word_start(insert_position() - 1);
        word_end = buffer()->word_end(insert_position() - 1);
      }

      buffer()->remove(word_st, word_end);

      auto suggestion_word = browser_items[browser->value() - 1];

      insert(suggestion_word.c_str());
      set_changed();
      if (when() & FL_WHEN_CHANGED) do_callback();

      hide_browser();
      restart_blink_timer();
      return 1;
    }
  }

  auto result = Fl_Text_Editor::handle(event);

  if (event == FL_LEFT_MOUSE)
  {
    restart_blink_timer();
    hide_browser();
  }

  if (event == FL_MOUSEWHEEL)
  {
    if (!browser_items.empty())
      show_browser();
  }

  if (event == FL_KEYBOARD)
  {
    int key = Fl::event_key();
    switch (key)
    {
      case FL_BackSpace:
      case FL_Up:
      case FL_Down:
      case FL_Left:
      case FL_Right:
      case FL_Home:
      case FL_End:
      case FL_Page_Up:
      case FL_Page_Down:
      {
        restart_blink_timer();
        break;
      }
      default:
        break;
    }
  }

  return result;
}

void Editor::ModifyCallback(int pos, int nInserted, int nDeleted, int, const char *)
{
  if (nDeleted > 0)
  {
    style_buffer()->remove(pos, pos + nDeleted);
  }

  if (nInserted > 0)
  {
    restart_blink_timer();
  }

  if (nDeleted > 0 || nInserted > 0)
  {
    populate_and_show_suggestions(pos, nDeleted);

    if (sel_st == sel_end) sel_st = sel_end = mCursorPos;

    auto edit_st = sel_st;
    auto edit_old_end = sel_end;
    auto edit_new_end = (edit_st + nInserted) - nDeleted;

    reparse_edit(edit_st, edit_old_end, edit_new_end);

    sel_st = 0;
    sel_end = 0;
  }

  if (browser_items.empty())
  {
    hide_browser();
  }

  if (nDeleted > 0)
  {
    return;
  }

  std::vector<std::string> keywords{"extern", "auto", "sizeof", "switch", "case", "static", "const", "new", "for", "if",
                                    "else", "void", "int", "bool", "long", "float", "struct", "short", "unsigned",
                                    "class"};
  std::string text = buffer()->text();

  auto highlight = [&](TSNode n) {
    auto st = static_cast<int>(ts_node_start_byte(n));
    auto end = static_cast<int>(ts_node_end_byte(n));
    auto node_text = IditorUtil::getNodeText(n, text);

    if (std::find(keywords.begin(), keywords.end(), node_text) != keywords.end())
    {
      for (int i = st; i < end; i++)
      {
        style_buffer()->replace(i, i + 1, "B");
      }
    }
    else
    {
      auto t = ts_node_type(n);
      std::string style = "A";

      if (std::string(t).find("identifier") != std::string::npos)
      {
        style = "C";
      }
      else if (strcmp(t, "number_literal") == 0)
      {
        style = "D";
      }
      else if (strcmp(t, "#include") == 0)
      {
        style = "E";
      }

      for (int i = st; i < end; i++)
      {
        style_buffer()->replace(i, i + 1, style.c_str());
      }
    }
  };

  if (nDeleted == 0 && nInserted == 0)
  {
    return;
  }

  std::vector<TSNode> leaf_nodes;
  auto text_tree = ts_parser_parse_string(parser, nullptr, text.c_str(), strlen(text.c_str()));
  IditorUtil::collectLeafNodes(ts_tree_root_node(text_tree), leaf_nodes);

  for (int i = 0; i < text.size(); i++)
  {
    style_buffer()->replace(i, i + 1, "A");
  }

  for (auto &n: leaf_nodes)
  {
    highlight(n);
  }
}

void Editor::load_font()
{
  auto font_file = std::filesystem::current_path().append("SF-Mono-Regular.otf");
  loaded_font = i_load_private_font(font_file.c_str());
  if (loaded_font)
  {
    Fl::set_font(test_font, "SF Mono Regular");
  }
}

void Editor::unload_font()
{
  if (loaded_font)
  {
    auto font_file = std::filesystem::current_path().append("SF-Mono-Regular.otf");
    v_unload_private_font(font_file.c_str());
  }
  loaded_font = 0;
}

void Editor::setBrowser(Fl_Hold_Browser *_browser)
{
  browser = _browser;
  browser->textfont(test_font);
}

void Editor::show_browser()
{
  browser->show();
  int X = 0, Y = 0;

  auto st = buffer()->word_start(mCursorPos);
  auto end = buffer()->word_start(mCursorPos);

  if (st > mCursorPos || st == end) st = buffer()->word_start(mCursorPos - 1);

  browser->clear();

  for (auto &s: browser_items)
  {
    browser->add(s.c_str());
  }
  browser->select(1);

  if (position_to_xy(st, &X, &Y))
  {
    browser->resize(X - 3, Y + 16, 200, static_cast<int>(browser_items.size()) * 14);
  }
  redraw();
  browser->redraw();
}

void Editor::hide_browser()
{
  browser_items.clear();
  browser->hide();
  redraw();
}
void Editor::restart_blink_timer()
{
  Fl::remove_timeout(Editor::blinkCursor);
  mCursorOn = 1;
  Fl::add_timeout(0.5, Editor::blinkCursor, this);
}
void Editor::populate_and_show_suggestions(int new_pos, int nDeleted)
{
  browser_items.clear();

  auto word_st = buffer()->word_start(new_pos + (nDeleted > 0 ? -1 : 0));
  auto word_end = buffer()->word_end(new_pos);

  if (word_end > word_st)
  {
    auto search_string = buffer()->text_range(word_st, word_end);
    auto startsWithSuggestions = Db::instance()->get_declarations_starting_with(search_string);

    if (!startsWithSuggestions.empty())
    {
      browser_items = startsWithSuggestions;
    }
    else
    {
      auto containsSuggestions = Db::instance()->get_declarations_containing(search_string);

      if (!containsSuggestions.empty())
      {
        browser_items = containsSuggestions;
      }
    }

    if (!browser_items.empty())
    {
      show_browser();
    }
  }
}

void Editor::text(const char *val)
{
  buffer()->text(val);
}

void Editor::reparse_edit(int st, int old_end, int new_end)
{
  TSInputEdit edit{(uint32_t) st, (uint32_t) old_end, (uint32_t) new_end};
  ts_tree_edit(tree, &edit);

  TSInput tsinput{buffer(), &tsinput_read_function, TSInputEncodingUTF8};
  tree = ts_parser_parse(parser, tree, tsinput);

  auto str = ts_node_string(ts_tree_root_node(tree));
  printf("\nTree: %s\n", str);
}
