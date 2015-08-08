/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * Copyright 2015 Ben Vanik. All rights reserved. Licensed as BSD 3-clause.   *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <memory>
#include <string>

#include "el/parsing/parse_node.h"
#include "el/parsing/parse_node_tree.h"
#include "el/parsing/text_parser.h"
#include "el/parsing/text_parser_stream.h"
#include "el/util/debug.h"
#include "el/util/string.h"
#include "el/util/string_builder.h"
#include "el/util/string_table.h"

namespace el {
namespace parsing {

ParseNode::~ParseNode() { Clear(); }

// static
ParseNode* ParseNode::Create(const char* name) {
  ParseNode* n = new ParseNode();
  n->m_name = strdup(name);
  return n;
}

// static
ParseNode* ParseNode::Create(const char* name, size_t name_len) {
  ParseNode* n = new ParseNode();
  n->m_name = (char*)malloc(name_len + 1);
  std::memcpy(n->m_name, name, name_len);
  n->m_name[name_len] = 0;
  return n;
}

// static
const char* ParseNode::GetNextNodeSeparator(const char* request) {
  while (*request != 0 && *request != '>') {
    request++;
  }
  return request;
}

ParseNode* ParseNode::GetNode(const char* request, MissingPolicy mp) {
  // Iterate one node deeper for each sub request (non recursive).
  ParseNode* n = this;
  while (*request && n) {
    const char* nextend = GetNextNodeSeparator(request);
    size_t name_len = nextend - request;
    ParseNode* n_child = n->GetNodeInternal(request, name_len);
    if (!n_child && mp == MissingPolicy::kCreate) {
      n_child = n->Create(request, name_len);
      n->Add(n_child);
    }
    n = n_child;
    request = *nextend == 0 ? nextend : nextend + 1;
  }
  return n;
}

ParseNode* ParseNode::GetNodeFollowRef(const char* request, MissingPolicy mp) {
  ParseNode* node = GetNode(request, mp);
  if (node) {
    node = ParseNodeTree::FollowNodeRef(node);
  }
  return node;
}

ParseNode* ParseNode::GetNodeInternal(const char* name, size_t name_len) const {
  for (ParseNode* n = first_child(); n; n = n->GetNext()) {
    if (strncmp(n->m_name, name, name_len) == 0 && n->m_name[name_len] == 0) {
      return n;
    }
  }
  return nullptr;
}

void ParseNode::Clone(ParseNode* source) {
  ParseNode* new_child = Create(source->m_name);
  new_child->m_value.Copy(source->m_value);
  Add(new_child);
  new_child->CloneChildren(source);
}

void ParseNode::CloneChildren(ParseNode* source) {
  ParseNode* item = source->first_child();
  while (item) {
    ParseNode* new_child = Create(item->m_name);
    new_child->m_value.Copy(item->m_value);
    Add(new_child);
    new_child->CloneChildren(item);
    item = item->GetNext();
  }
}

Value& ParseNode::GetValueFollowRef() {
  return ParseNodeTree::FollowNodeRef(this)->value();
}

int ParseNode::GetValueInt(const char* request, int def) {
  ParseNode* n = GetNodeFollowRef(request);
  return n ? n->m_value.as_integer() : def;
}

float ParseNode::GetValueFloat(const char* request, float def) {
  ParseNode* n = GetNodeFollowRef(request);
  return n ? n->m_value.as_float() : def;
}

const char* ParseNode::GetValueString(const char* request, const char* def) {
  if (ParseNode* node = GetNodeFollowRef(request)) {
    // We might have a language string. Those are not
    // looked up in GetNode/ResolveNode.
    if (node->value().is_string()) {
      const char* string = node->value().as_string();
      if (*string == '@' && *ParseNode::GetNextNodeSeparator(string) == 0) {
        // TODO(benvanik): replace this with something better (std::string all
        // around?). This is nasty and will break a great many things.
        static std::string temp;
        temp = util::GetString(string + 1);
        string = temp.c_str();
      }
      return string;
    }
    return node->value().as_string();
  }
  return def;
}

const char* ParseNode::GetValueStringRaw(const char* request, const char* def) {
  ParseNode* n = GetNodeFollowRef(request);
  return n ? n->m_value.as_string() : def;
}

class ParseNodeTarget : public TextParserTarget {
 public:
  ParseNodeTarget(ParseNode* root, const std::string& filename)
      : m_root_node(root), m_target_node(root), m_filename(filename) {}
  void OnError(int line_nr, const std::string& error) override {
    TBDebugOut("%s(%d):Parse error: %s\n", m_filename.c_str(), line_nr,
               error.c_str());
  }
  void OnComment(int line_nr, const char* comment) override {}
  void OnToken(int line_nr, const char* name, Value& value) override {
    if (!m_target_node) return;
    if (strcmp(name, "@file") == 0) {
      IncludeFile(line_nr, value.as_string());
    } else if (strcmp(name, "@include") == 0) {
      IncludeRef(line_nr, value.as_string());
    } else {
      ParseNode* n = ParseNode::Create(name);
      n->TakeValue(value);
      m_target_node->Add(n);
    }
  }
  void Enter() override {
    if (m_target_node) {
      m_target_node = m_target_node->last_child();
    }
  }
  void Leave() override {
    assert(m_target_node != m_root_node);
    if (m_target_node) {
      m_target_node = m_target_node->m_parent;
    }
  }
  void IncludeFile(int line_nr, const char* filename) {
    // Read the included file into a new ParseNode and then move all the
    // children to
    // m_target_node.
    util::StringBuilder include_filename;
    include_filename.AppendPath(m_filename);
    include_filename.AppendString(filename);
    ParseNode content;
    if (content.ReadFile(include_filename.c_str())) {
      while (ParseNode* content_n = content.first_child()) {
        content.Remove(content_n);
        m_target_node->Add(content_n);
      }
    } else {
      OnError(line_nr,
              el::util::format_string("Referenced file \"%s\" was not found!",
                                      include_filename.c_str()));
    }
  }
  void IncludeRef(int line_nr, const char* refstr) {
    ParseNode* refnode = nullptr;
    if (*refstr == '@') {
      ParseNode tmp;
      tmp.value().set_string(refstr, Value::Set::kAsStatic);
      refnode = ParseNodeTree::FollowNodeRef(&tmp);
    } else {
      // Local look-up.
      // NOTE: If we read to a target node that already contains
      //       nodes, we might look up nodes that's already there
      //       instead of new nodes.
      refnode = m_root_node->GetNode(refstr, ParseNode::MissingPolicy::kNull);

      // Detect cycles.
      ParseNode* cycle_detection = m_target_node;
      while (cycle_detection && refnode) {
        if (cycle_detection == refnode) {
          refnode = nullptr;  // We have a cycle, so just fail the inclusion.
        }
        cycle_detection = cycle_detection->parent();
      }
    }
    if (refnode) {
      m_target_node->CloneChildren(refnode);
    } else {
      OnError(line_nr,
              el::util::format_string("Include \"%s\" was not found!", refstr));
    }
  }

