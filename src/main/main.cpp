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

void HoldBrowserCallback(Fl_Widget *w, void *data) {
  Fl_Hold_Browser *brow = (Fl_Hold_Browser*)w;
  int line = brow->value();
  printf("[hold browser] item %d picked: %s\n", line, brow->text(line));
}

int escKeyConsumer(int event)
{
  if (event == FL_SHORTCUT && Fl::event_key() == FL_Escape)
  {
    return 1;
  }

  return 0;
}

int main()
{
  Fl::add_handler(escKeyConsumer);

  write_rc_file_to_run_dir("SF-Mono-Regular.otf");
  
  Globals::includeDirectories.emplace(
      "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/include/c++/v1");
  Globals::includeDirectories.emplace("/Users/izmar/git/editor/inctest");
  
  Fl::scrollbar_size(8);
  
  auto* win = new Fl_Window(500, 100, 600, 700, "iditor");

  win->begin();

  auto editor = new Editor(0, 0, win->w(), win->h());
  auto browser = new Fl_Hold_Browser(0, 0, 0, 0, "Suggestions");
  browser->box(FL_FLAT_BOX);
  browser->callback(HoldBrowserCallback);
  browser->color(FL_DARK3);
  browser->color2(FL_GRAY);
  browser->selection_color(FL_BLUE);
  browser->textcolor(FL_GRAY);
  browser->scrollbar_size(0);
  browser->scrollbar_width(0);
  browser->textsize(12);
  browser->clear_visible_focus();
  browser->hide();
  editor->setBrowser(browser);
  editor->text("class Foo { int x = 42; };\n");
  win->add(editor);
  win->add(browser);
  win->resizable(editor);
  win->show();
  win->end();
  auto result = Fl::run();
  return result;
}
