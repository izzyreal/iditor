#include "Editor.h"

#include <string>
#include <algorithm>
#include "IditorUtil.h"
#include "Preproc.h"

extern "C" {
TSLanguage *tree_sitter_cpp();
}

Editor::Editor(int X, int Y, int W, int H) : Fl_Text_Editor(X, Y, W, H)
{
  load_font();

  ts_parser_set_language(parser, tree_sitter_cpp());

  static const Fl_Text_Editor::Style_Table_Entry stable[] = {
      {FL_GRAY,       test_font, 12, ATTR_BGCOLOR},
      {FL_DARK_GREEN, test_font, 12, ATTR_BGCOLOR},
      {FL_DARK_CYAN,  test_font, 12, ATTR_BGCOLOR},
      {FL_DARK_YELLOW,  test_font, 12, ATTR_BGCOLOR},
      {FL_DARK_MAGENTA,  test_font, 12, ATTR_BGCOLOR},
  };

  tbuff = new Fl_Text_Buffer();
  sbuff = new Fl_Text_Buffer();

  buffer(tbuff);

  int stable_size = sizeof(stable) / sizeof(stable[0]);
  highlight_data(sbuff, stable, stable_size, 'A', nullptr, nullptr);
  tbuff->add_modify_callback(ModifyCallback_STATIC, (void *) this);

  set_flag(NOBORDER);
  color(FL_BLACK);
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

  Fl::repeat_timeout(0.5, blinkCursor, data);
}

Editor::~Editor()
{
  free_extra_font();
  ts_tree_delete(tree);
  ts_parser_delete(parser);
}

int Editor::handle(int event)
{
  auto result = Fl_Text_Editor::handle(event);

  if (event == FL_KEYBOARD) {
    int key = Fl::event_key();
    switch (key) {
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

  const char *source_code = tbuff->text();

//  auto preprocessed = Preproc().getPreprocessed(source_code, "/Users/izmar/git/editor/inctest");

  std::string preprocessed = source_code;

  tree = ts_parser_parse_string(parser, nullptr, preprocessed.c_str(), strlen(preprocessed.c_str()));

  TSNode root_node = ts_tree_root_node(tree);
  char *string = ts_node_string(root_node);
//  printf("Syntax tree: %s\n", string);
  free(string);

  if (nDeleted > 0) {
    return;
  }

  std::vector<std::string> keywords{"extern", "auto", "sizeof", "switch", "case", "static", "const", "new", "for", "if", "else", "void", "int", "bool", "long", "float", "struct", "short", "unsigned", "class"};
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
  loaded_font = i_load_private_font("/Users/izmar/git/editor/resources/SF-Mono-Regular.otf");

  if (loaded_font) {
    Fl::set_font(test_font, "SF Mono Regular");
  }
}
