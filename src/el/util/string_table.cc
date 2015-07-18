/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include "el/parsing/parse_node.h"
#include "el/util/string.h"
#include "el/util/string_table.h"

namespace el {
namespace util {

std::unique_ptr<StringTable> StringTable::string_table_singleton_;

bool StringTable::Load(const char* filename) {
  // Read the file into a node tree (even though it's only a flat list).
  parsing::ParseNode node;
  if (!node.ReadFile(filename)) {
    return false;
  }

  // Go through all nodes and add to the strings hash table.
  auto n = node.first_child();
  while (n) {
    const char* str = n->value().as_string();
    table_.emplace(TBID(n->name()), str);
    n = n->GetNext();
  }
  return true;
}

void StringTable::Clear() { table_.clear(); }

std::string StringTable::GetString(const TBID& id) {
  auto it = table_.find(id);
  if (it != table_.end()) {
    return it->second;
  }
#ifdef EL_RUNTIME_DEBUG_INFO
  return el::util::format_string("<TRANSLATE:%s>", id.debug_string.c_str());
#else
  return "<TRANSLATE!>";
#endif  // EL_RUNTIME_DEBUG_INFO
}

}  // namespace util
}  // namespace el
