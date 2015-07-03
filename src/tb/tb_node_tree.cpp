/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#include "tb_node_tree.h"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <string>

#include "tb_node_ref_tree.h"
#include "tb_system.h"
#include "tb_string_builder.h"
#include "tb_language.h"

#include "tb/util/string.h"

namespace tb {

Node::~Node() { Clear(); }

// static
Node* Node::Create(const char* name) {
  Node* n = new Node();
  n->m_name = strdup(name);
  return n;
}

// static
Node* Node::Create(const char* name, size_t name_len) {
  Node* n = new Node();
  n->m_name = (char*)malloc(name_len + 1);
  std::memcpy(n->m_name, name, name_len);
  n->m_name[name_len] = 0;
  return n;
}

// static
const char* Node::GetNextNodeSeparator(const char* request) {
  while (*request != 0 && *request != '>') {
    request++;
  }
  return request;
}

Node* Node::GetNode(const char* request, MissingPolicy mp) {
  // Iterate one node deeper for each sub request (non recursive).
  Node* n = this;
  while (*request && n) {
    const char* nextend = GetNextNodeSeparator(request);
    size_t name_len = nextend - request;
    Node* n_child = n->GetNodeInternal(request, name_len);
    if (!n_child && mp == MissingPolicy::kCreate) {
      n_child = n->Create(request, name_len);
      n->Add(n_child);
    }
    n = n_child;
    request = *nextend == 0 ? nextend : nextend + 1;
  }
  return n;
}

Node* Node::GetNodeFollowRef(const char* request, MissingPolicy mp) {
  Node* node = GetNode(request, mp);
  if (node) {
    node = NodeRefTree::FollowNodeRef(node);
  }
  return node;
}

Node* Node::GetNodeInternal(const char* name, size_t name_len) const {
  for (Node* n = GetFirstChild(); n; n = n->GetNext()) {
    if (strncmp(n->m_name, name, name_len) == 0 && n->m_name[name_len] == 0) {
      return n;
    }
  }
  return nullptr;
}

void Node::Clone(Node* source) {
  Node* new_child = Create(source->m_name);
  new_child->m_value.Copy(source->m_value);
  Add(new_child);
  new_child->CloneChildren(source);
}

void Node::CloneChildren(Node* source) {
  Node* item = source->GetFirstChild();
  while (item) {
    Node* new_child = Create(item->m_name);
    new_child->m_value.Copy(item->m_value);
    Add(new_child);
    new_child->CloneChildren(item);
    item = item->GetNext();
  }
}

Value& Node::GetValueFollowRef() {
  return NodeRefTree::FollowNodeRef(this)->GetValue();
}

int Node::GetValueInt(const char* request, int def) {
  Node* n = GetNodeFollowRef(request);
  return n ? n->m_value.GetInt() : def;
}

float Node::GetValueFloat(const char* request, float def) {
  Node* n = GetNodeFollowRef(request);
  return n ? n->m_value.GetFloat() : def;
}

const char* Node::GetValueString(const char* request, const char* def) {
  if (Node* node = GetNodeFollowRef(request)) {
    // We might have a language string. Those are not
    // looked up in GetNode/ResolveNode.
    if (node->GetValue().IsString()) {
      const char* string = node->GetValue().GetString();
      if (*string == '@' && *Node::GetNextNodeSeparator(string) == 0) {
        // TODO(benvanik): replace this with something better (std::string all
        // around?). This is nasty and will break a great many things.
        static std::string temp;
        temp = g_tb_lng->GetString(string + 1);
        string = temp.c_str();
      }
      return string;
    }
    return node->GetValue().GetString();
  }
  return def;
}

const char* Node::GetValueStringRaw(const char* request, const char* def) {
  Node* n = GetNodeFollowRef(request);
  return n ? n->m_value.GetString() : def;
}

class FileParserStream : public ParserStream {
 public:
  bool Read(const std::string& filename, ParserTarget* target) {
    f = TBFile::Open(filename, TBFile::Mode::kRead);
    if (!f) {
      return false;
    }
    Parser p;
    auto status = p.Read(this, target);
    delete f;
    return status == Parser::Status::kOk ? true : false;
  }

  size_t GetMoreData(char* buf, size_t buf_len) override {
    return f->Read(buf, 1, buf_len);
  }

