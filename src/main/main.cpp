#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include "Editor.h"

#include "Globals.h"

int main()
{
  Globals::includeDirectories.emplace(
      "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/include/c++/v1");
  Globals::includeDirectories.emplace("/Users/izmar/git/editor/inctest");

  auto* win = new Fl_Window(720, 300, "Editor");

  auto editor = new Editor(0, 0, win->w(), win->h());
//  editor->text("#define min(X, Y)  ((X) < (Y) ? (X) : (Y))\n#define max(X, Y)  ((X) > (Y) ? (X) : (Y))\nmin (1, 2);\nmax (4, 5);\n");
  win->resizable(editor);
  win->show();

  auto result = Fl::run();
  return result;
}
