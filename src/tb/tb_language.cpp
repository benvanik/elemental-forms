/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#include "tb_node_tree.h"
#include "tb_language.h"

#include "tb/util/string.h"

namespace tb {

bool Language::Load(const char* filename) {
  // Read the file into a node tree (even though it's only a flat list).
  Node node;
  if (!node.ReadFile(filename)) {
    return false;
  }

  // Go through all nodes and add to the strings hash table.
  Node* n = node.GetFirstChild();
  while (n) {
    const char* str = n->GetValue().GetString();
    table_.emplace(TBID(n->GetName()), str);
    n = n->GetNext();
  }
  return true;
}

void Language::Clear() { table_.clear(); }

std::string Language::GetString(const TBID& id) {
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

}  // namespace tb
