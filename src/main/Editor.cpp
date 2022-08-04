#include "Editor.h"

#include "IditorUtil.h"
#include "Preproc.h"
#include "Db.h"

extern "C" {
TSLanguage *tree_sitter_cpp();
}

Editor::Editor(int X, int Y, int W, int H)
    : Fl_Text_Editor(X, Y, W, H), browser(nullptr)
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

  tbuff = new Fl_Text_Buffer();
  sbuff = new Fl_Text_Buffer();

  buffer(tbuff);

  int stable_size = sizeof(stable) / sizeof(stable[0]);
  highlight_data(sbuff, stable, stable_size, 'A', nullptr, nullptr);
  tbuff->add_modify_callback(ModifyCallback_STATIC, (void *) this);

  color(FL_BLACK);
  color2(FL_YELLOW);
  cursor_color(FL_DARK_YELLOW);
  cursor_style(SIMPLE_CURSOR);
  selection_color(FL_DARK_CYAN);

  Fl::add_timeout(0.5, Editor::blinkCursor, this);
}

void Editor::blinkCursor(void *data)
{
  auto editor = (Editor *) data;
  editor->cursorOn = !editor->cursorOn;

  if (editor->cursorOn) {
    editor->cursor_style(SIMPLE_CURSOR);
  } else {
    editor->cursor_style(DIM_CURSOR);
  }
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
  if (event == FL_KEYBOARD && Fl::event_ctrl() && Fl::event_key() == FL_BackSpace) {
    for (int i = 0; i < tbuff->length(); i++) {
      while (tbuff->is_word_separator(i) && i < tbuff->length()) i++;
      if (i >= tbuff->length()) break;
      auto st = tbuff->word_start(i);
      auto end = tbuff->word_end(i);
      auto word = tbuff->text_range(st, end);
      Db::instance()->insert_declaration(word, "");
      i = end;
    }
    Db::instance()->print_declarations();
    return 1;
  }

  if (event == FL_ENTER)
  {
    // Maybe on Windows we need to show_cursor. See Fl_Text_Editor.cxx implementation.
    // Try it on Windows without and check if really necessary.
    show_cursor(mCursorOn);
    return 1;
  }

  auto result = Fl_Text_Editor::handle(event);

  if (event == FL_LEFT_MOUSE) {
    Fl::remove_timeout(Editor::blinkCursor);
    cursor_style(SIMPLE_CURSOR);
    Fl::add_timeout(0.5, Editor::blinkCursor, this);
    hide_browser();
  }

  if (event == FL_MOUSEWHEEL)
  {
    if (browser_items.size() > 0)
      show_browser();
  }

  if (event == FL_KEYBOARD) {
    int key = Fl::event_key();
    switch (key) {
      case FL_BackSpace:
      case FL_Up:
      case FL_Down:
      case FL_Left:
      case FL_Right:
      case FL_Home:
      case FL_End:
      case FL_Page_Up:
      case FL_Page_Down: {
        Fl::remove_timeout(Editor::blinkCursor);
        cursor_style(SIMPLE_CURSOR);
        Fl::add_timeout(0.5, Editor::blinkCursor, this);
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
  if (nDeleted > 0) {
    sbuff->remove(pos, pos + nDeleted);
  }

  if (nInserted > 0) {
    Fl::remove_timeout(Editor::blinkCursor);
    cursor_style(SIMPLE_CURSOR);
    Fl::add_timeout(0.5, Editor::blinkCursor, this);
  }

  const char *source_code = tbuff->text();

//  auto preprocessed = Preproc().getPreprocessed(source_code, "/Users/izmar.verhage/git/iditor/inctest");

  std::string preprocessed = source_code;

  tree = ts_parser_parse_string(parser, nullptr, preprocessed.c_str(), strlen(preprocessed.c_str()));

  TSNode root_node = ts_tree_root_node(tree);
  char *string = ts_node_string(root_node);
//  printf("Syntax tree: %s\n", string);
  free(string);

  if (nDeleted > 0 || nInserted > 0) {
    auto word_st = tbuff->word_start(pos + (nDeleted > 0 ? -1 : 0));
    auto word_end = tbuff->word_end(pos);

    if (word_end > word_st) {
      auto search_string = tbuff->text_range(word_st, word_end);
      browser_items.clear();

      auto startsWithSuggestions = Db::instance()->get_declarations_starting_with(search_string);
      if (!startsWithSuggestions.empty()) {
        printf("Did you mean: ");
        for (auto &d: startsWithSuggestions) {
          printf("%s ", d.c_str());
        }
        printf("\n");
        browser_items = startsWithSuggestions;
      } else {
        auto containsSuggestions = Db::instance()->get_declarations_containing(search_string);
        if (!containsSuggestions.empty()) {
          printf("Did you mean: ");
          for (auto &d: containsSuggestions) {
            printf("%s ", d.c_str());
          }
          printf("\n");
          browser_items = containsSuggestions;
        }
      }

      if (!browser_items.empty()) {
        show_browser();
      } else {
        hide_browser();
      }
    } else {
      hide_browser();
    }
  } else {
    hide_browser();
  }

  if (nDeleted > 0) {
    return;
  }

  std::vector<std::string> keywords{"extern", "auto", "sizeof", "switch", "case", "static", "const", "new", "for", "if",
                                    "else", "void", "int", "bool", "long", "float", "struct", "short", "unsigned",
                                    "class"};
  std::string text = tbuff->text();

  auto highlight = [&](TSNode n) {
    auto st = ts_node_start_byte(n);
    auto end = ts_node_end_byte(n);
    auto node_text = IditorUtil::getNodeText(n, text);

    if (std::find(keywords.begin(), keywords.end(), node_text) != keywords.end()) {
      for (int i = st; i < end; i++) {
        sbuff->replace(i, i + 1, "B");
      }
    } else {
      auto t = ts_node_type(n);
      std::string style = "A";

      if (std::string(t).find("identifier") != std::string::npos) {
        style = "C";
      } else if (strcmp(t, "number_literal") == 0) {
        style = "D";
      } else if (strcmp(t, "#include") == 0) {
        style = "E";
      }

      for (int i = st; i < end; i++)
        sbuff->replace(i, i + 1, style.c_str());
    }
  };

  if (nDeleted == 0 && nInserted == 0) return;

  std::vector<TSNode> leaf_nodes;
  auto text_tree = ts_parser_parse_string(parser, nullptr, text.c_str(), strlen(text.c_str()));
  IditorUtil::collectLeafNodes(ts_tree_root_node(text_tree), leaf_nodes);

  for (int i = 0; i < text.size(); i++)
    sbuff->replace(i, i + 1, "A");

  for (auto &n: leaf_nodes)
    highlight(n);
}

void Editor::load_font()
{
  auto font_file = std::filesystem::current_path().append("SF-Mono-Regular.otf");
  loaded_font = i_load_private_font(font_file.c_str());
  if (loaded_font) {
    Fl::set_font(test_font, "SF Mono Regular");
  }
}

void Editor::unload_font()
{
  if (loaded_font) {
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

  auto st = tbuff->word_start(mCursorPos);
  auto end = tbuff->word_start(mCursorPos);

  if (st > mCursorPos || st == end) st = tbuff->word_start(mCursorPos - 1);

  browser->clear();

  for (auto &s: browser_items) {
    browser->add(s.c_str());
  }
  browser->select(1);

  if (position_to_xy(st, &X, &Y)) {
    browser->resize(X - 3, Y + 16, 200, browser_items.size() * 14);
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
