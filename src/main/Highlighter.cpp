#include "Highlighter.h"

#include <FL/Fl_Text_Buffer.H>
#include <tree_sitter/api.h>

#include "IditorUtil.h"

void Highlighter::do_highlighting(std::string& text, Fl_Text_Buffer* style_buffer, TSParser* parser)
{
  std::vector<std::string> keywords{"extern", "auto", "sizeof", "switch", "case", "static", "const", "new", "for", "if",
                                    "else", "void", "int", "bool", "long", "float", "struct", "short", "unsigned",
                                    "class"};

  auto highlight = [&](TSNode n) {
    auto st = static_cast<int>(ts_node_start_byte(n));
    auto end = static_cast<int>(ts_node_end_byte(n));
    auto node_text = IditorUtil::getNodeText(n, text);

    if (std::find(keywords.begin(), keywords.end(), node_text) != keywords.end())
    {
      for (int i = st; i < end; i++)
      {
        style_buffer->replace(i, i + 1, "B");
      }
    }
    else
    {
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
      {
        style_buffer->replace(i, i + 1, style.c_str());
      }
    }
  };

  std::vector<TSNode> leaf_nodes;
  auto text_tree = ts_parser_parse_string(parser, nullptr, text.c_str(), strlen(text.c_str()));
  IditorUtil::collectLeafNodes(ts_tree_root_node(text_tree), leaf_nodes);

  for (int i = 0; i < text.size(); i++)
  {
    style_buffer->replace(i, i + 1, "A");
  }

  for (auto &n: leaf_nodes)
  {
    highlight(n);
  }
}
