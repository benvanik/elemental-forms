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
#include "tb_system.h"

namespace tb {

TBLanguage::~TBLanguage() { Clear(); }

bool TBLanguage::Load(const char* filename) {
  // Read the file into a node tree (even though it's only a flat list)
  TBNode node;
  if (!node.ReadFile(filename)) return false;

  // Go through all nodes and add to the strings hash table
  TBNode* n = node.GetFirstChild();
  while (n) {
    const char* str = n->GetValue().GetString();
    std::string* new_str = new std::string(str);
    strings.Add(TBID(n->GetName()), new_str);
    n = n->GetNext();
  }
  return true;
}

void TBLanguage::Clear() { strings.DeleteAll(); }

const char* TBLanguage::GetString(const TBID& id) {
  if (std::string* str = strings.Get(id)) {
    return str->c_str();
  }
#ifdef TB_RUNTIME_DEBUG_INFO
  static std::string tmp;
  tmp = tb::format_string("<TRANSLATE:%s>", id.debug_string.c_str());
  return tmp.c_str();
#else
  return "<TRANSLATE!>";
#endif
}

}  // namespace tb
