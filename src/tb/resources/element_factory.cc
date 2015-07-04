/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#include "tb/resources/element_factory.h"

#include "tb_node_tree.h"
#include "tb_widgets.h"

namespace tb {
namespace resources {

std::unique_ptr<ElementFactory> ElementFactory::element_reader_singleton_;

void ElementFactory::RegisterInflater(
    std::unique_ptr<ElementInflater> inflater) {
  inflaters_.emplace_back(std::move(inflater));
}

bool ElementFactory::LoadFile(Element* target, const char* filename) {
  Node node;
  if (!node.ReadFile(filename)) {
    return false;
  }
  LoadNodeTree(target, &node);
  return true;
}

bool ElementFactory::LoadData(Element* target, const char* data,
                              size_t data_length) {
  Node node;
  node.ReadData(data, data_length);
  LoadNodeTree(target, &node);
  return true;
}

void ElementFactory::LoadNodeTree(Element* target, Node* node) {
  // Iterate through all nodes and create elements.
  for (Node* child = node->GetFirstChild(); child; child = child->GetNext()) {
    CreateElement(target, child);
  }
}

bool ElementFactory::CreateElement(Element* target, Node* node) {
  // Find a element creator from the node name.
  ElementInflater* source_factory = nullptr;
  for (auto& inflater : inflaters_) {
    if (std::strcmp(inflater->name(), node->GetName()) == 0) {
      source_factory = inflater.get();
      break;
    }
  }
  if (!source_factory) {
    return false;
  }

  // Create the element.
  InflateInfo info(this, target->GetContentRoot(), node,
                   source_factory->sync_type());
  Element* new_element = source_factory->Create(&info);
  if (!new_element) {
    return false;
  }

  // Read properties and add i to the hierarchy.
  new_element->OnInflate(info);

  // If this assert is trigged, you probably forgot to call Element::OnInflate
  // from an overridden version.
  assert(new_element->GetParent());

  // Iterate through all nodes and create elements.
  for (Node* n = node->GetFirstChild(); n; n = n->GetNext()) {
    CreateElement(new_element, n);
  }

  if (node->GetValueInt("autofocus", 0)) {
    new_element->SetFocus(FocusReason::kUnknown);
  }

  return true;
}

}  // namespace resources
}  // namespace tb
