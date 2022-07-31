#include "Editor.h"

#include <string>
#include <filesystem>
#include <algorithm>
#include <fstream>
#include <set>

namespace fs = std::filesystem;

extern "C" {
TSLanguage *tree_sitter_cpp();
}

std::vector<std::string> Editor::includeDirectories = {
    "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/include/c++/v1"};

std::string getNodeText(TSNode node, std::string &text)
{
  auto st = ts_node_start_byte(node);
  auto end = ts_node_end_byte(node);
  return text.substr(st, end - st);
}

std::string readFile(const std::string &fileName)
{
  std::ifstream ifs(fileName.c_str(), std::ios::in | std::ios::binary | std::ios::ate);

  std::ifstream::pos_type fileSize = ifs.tellg();
  ifs.seekg(0, std::ios::beg);

  std::vector<char> bytes(fileSize);
  ifs.read(bytes.data(), fileSize);

  auto result = std::string(bytes.data(), fileSize);
  return result;
}

std::vector<std::string> Editor::getFirstIdentifier(TSNode n, std::string &text)
{
  auto sibling = ts_node_next_sibling(n);
  auto nodeText = getNodeText(n, text);

  while (!ts_node_is_null(sibling)) {
    auto sibling_type = ts_node_type(sibling);

    if (strcmp(sibling_type, "type_identifier") == 0 ||
        strcmp(sibling_type, "identifier") == 0) {
      auto candidate = getNodeText(sibling, text);
      return {candidate};
    }

    sibling = ts_node_next_sibling(sibling);
  }
  return {};
}

void Editor::addPreprocessorDefinitions(TSNode n, std::vector<std::string> &directives, std::string &text)
{
  auto t = ts_node_type(n);

  if (strcmp(t, "preproc_def") == 0) {
    auto defname = getFirstIdentifier(ts_node_child(n, 0), text);
    auto node_text = getNodeText(n, text);
//    printf("preproc def text: %s\n", node_text.c_str());
    if (!defname.empty()) {
      directives.push_back(defname[0]);
//      printf("preproc def name: %s\n", defname[0].c_str());
    }
  }

  for (int i = 0; i < ts_node_child_count(n); i++)
    addPreprocessorDefinitions(ts_node_child(n, i), directives, text);

}

std::string Editor::getNamespace(TSNode node, std::string &text)
{
  auto parent = ts_node_parent(node);
  std::string result;
  while (!ts_node_is_null(parent)) {
    auto t = ts_node_type(parent);
    if (strcmp(t, "namespace_definition") == 0) {
      for (int i = 0; i < ts_node_child_count(parent); i++) {
        auto child = ts_node_child(parent, i);
        auto child_type = ts_node_type(child);
        if (strcmp(child_type, "namespace") == 0) {
          auto ns_candidate = getFirstIdentifier(child, text);
          if (!ns_candidate.empty()) {
            result = ns_candidate[0].append("::").append(result);
          }
        }
      }
    }
    parent = ts_node_parent(parent);
  }
  return result;
}

void Editor::getDeclarations(TSNode node, std::vector<std::string> &declarations, std::string &text)
{
  auto t = ts_node_type(node);
  if (strcmp(t, "class") == 0/* || strcmp(t, "typedef") == 0*/) {
    auto className = getFirstIdentifier(node, text);
    if (!className.empty()) {
      auto ns = getNamespace(node, text);
      declarations.push_back(ns + className[0]);
    }
  }

  for (int i = 0; i < ts_node_child_count(node); i++)
    getDeclarations(ts_node_child(node, i), declarations, text);
}

void Editor::printDeclarations(TSTree *t, std::string &text, const std::string &file_name = "")
{
  std::vector<std::string> declarations;

  getDeclarations(ts_tree_root_node(t), declarations, text);

  if (declarations.empty()) {
//          printf("No declarations found in %s\n", entry.path().filename().c_str());
  } else {
    printf("\nDeclarations in string 'text'%s:\n", (file_name.empty() ? "" : (" file " + file_name)).c_str());
    for (auto &declaration: std::set(declarations.begin(), declarations.end()))
      printf("%s\n", declaration.c_str());
  }

}

