/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * Copyright 2015 Ben Vanik. All rights reserved. Licensed as BSD 3-clause.   *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include "el/element.h"
#include "el/parsing/element_factory.h"
#include "el/parsing/element_inflater.h"
#include "el/parsing/parse_node.h"

namespace el {
namespace parsing {

std::unique_ptr<ElementFactory> ElementFactory::element_reader_singleton_;

ElementFactory::ElementFactory() = default;
ElementFactory::~ElementFactory() = default;

void ElementFactory::RegisterInflater(
    std::unique_ptr<ElementInflater> inflater) {
  inflaters_.emplace_back(std::move(inflater));
}

bool ElementFactory::LoadFile(Element* target, const char* filename) {
  ParseNode node;
  if (!node.ReadFile(filename)) {
    return false;
  }
  LoadNodeTree(target, &node);
  return true;
}

bool ElementFactory::LoadData(Element* target, const char* data,
                              size_t data_length) {
  ParseNode node;
  node.ReadData(data, data_length);
  LoadNodeTree(target, &node);
  return true;
}

void ElementFactory::LoadNodeTree(Element* target, ParseNode* node) {
  // Iterate through all nodes and create elements.
  for (ParseNode* child = node->first_child(); child;
       child = child->GetNext()) {
    CreateElement(target, child);
  }
}

bool ElementFactory::CreateElement(Element* target, ParseNode* node) {
  // Find a element creator from the node name.
  ElementInflater* source_factory = nullptr;
  for (auto& inflater : inflaters_) {
    if (std::strcmp(inflater->name(), node->name()) == 0) {
      source_factory = inflater.get();
      break;
    }
  }
  if (!source_factory) {
    return false;
  }

  // Create the element.
  InflateInfo info(this, target->content_root(), node,
                   source_factory->sync_type());
  Element* new_element = source_factory->Create(&info);
  if (!new_element) {
    return false;
  }

  // Read properties and add i to the hierarchy.
  new_element->OnInflate(info);

  // If this assert is trigged, you probably forgot to call Element::OnInflate
  // from an overridden version.
  assert(new_element->parent());

  // Iterate through all nodes and create elements.
  for (ParseNode* n = node->first_child(); n; n = n->GetNext()) {
    CreateElement(new_element, n);
  }

  if (node->GetValueInt("autofocus", 0)) {
    new_element->set_focus(FocusReason::kUnknown);
  }

  return true;
}

}  // namespace parsing
}  // namespace el
