/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#include "tb_node_ref_tree.h"

#include "tb_language.h"
#include "tb_system.h"

namespace tb {

// static
util::TBLinkListOf<NodeRefTree> NodeRefTree::s_ref_trees;

NodeRefTree::NodeRefTree(const char* name) : m_name(name), m_name_id(name) {
  s_ref_trees.AddLast(this);
}

NodeRefTree::~NodeRefTree() { s_ref_trees.Remove(this); }

Value& NodeRefTree::GetValue(const char* request) {
  if (Node* node = m_node.GetNodeFollowRef(request)) {
    return node->GetValue();
  }
  TBDebugOut("NodeRefTree::GetValue - request not found: %s\n", request);
  static Value nullval;
  return nullval;
}

// static
Value& NodeRefTree::GetValueFromTree(const char* request) {
  assert(*request == '@');
  Node tmp;
  tmp.GetValue().SetString(request, Value::Set::kAsStatic);
  Node* node = NodeRefTree::FollowNodeRef(&tmp);
  if (node != &tmp) {
    return node->GetValue();
  }
  static Value nullval;
  return nullval;
}

void NodeRefTree::SetValue(const char* request, const Value& value) {
  if (Node* node = m_node.GetNode(request, Node::MissingPolicy::kCreate)) {
    // FIX: Only invoke the listener if it really changed.
    node->GetValue().Copy(value);
    InvokeChangeListenersInternal(request);
  }
}

void NodeRefTree::InvokeChangeListenersInternal(const char* request) {
  auto iter = m_listeners.IterateForward();
  while (NodeRefTreeListener* listener = iter.GetAndStep()) {
    listener->OnDataChanged(this, request);
  }
}

// static
NodeRefTree* NodeRefTree::GetRefTree(const char* name, size_t name_len) {
  for (NodeRefTree* rt = s_ref_trees.GetFirst(); rt; rt = rt->GetNext()) {
    if (strncmp(rt->GetName().c_str(), name, name_len) == 0) {
      return rt;
    }
  }
  return nullptr;
}

// static
Node* NodeRefTree::FollowNodeRef(Node* node) {
  // Detect circular loops by letting this call get a unique id.
  // Update the id on each visited node and if it's already set,
  // there's a loop. This cost the storage of id in each Node,
  // and assumes the look up doesn't cause other lookups
  // recursively.
  // FIX: Switch to hare and teleporting tortouise?
  static uint32_t s_cycle_id = 0;
  uint32_t cycle_id = ++s_cycle_id;
  Node* start_node = node;

  while (node->GetValue().IsString()) {
    // If not a reference at all, we're done.
    const char* node_str = node->GetValue().GetString();
    if (*node_str != '@') {
      break;
    }

    // If there's no tree name and request, we're done. It's probably a language
    // string.
    const char* name_start = node_str + 1;
    const char* name_end = Node::GetNextNodeSeparator(name_start);
    if (*name_end == 0) {
      break;
    }

    Node* next_node = nullptr;

    // We have a "@>noderequest" string. Go ahead and do a local look up.
    if (*name_start == '>') {
      Node* local_root = node;
      while (local_root->GetParent()) {
        local_root = local_root->GetParent();
      }
      next_node =
          local_root->GetNode(name_start + 1, Node::MissingPolicy::kNull);
    }
    // We have a "@treename>noderequest" string. Go ahead and look it up from
    // the right node tree.
    else if (NodeRefTree* rt =
                 NodeRefTree::GetRefTree(name_start, name_end - name_start)) {
      next_node = rt->m_node.GetNode(name_end + 1, Node::MissingPolicy::kNull);
    } else {
      TBDebugOut(
          "NodeRefTree::ResolveNode - No tree found for request \"%s\" from "
          "node \"%s\"\n",
          node_str, node->GetValue().GetString());
      break;
    }

    if (!next_node) {
      TBDebugOut(
          "NodeRefTree::ResolveNode - Node not found on request \"%s\"\n",
          node_str);
      break;
    }
    node = next_node;

    // Detect circular reference loop.
    if (node->m_cycle_id != cycle_id) {
      node->m_cycle_id = cycle_id;
    } else {
      TBDebugOut(
          "NodeRefTree::ResolveNode - Reference loop detected on request "
          "\"%s\" from node \"%s\"\n",
          node_str, node->GetValue().GetString());
      return start_node;
    }
  }
  return node;
}

// static
void NodeRefTree::ResolveConditions(Node* parent_node) {
  bool condition_ret = false;
  Node* node = parent_node->GetFirstChild();
  while (node) {
    bool delete_node = false;
    bool move_children = false;
    if (strcmp(node->GetName(), "@if") == 0) {
      condition_ret = node->GetValueFollowRef().GetInt() ? true : false;
      if (condition_ret) move_children = true;
      delete_node = true;
    } else if (strcmp(node->GetName(), "@else") == 0) {
      condition_ret = !condition_ret;
      if (condition_ret) move_children = true;
      delete_node = true;
    }

    // Make sure we'll skip any nodes added from a conditional branch.
    Node* node_next = node->GetNext();

    if (move_children) {
      // Resolve the branch first, since we'll skip it.
      ResolveConditions(node);
      while (Node* content = node->GetLastChild()) {
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

}  // namespace tb
