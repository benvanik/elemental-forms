/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#ifndef TB_NODE_TREE_H
#define TB_NODE_TREE_H

#include "tb_linklist.h"
#include "tb_parser.h"

namespace tb {

enum class ReadFlags {
  kNone = 0,
  // Read nodes without clearing first. Can be used to append data from multiple
  // sources, or inject dependencies.
  kAppend = 1,
};
MAKE_ENUM_FLAG_COMBO(ReadFlags);

// A tree node with a string name and a value (Value).
// It may have a parent Node and child Nodes.
//
// Getting the value of this node or any child, may optionally follow references
// to nodes in any existing NodeRefTree (by name).
//
// During ReadFile/ReadData, it may also select which branches to include or
// exclude conditionally by lookup up values in NodeRefTree.
class Node : public TBLinkOf<Node> {
 public:
  Node() = default;
  ~Node();

  // Creates a new node with the given name.
  static Node* Create(const char* name);

  void TakeValue(Value& value);

  // Reads a tree of nodes from file into this node.
  // Returns true on success.
  bool ReadFile(const std::string& filename,
                ReadFlags flags = ReadFlags::kNone);

  // Reads a tree of nodes from a null terminated string buffer.
  void ReadData(const char* data, ReadFlags flags = ReadFlags::kNone);

  // Reads a tree of nodes from a buffer with a known length.
  void ReadData(const char* data, size_t data_len,
                ReadFlags flags = ReadFlags::kNone);

  // Clears the contents of this node.
  void Clear();

  // FIX: Add write support!
  // bool WriteFile(const char *filename);

  // Adds node as child to this node.
  void Add(Node* n) {
    m_children.AddLast(n);
    n->m_parent = this;
  }

  // Adds node before the reference node (which must be a child to this node).
  void AddBefore(Node* n, Node* reference) {
    m_children.AddBefore(n, reference);
    n->m_parent = this;
  }

  // Adds node after the reference node (which must be a child to this node).
  void AddAfter(Node* n, Node* reference) {
    m_children.AddAfter(n, reference);
    n->m_parent = this;
  }

  // Removes child node n from this node.
  void Remove(Node* n) {
    m_children.Remove(n);
    n->m_parent = nullptr;
  }

  // Removes and deletes child node n from this node.
  void Delete(Node* n) { m_children.Delete(n); }

  // Creates duplicates of all items in source and add them to this node.
  // NOTE: nodes do not replace existing nodes with the same name. Cloned nodes
  // are added after any existing nodes.
  bool CloneChildren(Node* source);

  enum class MissingPolicy {
    // GetNode will return nullptr if the node doesn't exist.
    kNull,
    // GetNode will create all missing nodes for the request.
    kCreate,
  };

  // Gets a node from the given request.
  // If the node doesn't exist, it will either return nullptr or create missing
  // nodes, depending on the miss policy. It can find nodes in children as well.
  // Names are separated by a ">".
  // Ex: GetNode("dishes>pizza>special>batman")
  Node* GetNode(const char* request, MissingPolicy mp = MissingPolicy::kNull);

  const char* GetName() const { return m_name; }

  Value& GetValue() { return m_value; }

  // Returns the value of this node.
  // Will follow eventual references to NodeRefTree.
  Value& GetValueFollowRef();

  // Gets a value from the given request as an integer.
  // Will follow eventual references to NodeRefTree.
  // If the value is not specified, it returns the default value (def).
  int GetValueInt(const char* request, int def);

  // Gets a value from the given request as an float.
  // Will follow eventual references to NodeRefTree.
  // If the value is not specified, it returns the default value (def).
  float GetValueFloat(const char* request, float def);

  // Gets a value from the given request as an string.
  // Will follow eventual references to NodeRefTree.
  // Will also return any referenced language string.
  // If the value is not specified, it returns the default value (def).
  const char* GetValueString(const char* request, const char* def);

  // Same as GetValueString, but won't look up language string references.
  const char* GetValueStringRaw(const char* request, const char* def);

  // Gets the next position in request that is a sub node separator, or the end
  // of the string.
  static const char* GetNextNodeSeparator(const char* request);

  inline Node* GetParent() const { return m_parent; }
  inline Node* GetFirstChild() const { return m_children.GetFirst(); }
  inline Node* GetLastChild() const { return m_children.GetLast(); }

 private:
  friend class NodeTarget;
  friend class NodeRefTree;

  Node* GetNodeFollowRef(const char* request,
                         MissingPolicy mp = MissingPolicy::kNull);
  Node* GetNodeInternal(const char* name, size_t name_len) const;
  static Node* Create(const char* name, size_t name_len);

  char* m_name = nullptr;
  Value m_value;
  TBLinkListOf<Node> m_children;
  Node* m_parent = nullptr;
  uint32_t m_cycle_id = 0;  // Used to detect circular references.
};

}  // namespace tb

#endif  // TB_NODE_TREE_H
