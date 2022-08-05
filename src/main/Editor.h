#pragma once

#include "EditorDraw.h"
#include <FL/Fl_Hold_Browser.H>

#include <tree_sitter/api.h>

#include <utility>
#include <vector>

#ifdef _WIN32
# define i_load_private_font(PATH) AddFontResourceEx((PATH),FR_PRIVATE,0)
# define v_unload_private_font(PATH) RemoveFontResourceEx((PATH),FR_PRIVATE,0)
#elif __APPLE__

#include <cstdio>
#include <src/drivers/Cocoa/Fl_Cocoa_Window_Driver.H>

static int i_load_private_font(const char *pf)
{
  int result = 0;
  CFErrorRef err;

  CFURLRef fontURL = CFURLCreateFromFileSystemRepresentation(kCFAllocatorDefault,
                                                             (const UInt8 *) pf,
                                                             strlen(pf),
                                                             false);

  if (CTFontManagerRegisterFontsForURL(fontURL, kCTFontManagerScopeProcess, &err)) {
    result = 1;
  } else {
    printf("Failed loading font: %s\n", pf);
  }

  if (fontURL) CFRelease(fontURL);
  return result;
}

static void v_unload_private_font(const char *pf)
{
  CFErrorRef err;

  CFURLRef fontURL = CFURLCreateFromFileSystemRepresentation(kCFAllocatorDefault,
                                                             (const UInt8 *) pf,
                                                             strlen(pf),
                                                             false);

  CTFontManagerUnregisterFontsForURL(fontURL, kCTFontManagerScopeProcess, &err);
  if (fontURL) CFRelease(fontURL);
}

#else

# define i_load_private_font(PATH) (int)FcConfigAppFontAddFile(NULL,(const FcChar8 *)(PATH))
# define v_unload_private_font(PATH) FcConfigAppFontClear(NULL)

#endif

class Editor : public EditorDraw {
public:
  Editor(int X, int Y, int W, int H);
  ~Editor() override;

  int handle(int event) override;

  void setBrowser(Fl_Hold_Browser* browser);

  void text(const char *val)
  {
    tbuff->text(val);
  }

private:
  Fl_Text_Buffer *tbuff;
  Fl_Text_Buffer *sbuff;
  Fl_Hold_Browser *browser;
  std::vector<std::string> preprocessorDefinitions;
  std::vector<std::string> browser_items;

  TSParser *parser = ts_parser_new();
  TSTree *tree = nullptr;

  int loaded_font = 0;
  Fl_Font test_font = (FL_FREE_FONT);

  void show_browser();
  void hide_browser();
  void populate_and_show_suggestions(int new_pos, int nDeleted);

  void restart_blink_timer();
  static void blinkCursor(void* data);

  void load_font();

  void unload_font();

  void ModifyCallback(int pos, int nInserted, int nDeleted, int, const char *);

  static void ModifyCallback_STATIC(int pos, int nInserted, int nDeleted, int nRestyled,
                                    const char *deletedText, void *cbarg)
  {
    auto *med = (Editor *) cbarg;
    med->ModifyCallback(pos, nInserted, nDeleted, nRestyled, deletedText);
  }
};
