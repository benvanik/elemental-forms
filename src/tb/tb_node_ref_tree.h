/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#ifndef TB_NODE_REF_TREE_H
#define TB_NODE_REF_TREE_H

#include "tb_id.h"
#include "tb_node_tree.h"

#include "tb/util/link_list.h"

namespace tb {

class Node;
class NodeRefTreeListener;

// A named Node.
// Nodes under this node may be referenced from other nodes, either when
// requesting a value (Node::GetValueFollowRef), or while parsing the node tree.
// While parsing, the values can be used for branch conditions or branches of
// nodes can be included.
class NodeRefTree : public util::TBLinkOf<NodeRefTree> {
 public:
  NodeRefTree(const char* name);
  virtual ~NodeRefTree();

  const std::string& GetName() const { return m_name; }
  const TBID& GetNameID() const { return m_name_id; }

  // Reads the data file. This will *not* invoke any change listener!
  bool ReadFile(const char* filename) { return m_node.ReadFile(filename); }
  void ReadData(const char* data) { m_node.ReadData(data); }

  // Adds a listener that is invoked on changes in this tree.
  void AddListener(NodeRefTreeListener* listener) {
    m_listeners.AddLast(listener);
  }

  // Removes a change listener from this tree.
  void RemoveListener(NodeRefTreeListener* listener) {
    m_listeners.Remove(listener);
  }

  // Sets the value for the given request and invoke the change listener.
  // Creates the nodes that doesn't exist.
  virtual void SetValue(const char* request, const Value& value);

  // Gets the value of the given request. Follows references if any.
  // Returns a null value if the request doesn't exist.
  virtual Value& GetValue(const char* request);

  // Gets the value of the given tree name and request (@treename>noderequest).
  // Returns a null value if the given tree or request doesn't exist.
  static Value& GetValueFromTree(const char* request);

  // Returns the tree with the given name, or nullptr if no matching tree
  // exists.
  static NodeRefTree* GetRefTree(const char* name, size_t name_len);

  /** Go through the tree of nodes recursively and include
          or remove branches depending on any conditions. */
  static void ResolveConditions(Node* parent_node);

 private:
  friend class Node;
  friend class NodeTarget;

  // Follows any references to data trees and return the destination node.
  // If there's broken references, the node will be returned.
  static Node* FollowNodeRef(Node* node);

  void InvokeChangeListenersInternal(const char* request);

  Node m_node;
  std::string m_name;
  TBID m_name_id;
  util::TBLinkListOf<NodeRefTreeListener> m_listeners;
  static util::TBLinkListOf<NodeRefTree> s_ref_trees;
};

// Receives OnDataChanged when the value of a node in a NodeRefTree is changed.
// FIX: The listener can currently only listen to one tree.
class NodeRefTreeListener : public util::TBLinkOf<NodeRefTreeListener> {
 public:
  // Called when the value is changed for the given node in the given ref tree.
  // The request is without tree name.
  virtual void OnDataChanged(NodeRefTree* rt, const char* request) = 0;
};

}  // namespace tb

#endif  // TB_NODE_REF_TREE_H
