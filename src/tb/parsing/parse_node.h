/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#ifndef TB_PARSING_PARSE_NODE_H_
#define TB_PARSING_PARSE_NODE_H_

#include "tb/types.h"
#include "tb/util/intrusive_list.h"
#include "tb/value.h"

namespace tb {
namespace parsing {

enum class ReadFlags {
  kNone = 0,
  // Read nodes without clearing first. Can be used to append data from multiple
  // sources, or inject dependencies.
  kAppend = 1,
};
MAKE_ENUM_FLAG_COMBO(ReadFlags);

// A tree node with a string name and a value (Value).
// It may have a parent ParseNode and child ParseNodes.
//
// Getting the value of this node or any child, may optionally follow references
// to nodes in any existing ParseNodeTree (by name).
//
// During ReadFile/ReadData, it may also select which branches to include or
// exclude conditionally by lookup up values in ParseNodeTree.
class ParseNode : public util::IntrusiveListEntry<ParseNode> {
 public:
  ParseNode() = default;
  ~ParseNode();

  // Creates a new node with the given name.
  static ParseNode* Create(const char* name);

  void TakeValue(Value& value);

  // Reads a tree of nodes from file into this node.
  // Returns true on success.
  bool ReadFile(const std::string& filename,
                ReadFlags flags = ReadFlags::kNone);

  // Reads a tree of nodes from a null terminated string buffer.
  void ReadData(const char* data, ReadFlags flags = ReadFlags::kNone);

  // Reads a tree of nodes from a buffer with a known length.
  void ReadData(const char* data, size_t data_length,
                ReadFlags flags = ReadFlags::kNone);

  // Clears the contents of this node.
  void Clear();

  // FIX: Add write support!
  // bool WriteFile(const char *filename);

  // Adds node as child to this node.
  void Add(ParseNode* n) {
    m_children.AddLast(n);
    n->m_parent = this;
  }

  // Adds node before the reference node (which must be a child to this node).
  void AddBefore(ParseNode* n, ParseNode* reference) {
    m_children.AddBefore(n, reference);
    n->m_parent = this;
  }

  // Adds node after the reference node (which must be a child to this node).
  void AddAfter(ParseNode* n, ParseNode* reference) {
    m_children.AddAfter(n, reference);
    n->m_parent = this;
  }

  // Removes child node n from this node.
  void Remove(ParseNode* n) {
    m_children.Remove(n);
    n->m_parent = nullptr;
  }

  // Removes and deletes child node n from this node.
  void Delete(ParseNode* n) { m_children.Delete(n); }

  // Creates duplicates of the source node and all child nodes.
  // NOTE: nodes do not replace existing nodes with the same name. Cloned nodes
  // are added after any existing nodes.
  void Clone(ParseNode* source);

  // Creates duplicates of all items in source and add them to this node.
  // NOTE: nodes do not replace existing nodes with the same name. Cloned nodes
  // are added after any existing nodes.
  void CloneChildren(ParseNode* source);

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
  ParseNode* GetNode(const char* request,
                     MissingPolicy mp = MissingPolicy::kNull);

  const char* GetName() const { return m_name; }

  Value& GetValue() { return m_value; }

  // Returns the value of this node.
  // Will follow eventual references to ParseNodeTree.
  Value& GetValueFollowRef();

  // Gets a value from the given request as an integer.
  // Will follow eventual references to ParseNodeTree.
  // If the value is not specified, it returns the default value (def).
  int GetValueInt(const char* request, int def);

  // Gets a value from the given request as an float.
  // Will follow eventual references to ParseNodeTree.
  // If the value is not specified, it returns the default value (def).
  float GetValueFloat(const char* request, float def);

  // Gets a value from the given request as an string.
  // Will follow eventual references to ParseNodeTree.
  // Will also return any referenced language string.
  // If the value is not specified, it returns the default value (def).
  const char* GetValueString(const char* request, const char* def);

  // Same as GetValueString, but won't look up language string references.
  const char* GetValueStringRaw(const char* request, const char* def);

  // Gets the next position in request that is a sub node separator, or the end
  // of the string.
  static const char* GetNextNodeSeparator(const char* request);

  inline ParseNode* GetParent() const { return m_parent; }
  inline ParseNode* GetFirstChild() const { return m_children.GetFirst(); }
  inline ParseNode* GetLastChild() const { return m_children.GetLast(); }

 private:
  friend class ParseNodeTarget;
  friend class ParseNodeTree;

  ParseNode* GetNodeFollowRef(const char* request,
                              MissingPolicy mp = MissingPolicy::kNull);
  ParseNode* GetNodeInternal(const char* name, size_t name_len) const;
  static ParseNode* Create(const char* name, size_t name_len);

  char* m_name = nullptr;
  Value m_value;
  util::AutoDeleteIntrusiveList<ParseNode> m_children;
  ParseNode* m_parent = nullptr;
  uint32_t m_cycle_id = 0;  // Used to detect circular references.
};

}  // namespace parsing
}  // namespace tb

#endif  // TB_PARSING_PARSE_NODE_H_