 private:
  ParseNode* m_root_node;
  ParseNode* m_target_node;
  std::string m_filename;
};

void ParseNode::TakeValue(Value& value) { m_value.TakeOver(value); }

void ParseNode::EmplaceValue(Value value) { m_value.TakeOver(value); }

bool ParseNode::ReadFile(const std::string& filename, ReadFlags flags) {
  if (!any(flags & ReadFlags::kAppend)) {
    Clear();
  }
  FileTextParserStream p;
  ParseNodeTarget t(this, filename);
  if (p.Read(filename, &t)) {
    ParseNodeTree::ResolveConditions(this);
    return true;
  }
  return false;
}

void ParseNode::ReadData(const char* data, ReadFlags flags) {
  ReadData(data, strlen(data), flags);
}

void ParseNode::ReadData(const char* data, size_t data_length,
                         ReadFlags flags) {
  if (!any(flags & ReadFlags::kAppend)) {
    Clear();
  }
  DataTextParserStream p;
  ParseNodeTarget t(this, "{data}");
  p.Read(data, data_length, &t);
  ParseNodeTree::ResolveConditions(this);
}

void ParseNode::Clear() {
  free(m_name);
  m_name = nullptr;
  m_children.DeleteAll();
}

}  // namespace parsing
}  // namespace el