void Editor::listIncludeDeclarations()
{
  for (auto &s: Editor::includeDirectories) {
    for (const auto &entry: fs::directory_iterator(s)) {

      if (entry.is_directory())
        continue;

      if (entry.path().filename().string() != "__config2" &&
          entry.path().filename().string() != "vector")
        continue;

      auto text = readFile(entry.path().string());
      auto headerTree = ts_parser_parse_string(parser, nullptr, text.c_str(), strlen(text.c_str()));
      cachePreprocessorDefinitions(ts_tree_root_node(headerTree), text);
      printDeclarations(headerTree, text, entry.path().filename().string());
    }
  }
}

Editor::Editor(int X, int Y, int W, int H) : Fl_Text_Editor(X, Y, W, H)
{
  load_font();

  ts_parser_set_language(parser, tree_sitter_cpp());

  listIncludeDeclarations();

  static const Fl_Text_Editor::Style_Table_Entry stable[] = {
      {FL_GRAY,       test_font, 12, ATTR_BGCOLOR},
      {FL_DARK_GREEN, test_font, 12, ATTR_BGCOLOR},
      {FL_RED,        test_font, 12, ATTR_BGCOLOR},
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

void collectLeafNodes(TSNode node, std::vector<TSNode> &leafNodes)
{
  if (ts_node_child_count(node) == 0) {
    leafNodes.emplace_back(node);
    return;
  }

  for (int i = 0; i < ts_node_child_count(node); i++) {
    collectLeafNodes(ts_node_child(node, i), leafNodes);
  }
}

std::vector<TSNode> getLeafAtPosition(const std::vector<TSNode> &leaf_nodes, int pos)
{
  assert(!leaf_nodes.empty());

  for (auto &node: leaf_nodes) {
    auto st = ts_node_start_byte(node);
    auto end = ts_node_end_byte(node);
    if (st <= pos && end > pos) {
      return {node};
    }
  }

  return {};
}

std::vector<TSNode> Editor::getCurrentNode()
{
  TSNode root_node = ts_tree_root_node(tree);
  std::vector<TSNode> leafNodes;
  collectLeafNodes(root_node, leafNodes);

  auto result = getLeafAtPosition(leafNodes, insert_position());

  if (!result.empty()) {
    auto desc = ts_node_string(result[0]);
    printf("Current node: %s", desc);

    auto node_type = std::string(ts_node_type(result[0]));

    if (node_type == "type_identifier" ||
        node_type == "field_identifier" ||
        node_type == "number_literal" ||
        node_type == "identifier" ||
        node_type == "system_lib_string" ||
        node_type == "namespace_identifier") {
      printf(" ");

      auto st = ts_node_start_byte(result[0]);
      auto end = ts_node_end_byte(result[0]);
      auto text = tbuff->text();

      text += st;

      for (int i = 0; i < end - st; i++)
        printf("%c", *text++);
    }

    printf("\n");

  } else {
    printf("No node here\n");
  }

  return result;
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
        getCurrentNode();
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
  if (nInserted == 0 && nDeleted == 0) return;

  if (nInserted > 0) {
    char *style = new char[nInserted + 1];
    memset(style, 'A', nInserted);
    style[nInserted] = '\0';
    sbuff->insert(pos, style);
    delete[] style;
  }

  if (nDeleted > 0) {
    sbuff->remove(pos, pos + nDeleted);
  }

  const char *source_code = tbuff->text();
  tree = ts_parser_parse_string(parser, nullptr, source_code, strlen(source_code));

  TSNode root_node = ts_tree_root_node(tree);
  char *string = ts_node_string(root_node);
  printf("Syntax tree: %s\n", string);
  free(string);

  if (nDeleted > 0) {
    return;
  }

  int start = pos;
  int end = pos + nInserted;

  for (int i = start; i < end; i++) {
    unsigned int c = tbuff->char_at(i);
    if (strchr("01234", static_cast<int>(c))) sbuff->replace(i, i + 1, "B");
    else if (strchr("56789", static_cast<int>(c))) sbuff->replace(i, i + 1, "C");
    else sbuff->replace(i, i + 1, "A");
  }
}

void Editor::load_font()
{
  loaded_font = i_load_private_font("/Users/izmar/git/editor/resources/SF-Mono-Regular.otf");

  if (loaded_font) {
    Fl::set_font(test_font, "SF Mono Regular");
  }
}
void Editor::cachePreprocessorDefinitions(TSNode n, std::string &text)
{
  addPreprocessorDefinitions(n, preprocessorDefinitions, text);
}
