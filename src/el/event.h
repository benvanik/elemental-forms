/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * Copyright 2015 Ben Vanik. All rights reserved. Licensed as BSD 3-clause.   *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#ifndef EL_EVENT_H_
#define EL_EVENT_H_

#include <string>
#include <vector>

#include "el/id.h"
#include "el/types.h"
#include "el/util/object.h"

namespace el {

class Element;

enum class EventType {
  // Click event is what should be used to trig actions in almost all cases.
  // It is invoked on a element after POINTER_UP if the pointer is still inside
  // its hit area. It can also be invoked by keyboard on some clickable elements
  // (see Element::SetClickByKey).
  // If panning of scrollable elements start while the pointer is down, CLICK
  // won't be invoked when releasing the pointer (since that should stop
  // panning).
  kClick,
  // Long click event is sent when the pointer has been down for some time
  // without moving much.
  // It is invoked on a element that has enabled it (Element::SetWantLongClick).
  // If this event isn't handled, the element will invoke a CONTEXT_MENU event.
  // If any of those are handled, the CLICK event that would normally be invoked
  // after the pending POINTER_UP will be suppressed.
  kLongClick,
  kPointerDown,
  kPointerUp,
  kPointerMove,
  kWheel,
  // Invoked after changing text in a Label, changing selected item in a
  // ListBox etc. Invoking this event trigs synchronization with connected
  // ElementValue and other elements connected to it.
  kChanged,
  kKeyDown,
  kKeyUp,
  // Invoked by the platform when a standard keyboard shortcut is pressed.
  // It's called before InvokeKeyDown (kKeyDown) and if the event is
  // handled (returns true), the KeyDown is canceled.
  // The ref_id will be set to one of the following:
  // "cut", "copy", "paste", "selectall", "undo", "redo", "new", "open", "save".
  kShortcut,
  // Invoked when a context menu should be opened at the event x and y
  // coordinates.
  // It may be invoked automatically for a element on long click, if nothing
  // handles the long click event.
  kContextMenu,
  // Invoked by the platform when one or multiple files has been dropped on the
  // element. The event is guaranteed to be a FileDropEvent.
  kFileDrop,
  // Custom event. Not used internally.
  // ref_id may be used for additional type info.
  kCustom,
};
MAKE_ORDERED_ENUM_STRING_UTILS(EventType, "kClick", "kLongClick",
                               "kPointerDown", "kPointerUp", "kPointerMove",
                               "kWheel", "kChanged", "kKeyDown", "kKeyUp",
                               "kShortcut", "kContextMenu", "kFileDrop",
                               "kCustom");

enum class ModifierKeys {
  kNone = 0,
  kCtrl = 1,
  kShift = 2,
  kAlt = 4,
  kSuper = 8,
};
MAKE_ENUM_FLAG_COMBO(ModifierKeys);

enum class SpecialKey {
  kUndefined,
  kUp,
  kDown,
  kLeft,
  kRight,
  kPageUp,
  kPageDown,
  kHome,
  kEnd,
  kTab,
  kBackspace,
  kInsert,
  kDelete,
  kEnter,
  kEsc,
  kF1,
  kF2,
  kF3,
  kF4,
  kF5,
  kF6,
  kF7,
  kF8,
  kF9,
  kF10,
  kF11,
  kF12,
};

class Event : public util::TypedObject {
 public:
  TBOBJECT_SUBCLASS(Event, util::TypedObject);

  EventType type;
  // The element that invoked the event.
  Element* target = nullptr;
  // X position in target element. Set for all pointer events, click and wheel.
  int target_x = 0;
  // Y position in target element. Set for all pointer events, click and wheel.
  int target_y = 0;
  // Set for EventType::kWheel. Positive is a turn right.
  int delta_x = 0;
  // Set for EventType::kWheel. Positive is a turn against the user.
  int delta_y = 0;
  // 1 for all events, but increased for POINTER_DOWN event to 2 for
  // doubleclick, 3 for tripleclick and so on.
  int count = 1;
  int key = 0;
  SpecialKey special_key = SpecialKey::kUndefined;
  ModifierKeys modifierkeys = ModifierKeys::kNone;
  // Sometimes (when documented) events have a ref_id (the id that caused this
  // event).
  TBID ref_id;
  // Set for pointer events. True if the event is a touch event (finger or pen
  // on screen).
  bool touch = false;

  explicit Event(EventType type) : type(type) {}

  Event(EventType type, int x, int y, bool touch,
        ModifierKeys modifierkeys = ModifierKeys::kNone)
      : type(type),
        target_x(x),
        target_y(y),
        modifierkeys(modifierkeys),
        touch(touch) {}

  // The count value may be 1 to infinity. If you f.ex want to see which count
  // it is for something handling click and double click, call GetCountCycle(2).
  // If you also handle triple click, call GetCountCycle(3) and so on. That way
  // you'll get a count that always cycle in the range you need.
  int GetCountCycle(int max) { return ((count - 1) % max) + 1; }

  bool is_pointer_event() const {
    return type == EventType::kPointerDown || type == EventType::kPointerUp ||
           type == EventType::kPointerMove;
  }
  bool is_key_event() const {
    return type == EventType::kKeyDown || type == EventType::kKeyUp;
  }
};

// An event of type EventType::kFileDrop.
// It contains a list of filenames of the files that was dropped.
class FileDropEvent : public Event {
 public:
  TBOBJECT_SUBCLASS(FileDropEvent, Event);

  std::vector<std::string> files;

  FileDropEvent() : Event(EventType::kFileDrop) {}
};

}  // namespace el

#endif  // EL_EVENT_H_
