#pragma once

#include <FL/Fl_Text_Editor.H>

class EditorDraw : public Fl_Text_Editor
{
public:
  EditorDraw(int X, int Y, int W, int H, const char* l = nullptr);

  void draw() override;
};
