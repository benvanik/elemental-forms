/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#ifndef EL_DSL_H_
#define EL_DSL_H_

#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

#include "el/color.h"
#include "el/element.h"
#include "el/parsing/parse_node.h"
#include "el/rect.h"

namespace el {
namespace dsl {

inline std::string operator"" _px(uint64_t value) {
  return std::to_string(value) + "px";
}
inline std::string operator"" _dp(uint64_t value) {
  return std::to_string(value) + "dp";
}
inline std::string operator"" _mm(uint64_t value) {
  return std::to_string(value) + "mm";
}

class Node {
 public:
  el::parsing::ParseNode* parse_node() const { return parse_node_; }

  Node& set(const char* key, int32_t value) {
    auto v = el::Value(value);
    return set(key, v);
  }

  Node& set(const char* key, float value) {
    auto v = el::Value(value);
    return set(key, v);
  }

  Node& set(const char* key, const char* value) {
    auto v = el::Value(value);
    return set(key, v);
  }

  Node& set(const char* key, const std::string& value) {
    auto v = el::Value(value.c_str());
    return set(key, v);
  }

  Node& set(const char* key, el::Rect value) {
    auto va = new el::ValueArray();
    va->AddInteger(value.x);
    va->AddInteger(value.y);
    va->AddInteger(value.w);
    va->AddInteger(value.h);
    auto node = GetOrCreateNode(key);
    node->value().set_array(va, el::Value::Set::kTakeOwnership);
    return *this;
  }

  Node& set(const char* key, el::Value& value) {
    auto node = GetOrCreateNode(key);
    node->TakeValue(value);
    return *this;
  }

  Node& child_list(std::initializer_list<Node> children) {
    for (auto& child : children) {
      parse_node_->Add(child.parse_node_);
    }
    return *this;
  }

 protected:
  Node(const char* name,
       std::vector<std::pair<const char*, const char*>> properties = {},
       std::vector<Node> children = {}) {
    parse_node_ = el::parsing::ParseNode::Create(name);
    for (auto& prop : properties) {
      set(prop.first, prop.second);
    }
    for (auto& child : children) {
      parse_node_->Add(child.parse_node_);
    }
  }

  el::parsing::ParseNode* GetOrCreateNode(const char* name) {
    return parse_node_->GetNode(name,
                                el::parsing::ParseNode::MissingPolicy::kCreate);
  }

  el::parsing::ParseNode* parse_node_ = nullptr;
};

struct Dimension {
  Dimension(int32_t value) : value(std::to_string(value) + "px") {}
  Dimension(const char* value) : value(value) {}
  Dimension(std::string value) : value(std::move(value)) {}
  operator std::string() { return value; }
  std::string value;
};

struct Id {
  Id(int32_t value) : is_int(true), int_value(value) {}
  Id(const char* value) : str_value(value) {}
  Id(std::string value) : str_value(std::move(value)) {}
  void set(Node* node, const char* key) {
    if (is_int) {
      node->set(key, int_value);
    } else {
      node->set(key, str_value);
    }
  }
  bool is_int = false;
  int32_t int_value = 0;
  std::string str_value;
};

struct CloneNode : public Node {
  using R = CloneNode;
  CloneNode(const Node& source) : Node(source.parse_node()->name()) {
    parse_node_->value().Copy(source.parse_node()->value());
    parse_node_->CloneChildren(source.parse_node());
  }
};

}  // namespace dsl
}  // namespace el

#endif  // EL_DSL_H_
