/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * Copyright 2015 Ben Vanik. All rights reserved. Licensed as BSD 3-clause.   *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#ifndef EL_TEXT_UNDO_STACK_H_
#define EL_TEXT_UNDO_STACK_H_

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace el {
namespace text {

class TextView;

// Event in the UndoStack. Each insert or remove change is stored as a
// UndoEvent, but they may also be merged when appropriate.
class UndoEvent {
 public:
  size_t gofs;
  std::string text;
  bool insert;
};

// Keeps track of all UndoEvents used for undo and redo functionality.
class UndoStack {
 public:
  UndoStack() = default;
  ~UndoStack() = default;

  void Undo(TextView* style_edit);
  void Redo(TextView* style_edit);
  void Clear(bool clear_undo, bool clear_redo);

  UndoEvent* Commit(TextView* style_edit, size_t gofs, size_t len,
                    const char* text, bool insert);

 public:
  std::vector<std::unique_ptr<UndoEvent>> undos;
  std::vector<std::unique_ptr<UndoEvent>> redos;
  bool applying = false;

 private:
  void Apply(TextView* style_edit, UndoEvent* e, bool reverse);
};

}  // namespace text
}  // namespace el

#endif  // EL_TEXT_UNDO_STACK_H_
