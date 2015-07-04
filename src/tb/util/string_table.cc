/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#include "tb_node_tree.h"

#include "tb/util/string.h"
#include "tb/util/string_table.h"

namespace tb {
namespace util {

std::unique_ptr<StringTable> StringTable::string_table_singleton_;

bool StringTable::Load(const char* filename) {
  // Read the file into a node tree (even though it's only a flat list).
  Node node;
  if (!node.ReadFile(filename)) {
    return false;
  }

  // Go through all nodes and add to the strings hash table.
  Node* n = node.GetFirstChild();
  while (n) {
    const char* str = n->GetValue().as_string();
    table_.emplace(TBID(n->GetName()), str);
    n = n->GetNext();
  }
  return true;
}

void StringTable::Clear() { table_.clear(); }

std::string StringTable::GetString(const TBID& id) {
  auto& it = table_.find(id);
  if (it != table_.end()) {
    return it->second;
  }
#ifdef TB_RUNTIME_DEBUG_INFO
  return tb::util::format_string("<TRANSLATE:%s>", id.debug_string.c_str());
#else
  return "<TRANSLATE!>";
#endif  // TB_RUNTIME_DEBUG_INFO
}

}  // namespace util
}  // namespace tb
