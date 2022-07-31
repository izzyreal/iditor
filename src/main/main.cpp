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
//  editor->text("#include \"a.h\"\nclass MyAaa : Aaa {};");
/*
  editor->text("// -*- C++ -*-\n"
               "//===------------------------------ vector --------------------------------===//\n"
               "//\n"
               "// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.\n"
               "// See https://llvm.org/LICENSE.txt for license information.\n"
               "// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception\n"
               "//\n"
               "//===----------------------------------------------------------------------===//\n"
               "\n"
               "#ifndef _LIBCPP_VECTOR\n"
               "#define _LIBCPP_VECTOR\n"
               "\n"
               "#include <__config>\n"
//               "#include <__bit_reference>\n"
//               "#include <__debug>\n"
//               "#include <__functional_base>\n"
//               "#include <__iterator/wrap_iter.h>\n"
//               "#include <__split_buffer>\n"
//               "#include <__utility/forward.h>\n"
//               "#include <algorithm>\n"
//               "#include <climits>\n"
//               "#include <compare>\n"
//               "#include <cstdlib>\n"
//               "#include <cstring>\n"
//               "#include <initializer_list>\n"
//               "#include <iosfwd> // for forward declaration of vector\n"
//               "#include <limits>\n"
//               "#include <memory>\n"
//               "#include <stdexcept>\n"
//               "#include <type_traits>\n"
//               "#include <version>\n"
//               "\n"
//               "#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)\n"
//               "#pragma GCC system_header\n"
//               "#endif\n"
//               "\n"
//               "_LIBCPP_PUSH_MACROS\n"
//               "#include <__undef_macros>\n"
//               "\n"
//               "_LIBCPP_BEGIN_NAMESPACE_STD\n"
//               "\n"
//               "template <bool>\n"
//               "class _LIBCPP_TEMPLATE_VIS __vector_base_common\n"
//               "{\n"
//               "protected:\n"
//               "    _LIBCPP_INLINE_VISIBILITY __vector_base_common() {}\n"
//               "    _LIBCPP_NORETURN void __throw_length_error() const;\n"
//               "    _LIBCPP_NORETURN void __throw_out_of_range() const;\n"
//               "};\n"
               "#endif\n"
               );
  */
  editor->text("#include <vector>");
  win->resizable(editor);
  win->show();

  auto result = Fl::run();
  return result;
}
