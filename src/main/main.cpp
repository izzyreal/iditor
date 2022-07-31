#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include "Editor.h"

#include "Globals.h"

int main()
{
  Globals::includeDirectories.emplace(
      "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/include/c++/v1");
  Globals::includeDirectories.emplace("/Users/izmar/git/editor/inctest");
  Fl::scrollbar_size(8);
  auto* win = new Fl_Window(500, 100, 600, 700, "Editor");

  auto editor = new Editor(0, 0, win->w(), win->h());
  editor->text("class Foo { int x = 42; };\n");
  win->resizable(editor);
  win->show();

  auto result = Fl::run();
  return result;
}
