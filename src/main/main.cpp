#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include "Editor.h"

#include "Globals.h"

#include <cmrc/cmrc.hpp>
#include <string_view>
#include <fstream>

CMRC_DECLARE(iditor);

void write_rc_file_to_run_dir(const std::string& path)
{
  auto fs = cmrc::iditor::get_filesystem();
  auto file = fs.open(path);
  auto data = std::string_view(file.begin(), file.size()).data();

  std::ofstream myfile;
  auto cwd = std::filesystem::current_path();
  myfile.open (cwd.append(path).c_str());
  myfile.write(data, file.size());
  myfile.close();
}

int main()
{
  write_rc_file_to_run_dir("SF-Mono-Regular.otf");
  
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
