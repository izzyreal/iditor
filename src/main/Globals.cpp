#include "Globals.h"

std::set<std::string> Globals::includeDirectories;
std::map<std::string, std::string> Globals::definitions;
std::map<std::string, Macro> Globals::macros;

const char* Globals::highlight_query = "; Functions\n"
                                                   "\n"
                                                   "(call_expression\n"
                                                   "  function: (qualified_identifier\n"
                                                   "    name: (identifier) @function))\n"
                                                   "\n"
                                                   "(template_function\n"
                                                   "  name: (identifier) @function)\n"
                                                   "\n"
                                                   "(template_method\n"
                                                   "  name: (field_identifier) @function)\n"
                                                   "\n"
                                                   "(template_function\n"
                                                   "  name: (identifier) @function)\n"
                                                   "\n"
                                                   "(function_declarator\n"
                                                   "  declarator: (qualified_identifier\n"
                                                   "    name: (identifier) @function))\n"
                                                   "\n"
                                                   "(function_declarator\n"
                                                   "  declarator: (qualified_identifier\n"
                                                   "    name: (identifier) @function))\n"
                                                   "\n"
                                                   "(function_declarator\n"
                                                   "  declarator: (field_identifier) @function)\n"
                                                   "\n"
                                                   "; Types\n"
                                                   "\n"
                                                   "((namespace_identifier) @type\n"
                                                   " (#match? @type \"^[A-Z]\"))\n"
                                                   "\n"
                                                   "(auto) @type\n"
                                                   "\n"
                                                   "; Constants\n"
                                                   "\n"
                                                   "(this) @variable.builtin\n"
                                                   "(nullptr) @constant\n"
                                                   "\n"
                                                   "; Keywords\n"
                                                   "\n"
                                                   "[\n"
                                                   " \"catch\"\n"
                                                   " \"class\"\n"
                                                   " \"co_await\"\n"
                                                   " \"co_return\"\n"
                                                   " \"co_yield\"\n"
                                                   " \"constexpr\"\n"
                                                   " \"constinit\"\n"
                                                   " \"consteval\"\n"
                                                   " \"delete\"\n"
                                                   " \"explicit\"\n"
                                                   " \"final\"\n"
                                                   " \"friend\"\n"
                                                   " \"mutable\"\n"
                                                   " \"namespace\"\n"
                                                   " \"noexcept\"\n"
                                                   " \"new\"\n"
                                                   " \"override\"\n"
                                                   " \"private\"\n"
                                                   " \"protected\"\n"
                                                   " \"public\"\n"
                                                   " \"template\"\n"
                                                   " \"throw\"\n"
                                                   " \"try\"\n"
                                                   " \"typename\"\n"
                                                   " \"using\"\n"
                                                   " \"virtual\"\n"
                                                   " \"concept\"\n"
                                                   " \"requires\"\n"
                                                   "] @keyword\n"
                                                   "\n"
                                                   "; Strings\n"
                                                   "\n"
                                                   "(raw_string_literal) @string";