 private:
  TBFile* f = nullptr;
};

class DataParserStream : public ParserStream {
 public:
  bool Read(const char* data, size_t data_len, ParserTarget* target) {
    m_data = data;
    m_data_len = data_len;
    Parser p;
    auto status = p.Read(this, target);
    return status == Parser::Status::kOk ? true : false;
  }
  size_t GetMoreData(char* buf, size_t buf_len) override {
    size_t consume = std::min(buf_len, m_data_len);
    memcpy(buf, m_data, consume);
    m_data += consume;
    m_data_len -= consume;
    return consume;
  }

 private:
  const char* m_data = nullptr;
  size_t m_data_len = 0;
};

class NodeTarget : public ParserTarget {
 public:
  NodeTarget(Node* root, const std::string& filename)
      : m_root_node(root), m_target_node(root), m_filename(filename) {}
  void OnError(int line_nr, const std::string& error) override {
#ifdef TB_RUNTIME_DEBUG_INFO
    TBDebugOut("%s(%d):Parse error: %s\n", m_filename, line_nr, error.c_str());
#endif  // TB_RUNTIME_DEBUG_INFO
  }
  void OnComment(int line_nr, const char* comment) override {}
  void OnToken(int line_nr, const char* name, Value& value) override {
    if (!m_target_node) return;
    if (strcmp(name, "@file") == 0) {
      IncludeFile(line_nr, value.GetString());
    } else if (strcmp(name, "@include") == 0) {
      IncludeRef(line_nr, value.GetString());
    } else {
      Node* n = Node::Create(name);
      n->TakeValue(value);
      m_target_node->Add(n);
    }
  }
  void Enter() override {
    if (m_target_node) {
      m_target_node = m_target_node->GetLastChild();
    }
  }
  void Leave() override {
    assert(m_target_node != m_root_node);
    if (m_target_node) {
      m_target_node = m_target_node->m_parent;
    }
  }
  void IncludeFile(int line_nr, const char* filename) {
    // Read the included file into a new Node and then move all the children to
    // m_target_node.
    StringBuilder include_filename;
    include_filename.AppendPath(m_filename);
    include_filename.AppendString(filename);
    Node content;
    if (content.ReadFile(include_filename.GetData())) {
      while (Node* content_n = content.GetFirstChild()) {
        content.Remove(content_n);
        m_target_node->Add(content_n);
      }
    } else {
      OnError(line_nr,
              tb::util::format_string("Referenced file \"%s\" was not found!",
                                      include_filename.GetData()));
    }
  }
  void IncludeRef(int line_nr, const char* refstr) {
    Node* refnode = nullptr;
    if (*refstr == '@') {
      Node tmp;
      tmp.GetValue().SetString(refstr, Value::Set::kAsStatic);
      refnode = NodeRefTree::FollowNodeRef(&tmp);
    } else {
      // Local look-up.
      // NOTE: If we read to a target node that already contains
      //       nodes, we might look up nodes that's already there
      //       instead of new nodes.
      refnode = m_root_node->GetNode(refstr, Node::MissingPolicy::kNull);

      // Detect cycles.
      Node* cycle_detection = m_target_node;
      while (cycle_detection && refnode) {
        if (cycle_detection == refnode) {
          refnode = nullptr;  // We have a cycle, so just fail the inclusion.
        }
        cycle_detection = cycle_detection->GetParent();
      }
    }
    if (refnode) {
      m_target_node->CloneChildren(refnode);
    } else {
      OnError(line_nr,
              tb::util::format_string("Include \"%s\" was not found!", refstr));
    }
  }

 private:
  Node* m_root_node;
  Node* m_target_node;
  std::string m_filename;
};

void Node::TakeValue(Value& value) { m_value.TakeOver(value); }

bool Node::ReadFile(const std::string& filename, ReadFlags flags) {
  if (!any(flags & ReadFlags::kAppend)) {
    Clear();
  }
  FileParserStream p;
  NodeTarget t(this, filename);
  if (p.Read(filename, &t)) {
    NodeRefTree::ResolveConditions(this);
    return true;
  }
  return false;
}

void Node::ReadData(const char* data, ReadFlags flags) {
  ReadData(data, strlen(data), flags);
}

void Node::ReadData(const char* data, size_t data_len, ReadFlags flags) {
  if (!any(flags & ReadFlags::kAppend)) {
    Clear();
  }
  DataParserStream p;
  NodeTarget t(this, "{data}");
  p.Read(data, data_len, &t);
  NodeRefTree::ResolveConditions(this);
}

void Node::Clear() {
  free(m_name);
  m_name = nullptr;
  m_children.DeleteAll();
}

}  // namespace tb
