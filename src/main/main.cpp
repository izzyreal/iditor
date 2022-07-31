#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include "Editor.h"

int main()
{
  Fl_Window *win = new Fl_Window(720, 480, "Editor");

  auto editor = new Editor(0, 0, win->w(), win->h());
//  editor->text("#include <string>\n\nnamespace foo {\n  class Bar {\n    private:\n      int x = 42;\n    public:\n      std::string y = \"42\";\n  };\n}\n");
  editor->text("#if __has_attribute(init_priority)\n"
               "# define _LIBCPP_INIT_PRIORITY_MAX __attribute__((init_priority(101)))\n"
               "#else\n"
               "# define _LIBCPP_INIT_PRIORITY_MAX\n"
               "#endif");
  editor->getCurrentNode();
  win->resizable(editor);
  win->show();

  auto result = Fl::run();
  return result;
}
