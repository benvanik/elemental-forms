/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * Copyright 2015 Ben Vanik. All rights reserved. Licensed as BSD 3-clause.   *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include "el/parsing/parse_node_tree.h"
#include "el/util/debug.h"

namespace el {
namespace parsing {

// static
util::IntrusiveList<ParseNodeTree> ParseNodeTree::s_ref_trees;

ParseNodeTree::ParseNodeTree(const char* name) : m_name(name), m_name_id(name) {
  s_ref_trees.AddLast(this);
}

ParseNodeTree::~ParseNodeTree() { s_ref_trees.Remove(this); }

Value& ParseNodeTree::GetValue(const char* request) {
  if (ParseNode* node = m_node.GetNodeFollowRef(request)) {
    return node->value();
  }
  TBDebugOut("ParseNodeTree::GetValue - request not found: %s\n", request);
  static Value nullval;
  return nullval;
}

// static
Value& ParseNodeTree::GetValueFromTree(const char* request) {
  assert(*request == '@');
  ParseNode tmp;
  tmp.value().set_string(request, Value::Set::kAsStatic);
  ParseNode* node = ParseNodeTree::FollowNodeRef(&tmp);
  if (node != &tmp) {
    return node->value();
  }
  static Value nullval;
  return nullval;
}

void ParseNodeTree::SetValue(const char* request, const Value& value) {
  if (ParseNode* node =
          m_node.GetNode(request, ParseNode::MissingPolicy::kCreate)) {
    // FIX: Only invoke the listener if it really changed.
    node->value().Copy(value);
    InvokeChangeListenersInternal(request);
  }
}

void ParseNodeTree::InvokeChangeListenersInternal(const char* request) {
  auto iter = m_listeners.IterateForward();
  while (ParseNodeTreeListener* listener = iter.GetAndStep()) {
    listener->OnDataChanged(this, request);
  }
}

// static
ParseNodeTree* ParseNodeTree::GetRefTree(const char* name, size_t name_len) {
  for (ParseNodeTree* rt = s_ref_trees.GetFirst(); rt; rt = rt->GetNext()) {
    if (strncmp(rt->name().c_str(), name, name_len) == 0) {
      return rt;
    }
  }
  return nullptr;
}

// static
ParseNode* ParseNodeTree::FollowNodeRef(ParseNode* node) {
  // Detect circular loops by letting this call get a unique id.
  // Update the id on each visited node and if it's already set,
  // there's a loop. This cost the storage of id in each ParseNode,
  // and assumes the look up doesn't cause other lookups
  // recursively.
  // FIX: Switch to hare and teleporting tortouise?
  static uint32_t s_cycle_id = 0;
  uint32_t cycle_id = ++s_cycle_id;
  ParseNode* start_node = node;

  while (node->value().is_string()) {
    // If not a reference at all, we're done.
    const char* node_str = node->value().as_string();
    if (*node_str != '@') {
      break;
    }

    // If there's no tree name and request, we're done. It's probably a language
    // string.
    const char* name_start = node_str + 1;
    const char* name_end = ParseNode::GetNextNodeSeparator(name_start);
    if (*name_end == 0) {
      break;
    }

    ParseNode* next_node = nullptr;

    // We have a "@>noderequest" string. Go ahead and do a local look up.
    if (*name_start == '>') {
      ParseNode* local_root = node;
      while (local_root->parent()) {
        local_root = local_root->parent();
      }
      next_node =
          local_root->GetNode(name_start + 1, ParseNode::MissingPolicy::kNull);
    }
    // We have a "@treename>noderequest" string. Go ahead and look it up from
    // the right node tree.
    else if (ParseNodeTree* rt =
                 ParseNodeTree::GetRefTree(name_start, name_end - name_start)) {
      next_node =
          rt->m_node.GetNode(name_end + 1, ParseNode::MissingPolicy::kNull);
    } else {
      TBDebugOut(
          "ParseNodeTree::ResolveNode - No tree found for request \"%s\" from "
          "node \"%s\"\n",
          node_str, node->value().as_string());
      break;
    }

    if (!next_node) {
      TBDebugOut(
          "ParseNodeTree::ResolveNode - ParseNode not found on request "
          "\"%s\"\n",
          node_str);
      break;
    }
    node = next_node;

    // Detect circular reference loop.
    if (node->m_cycle_id != cycle_id) {
      node->m_cycle_id = cycle_id;
    } else {
      TBDebugOut(
          "ParseNodeTree::ResolveNode - Reference loop detected on request "
          "\"%s\" from node \"%s\"\n",
          node_str, node->value().as_string());
      return start_node;
    }
  }
  return node;
}

// static
void ParseNodeTree::ResolveConditions(ParseNode* parent_node) {
  bool condition_ret = false;
  ParseNode* node = parent_node->first_child();
  while (node) {
    bool delete_node = false;
    bool move_children = false;
    if (strcmp(node->name(), "@if") == 0) {
      condition_ret = node->GetValueFollowRef().as_integer() ? true : false;
      if (condition_ret) move_children = true;
      delete_node = true;
    } else if (strcmp(node->name(), "@else") == 0) {
      condition_ret = !condition_ret;
      if (condition_ret) move_children = true;
      delete_node = true;
    }

    // Make sure we'll skip any nodes added from a conditional branch.
    ParseNode* node_next = node->GetNext();

    if (move_children) {
      // Resolve the branch first, since we'll skip it.
      ResolveConditions(node);
      while (ParseNode* content = node->last_child()) {
        node->Remove(content);
        parent_node->AddAfter(content, node);
      }
    }

    if (delete_node) {
      parent_node->Delete(node);
    } else {
      ResolveConditions(node);
    }
    node = node_next;
  }
}

}  // namespace parsing
}  // namespace el
