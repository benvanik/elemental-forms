/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#ifndef TB_WIDGETS_H
#define TB_WIDGETS_H

#include <algorithm>

#include "tb_core.h"
#include "tb_font_desc.h"
#include "tb_geometry.h"
#include "tb_linklist.h"
#include "tb_object.h"
#include "tb_skin.h"
#include "tb_types.h"
#include "tb_widget_value.h"

namespace tb {

class Window;
class TBWidget;
class FontFace;
class Scroller;
class WidgetListener;
class TBLongClickTimer;
struct InflateInfo;

enum class Align {
  kLeft,
  kTop,
  kRight,
  kBottom,
};
MAKE_ORDERED_ENUM_STRING_UTILS(Align, "left", "top", "right", "bottom");

enum class EventType {
  // Click event is what should be used to trig actions in almost all cases.
  // It is invoked on a widget after POINTER_UP if the pointer is still inside
  // its hit area. It can also be invoked by keyboard on some clickable widgets
  // (see TBWidget::SetClickByKey).
  // If panning of scrollable widgets start while the pointer is down, CLICK
  // won't be invoked when releasing the pointer (since that should stop
  // panning).
  kClick,
  // Long click event is sent when the pointer has been down for some time
  // without moving much.
  // It is invoked on a widget that has enabled it (TBWidget::SetWantLongClick).
  // If this event isn't handled, the widget will invoke a CONTEXT_MENU event.
  // If any of those are handled, the CLICK event that would normally be invoked
  // after the pending POINTER_UP will be suppressed.
  kLongClick,
  kPointerDown,
  kPointerUp,
  kPointerMove,
  kWheel,
  // Invoked after changing text in a Label, changing selected item in a
  // SelectList etc. Invoking this event trigs synchronization with connected
  // TBWidgetValue and other widgets connected to it.
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
  // It may be invoked automatically for a widget on long click, if nothing
  // handles the long click event.
  kContextMenu,
  // Invoked by the platform when one or multiple files has been dropped on the
  // widget. The event is guaranteed to be a TBWidgetEventFileDrop.
  kFileDrop,
  // Custom event. Not used internally.
  // ref_id may be used for additional type info.
  kCustom,
};

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

class TBWidgetEvent : public TBTypedObject {
 public:
  TBWidget* target;  ///< The widget that invoked the event
  EventType type;    ///< Which type of event
  int target_x;  ///< X position in target widget. Set for all pointer events,
  /// click and wheel.
  int target_y;  ///< Y position in target widget. Set for all pointer events,
  /// click and wheel.
  int delta_x;  ///< Set for EventType::kWheel. Positive is a turn right.
  int delta_y;  ///< Set for EventType::kWheel. Positive is a turn against the
  /// user.
  int count;  ///< 1 for all events, but increased for POINTER_DOWN event to 2
  /// for doubleclick, 3 for tripleclick and so on.
  int key;
  SpecialKey special_key;
  ModifierKeys modifierkeys;
  TBID ref_id;  ///< Sometimes (when documented) events have a ref_id (The id
  /// that caused this event)
  bool touch;  ///< Set for pointer events. True if the event is a touch event
               ///(finger or pen on screen)
               ///< False if mouse or other cursor input.

  TBOBJECT_SUBCLASS(TBWidgetEvent, TBTypedObject);

  TBWidgetEvent(EventType type)
      : target(nullptr),
        type(type),
        target_x(0),
        target_y(0),
        delta_x(0),
        delta_y(0),
        count(1),
        key(0),
        special_key(SpecialKey::kUndefined),
        modifierkeys(ModifierKeys::kNone),
        touch(false) {}

  TBWidgetEvent(EventType type, int x, int y, bool touch,
                ModifierKeys modifierkeys = ModifierKeys::kNone)
      : target(nullptr),
        type(type),
        target_x(x),
        target_y(y),
        delta_x(0),
        delta_y(0),
        count(1),
        key(0),
        special_key(SpecialKey::kUndefined),
        modifierkeys(modifierkeys),
        touch(touch) {}

  /** The count value may be 1 to infinity. If you f.ex want to see which count
     it is for something
          handling click and double click, call GetCountCycle(2). If you also
     handle triple click, call
          GetCountCycle(3) and so on. That way you'll get a count that always
     cycle in the range you need. */
  int GetCountCycle(int max) { return ((count - 1) % max) + 1; }

  bool IsPointerEvent() const {
    return type == EventType::kPointerDown || type == EventType::kPointerUp ||
           type == EventType::kPointerMove;
  }
  bool IsKeyEvent() const {
    return type == EventType::kKeyDown || type == EventType::kKeyUp;
  }
};

/** TBWidgetEventFileDrop is a event of type EventType::kFileDrop.
        It contains a list of filenames of the files that was dropped. */
class TBWidgetEventFileDrop : public TBWidgetEvent {
 public:
  TBListAutoDeleteOf<std::string> files;

  TBOBJECT_SUBCLASS(TBWidgetEventFileDrop, TBWidgetEvent);

  TBWidgetEventFileDrop() : TBWidgetEvent(EventType::kFileDrop) {}
};

// TBWidget gravity (may be combined).
// Gravity gives hints about positioning and sizing preferences.
enum class Gravity {
  kNone = 0,
  kLeft = 1,
  kRight = 2,
  kTop = 4,
  kBottom = 8,

  kLeftRight = kLeft | kRight,
  kTopBottom = kTop | kBottom,
  kAll = kLeftRight | kTopBottom,
  kDefault = kLeft | kTop,
};
MAKE_ENUM_FLAG_COMBO(Gravity);
inline std::string to_string(Gravity value) {
  if (value == Gravity::kNone) {
    return "none";
  } else if (value == Gravity::kAll) {
    return "all";
  }
  std::string result;
  if (any(value & Gravity::kLeft)) {
    result += " left";
  }
  if (any(value & Gravity::kRight)) {
    result += " right";
  }
  if (any(value & Gravity::kTop)) {
    result += " top";
  }
  if (any(value & Gravity::kBottom)) {
    result += " bottom";
  }
  return result.substr(1);
}

enum class Axis {
  kX,  // Horizontal layout.
  kY,  // Vertical layout.
};
MAKE_ORDERED_ENUM_STRING_UTILS(Axis, "x", "y");

// Defines how the size in one axis depend on the other axis when a widgets size
// is affected by constraints.
enum class SizeDependency {
  // No dependency (Faster layout).
  kNone = 0,
  // The width is dependant on the height. Additional layout pass may be
  // required.
  kWidthOnHeight = 1,
  // The height is dependant on the width. Additional layout pass may be
  // required.
  kHeightOnWidth = 2,
  // Both width and height are dependant on each other. Additional layout pass
  // may be required.
  kWidthAndHeight = kWidthOnHeight | kHeightOnWidth
};
MAKE_ENUM_FLAG_COMBO(SizeDependency);

/** PreferredSize contains size preferences for a TBWidget.
        This is calculated during layout for each widget from
        the current skin, widget preferences and LayoutParams. */
class PreferredSize {
 public:
  PreferredSize()
      : min_w(0),
        min_h(0),
        max_w(10000),
        max_h(10000),
        pref_w(0),
        pref_h(0),
        size_dependency(SizeDependency::kNone) {}
  PreferredSize(int w, int h)
      : min_w(w),
        min_h(h),
        max_w(w),
        max_h(h),
        pref_w(w),
        pref_h(h),
        size_dependency(SizeDependency::kNone) {}

  int min_w, min_h;    ///< The minimal preferred width and height.
  int max_w, max_h;    ///< The maximum preferred width and height.
  int pref_w, pref_h;  ///< The preferred width and height.
  SizeDependency
      size_dependency;  ///< The size dependency when size is affected by
  /// constraints.
};

/** LayoutParams defines size preferences for a TBWidget that
        are set on the widget to override size preferences from
        skin and widget. */
class LayoutParams {
 public:
  static const int kUnspecified = kInvalidDimension;
  LayoutParams()
      : min_w(kUnspecified),
        min_h(kUnspecified),
        max_w(kUnspecified),
        max_h(kUnspecified),
        pref_w(kUnspecified),
        pref_h(kUnspecified) {}

  /** Set both min max and preferred width to the given width. */
  void SetWidth(int width) { min_w = max_w = pref_w = width; }

  /** Set both min max and preferred height to the given height. */
  void SetHeight(int height) { min_h = max_h = pref_h = height; }

  int min_w, min_h;    ///< The minimal preferred width and height.
  int max_w, max_h;    ///< The maximum preferred width and height.
  int pref_w, pref_h;  ///< The preferred width and height.
};

/** Specifies size constraints used during size calculations. */
class SizeConstraints {
 public:
  static const int NO_RESTRICTION = 10000;

  /** The available width and height. May be NO_RESTRICTION which is a large
   * value. */
  int available_w, available_h;

  /** Constrain to the given width and height. */
  SizeConstraints(int w, int h) : available_w(w), available_h(h) {}

  /** No constraints. */
  SizeConstraints()
      : available_w(NO_RESTRICTION), available_h(NO_RESTRICTION) {}

  /** Return new constraints reduced by the given padding. */
  SizeConstraints ConstrainByPadding(int horizontal_padding,
                                     int vertical_padding) const {
    return SizeConstraints(
        available_w == NO_RESTRICTION ? NO_RESTRICTION
                                      : available_w - horizontal_padding,
        available_h == NO_RESTRICTION ? NO_RESTRICTION
                                      : available_h - vertical_padding);
  }

  /** Return new constraints that are constrained by LayoutParams. */
  SizeConstraints ConstrainByLayoutParams(const LayoutParams& lp) const {
    return SizeConstraints(ConstrainByLPMax(available_w, lp.min_w, lp.max_w),
                           ConstrainByLPMax(available_h, lp.min_h, lp.max_h));
  }

  bool operator==(const SizeConstraints& sc) const {
    return available_w == sc.available_w && available_h == sc.available_h;
  }

 private:
  int ConstrainByLPMax(int constraint, int lp_min, int lp_max) const {
    if (constraint == NO_RESTRICTION)
      return lp_max != LayoutParams::kUnspecified ? lp_max : NO_RESTRICTION;
    int ret = constraint;
    if (lp_min != LayoutParams::kUnspecified) ret = std::max(ret, lp_min);
    if (lp_max != LayoutParams::kUnspecified) ret = std::min(ret, lp_max);
    return ret;
  }
};

// Defines widget z level, used with TBWidget::SetZ, TBWidget::AddChild.
enum class WidgetZ {
  kTop,     // The toplevel (Visually drawn on top of everything else).
  kBottom,  // The bottomlevel (Visually drawn behind everything else).
};
MAKE_ORDERED_ENUM_STRING_UTILS(WidgetZ, "top", "bottom");

// Defines widget z level relative to another widget, used with
// TBWidget::AddChildRelative.
enum class WidgetZRel {
  kBefore,  // Before the reference widget (visually behind reference).
  kAfter,   // After the reference widget (visually above reference).
};
MAKE_ORDERED_ENUM_STRING_UTILS(WidgetZRel, "before", "after");

// Defines widget visibility, used with TBWidget::SetVisibility.
enum class Visibility {
  kVisible,    // Visible (default).
  kInvisible,  // Invisible, but layouted. Interaction disabled.
  kGone        // Invisible and no layout. Interaction disabled.
};
MAKE_ORDERED_ENUM_STRING_UTILS(Visibility, "visible", "invisible", "gone");

enum class InvokeInfo {
  kNormal,
  kNoCallbacks,
};

enum class FocusReason {
  kNavigation,  // Set focus by navigation (i.e. keyboard tab). This will scroll
                // to the widget if needed.
  kPointer,     // Set focus by pointer (i.e. clicking).
  kUnknown,     // Set focus by anything else.
};

// Hit status return value for TBWidget::GetHitStatus.
enum class HitStatus {
  kNoHit,          // The widget was not hit.
  kHit,            // The widget was hit, any child may be hit too.
  kHitNoChildren,  // The widget was hit, no children should be hit.
};

/** The base TBWidget class.
        Make a subclass to implement UI controls.
        Each widget has a background skin (no skin specified by default) which
   will be used to
        calculate the default size preferences and padding around the preferred
   content size.

        Note: When you subclass a widget, use the TBOBJECT_SUBCLASS macro to
   define the type
        casting functions instead of implementing those manually. */

class TBWidget : public TBTypedObject, public TBLinkOf<TBWidget> {
 public:
  TBOBJECT_SUBCLASS(TBWidget, TBTypedObject);

  TBWidget();
  virtual ~TBWidget();

  /** Set the rect for this widget in its parent. The rect is relative to the
     parent widget.
          The skin may expand outside this rect to draw f.ex shadows. */
  void SetRect(const Rect& rect);
  inline Rect GetRect() const { return m_rect; }

  /** Set position of this widget in its parent. The position is relative to the
   * parent widget. */
  void SetPosition(const Point& pos) {
    SetRect(Rect(pos.x, pos.y, m_rect.w, m_rect.h));
  }

  /** Set size of this widget. */
  void SetSize(int width, int height) {
    SetRect(Rect(m_rect.x, m_rect.y, width, height));
  }

  /** Invalidate should be called if the widget need to be repainted,
          to make sure the renderer repaints it and its children next frame. */
  void Invalidate();

  /** Call if something changes that might need other widgets to update their
     state.
          F.ex if a action availability changes, some widget might have to
     become enabled/disabled.
          Calling this will result in a later call to OnProcessStates().
          This is done automatically for all invoked events of type:
                  EventType::kClick, EventType::kLongClick, EventType::kChanged,
     EventType_KEYDOWN,
                  EventType_KEYUP. */
  void InvalidateStates();

  /** Call if something changes that might cause any skin to change due to
     different state
          or conditions. This is called automatically from InvalidateStates(),
     when event
          EventType::kChanged is invoked, and in various other situations. */
  void InvalidateSkinStates();

  /** Delete the widget with the possibility for some extended life during
     animations.

          If any widget listener responds true to OnWidgetDying it will be kept
     as a child and live
          until the animations are done, but the widgets and all its children
     are marked as dying.
          Dying widgets get no input or focus.

          If no widget listener responded, it will be deleted immediately. */
  void Die();

  /** Return true if this widget or any of its parents is dying. */
  bool GetIsDying() const {
    return m_packed.is_dying || (m_parent && m_parent->GetIsDying());
  }

  /** Set the id reference for this widgets. This id is 0 by default.
          You can use this id to receive the widget from GetWidgetByID (or
          preferable TBSafeGetByID to avoid dangerous casts). */
  void SetID(const TBID& id);
  TBID& GetID() { return m_id; }

  /** Set the group id reference for this widgets. This id is 0 by default.
          All widgets with the same group id under the same group root will
          be automatically changed when one change its value. */
  void SetGroupID(const TBID& id) { m_group_id = id; }
  TBID& GetGroupID() { return m_group_id; }

  /** Get this widget or any child widget with a matching id, or nullptr if none
   * is found. */
  TBWidget* GetWidgetByID(const TBID& id) { return GetWidgetByIDInternal(id); }

  /** Get this widget or any child widget with a matching id and type, or
   * nullptr if none is found. */
  template <class T>
  T* GetWidgetByIDAndType(const TBID& id) {
    return (T*)GetWidgetByIDInternal(id, GetTypeId<T>());
  }

  /** Enable or disable the given state(s). The state affects which skin state
     is used when drawing.
          Some states are set automatically on interaction. See GetAutoState().
     */
  void SetState(SkinState state, bool on);

  /** Get status of the given state(s). Returns true if the given state
   * combination is set. */
  bool GetState(SkinState state) const { return any(m_state & state); }

  /** Set the widget state. Like SetState but setting the entire state as given,
     instead
          of toggling individual states. See SetState for more info on states.
     */
  void SetStateRaw(SkinState state);

  /** Get the widget state. */
  SkinState GetStateRaw() const { return m_state; }

  /** Return the current combined state for this widget. It will also add some
          automatic states, such as hovered (if the widget is currently
     hovered), or pressed etc.

          Automatic states: SkinState::kPressed, SkinState::kHovered,
     SkinState::kFocused.

          Remarks for SkinState::kFocused: May also be controlled by calling
     SetAutoFocusState and
          the define TB_ALWAYS_SHOW_EDIT_FOCUS. */
  SkinState GetAutoState() const;

  /** Set if the state SkinState::kFocused should be set automatically for the
     focused widget.
          This value is set to true when moving focus by keyboard, and set to
     off when clicking
          with the pointer. */
  static void SetAutoFocusState(bool on);

  /** Set opacity for this widget and its children from 0.0 - 1.0.
          If opacity is 0 (invisible), the widget won't receive any input. */
  void SetOpacity(float opacity);
  float GetOpacity() const { return m_opacity; }

  /** Set visibility for this widget and its children.
          If visibility is not Visibility::kVisible, the widget won't
     receive any input. */
  void SetVisibilility(Visibility vis);
  Visibility GetVisibility() const;

  /** Return true if this widget and all its ancestors are visible
          (has a opacity > 0 and visibility Visibility::kVisible) */
  bool GetVisibilityCombined() const;

  /** Return true if this widget or any of its parents are disabled (has state
   * SkinState::kDisabled). */
  bool GetDisabled() const;

  /** Add the child to this widget. The child widget will automatically be
     deleted when
          this widget is deleted. (If the child isn't removed again with
     RemoveChild.) */
  void AddChild(TBWidget* child, WidgetZ z = WidgetZ::kTop,
                InvokeInfo info = InvokeInfo::kNormal);

  /** Add the child to this widget. See AddChild for adding a child to the top
     or bottom.
          This takes a relative Z and insert the child before or after the given
     reference widget.*/
  void AddChildRelative(TBWidget* child, WidgetZRel z, TBWidget* reference,
                        InvokeInfo info = InvokeInfo::kNormal);

  /** Remove child from this widget without deleting it. */
  void RemoveChild(TBWidget* child, InvokeInfo info = InvokeInfo::kNormal);

  /** Remove and delete all children in this widget.
          Note: This won't invoke Die so there's no chance for widgets to
     survive or
          animate. They will be instantly removed and deleted. */
  void DeleteAllChildren();

  /** Sets the z-order of this widget related to its siblings. When a widget is
     added with AddChild, it will be
          placed at the top in the parent (Above previously added widget). SetZ
     can be used to change the order. */
  void SetZ(WidgetZ z);

  /** Set the z order in which children are added during resource loading. */
  void SetZInflate(WidgetZ z) { m_packed.inflate_child_z = int(z) ? 1 : 0; }
  WidgetZ GetZInflate() const { return (WidgetZ)m_packed.inflate_child_z; }

  /** Set the widget gravity (any combination of Gravity).
          For child widgets in a layout, the gravity affects how the layout is
     done depending on the layout settings.
          For child widgets in a non layout widget, it will do some basic
     resizing/moving:
                  -left && right: Widget resize horizontally when parent resize.
                  -!left && right: Widget follows the right edge when parent
     resize.
                  -top && bottom: Widget resize vertically when parent resize.
                  -!top && bottom: Widget follows the bottom edge when parent
     resize. */
  void SetGravity(Gravity g);
  Gravity GetGravity() const { return m_gravity; }

  /** Set the skin background for this widget and call OnSkinChanged if it
     changed.

          The skin background is used for calculating padding, preferred size
          etc. if the widget doesn't have any preferences itself.

          The skin will be painted according to the current widget state
     (SkinState).
          If there is no special skin state for SkinState::kFocused, it will
     paint the skin
          element called "generic_focus" (if it exist) after painting all widget
     children.

          It's possible to omit the OnSkinChanged callback using
     InvokeInfo::kNoCallbacks. */
  void SetSkinBg(const TBID& skin_bg, InvokeInfo info = InvokeInfo::kNormal);

  /** Return the current skin background, as set by SetSkinBg. */
  TBID GetSkinBg() const { return m_skin_bg; }

  /** Return the skin background element, or nullptr. */
  SkinElement* GetSkinBgElement();

  /** Set if this widget is a group root. Grouped widgets (such as
     RadioButton) will toggle all other
          widgets with the same group_id under the nearest parent group root.
     Window is a group root by default. */
  void SetIsGroupRoot(bool group_root) { m_packed.is_group_root = group_root; }
  bool GetIsGroupRoot() const { return m_packed.is_group_root; }

  /** Set if this widget should be able to receive focus or not. */
  void SetIsFocusable(bool focusable) { m_packed.is_focusable = focusable; }
  bool GetIsFocusable() const { return m_packed.is_focusable; }

  /** Set if this widget should emulate a click when it's focused and pressing
   * enter or space. */
  void SetClickByKey(bool click_by_key) {
    m_packed.click_by_key = click_by_key;
  }
  bool GetClickByKey() const { return m_packed.click_by_key; }

  /** Set if this widget should generate long-click event (or context menu event
     if nothing
          handles the long click event). The default is false. */
  void SetWantLongClick(bool want_long_click) {
    m_packed.want_long_click = want_long_click;
  }
  bool GetWantLongClick() const { return m_packed.want_long_click; }

  /** Set if this widget should ignore input, as if it didn't exist. */
  void SetIgnoreInput(bool ignore_input) {
    m_packed.ignore_input = ignore_input;
  }
  bool GetIgnoreInput() const { return m_packed.ignore_input; }

  /** Get if this widget wants interaction depending on various states.
          Cares about zero opacity, visibility, flag set by SetIgnoreInput,
     disabled state,
          and if the widget is currently dying. */
  bool GetIsInteractable() const;

  /** Set this widget to be the focused widget. It will be the one receiving
     keyboard input.
          Widgets can be focused only after enabling it (See
     SetIsFocusable(true)).
          Invisible or disabled widgets can not be focused.

          If SetFocus is called on a widget in a inactive window, it will
     succeed (return true),
          but it won't actually become focused until that window is activated
     (See Window::SetLastFocus).

          Returns true if successfully focused, or if set as last focus in its
     window. */
  bool SetFocus(FocusReason reason, InvokeInfo info = InvokeInfo::kNormal);
  bool GetIsFocused() const { return focused_widget == this; }

  /** Call SetFocus on all children and their children, until a widget is found
     that accepts it.
          Returns true if some child was successfully focused. */
  bool SetFocusRecursive(FocusReason reason);

  /** Move focus from the currently focused widget to another focusable widget.
     It will search
          for a focusable widget in the same Window (or top root if there is
     no window) forward or
          backwards in the widget order. */
  bool MoveFocus(bool forward);

  /** Returns the child widget that contains the coordinate or nullptr if no one
     does. If include_children
          is true, the search will recurse into the childrens children. */
  TBWidget* GetWidgetAt(int x, int y, bool include_children) const;

  /** Get the child at the given index, or nullptr if there was no child at that
     index.
          Note: Avoid calling this in loops since it does iteration. Consider
     iterating
          the widgets directly instead! */
  TBWidget* GetChildFromIndex(int index) const;

  /** Get the child index of the given widget (that must be a child of this
     widget).
          Note: Avoid calling this in loops since it does iteration. Consider
     iterating
          the widgets directly instead! */
  int GetIndexFromChild(TBWidget* child) const;

  /** Get the text of a child widget with the given id, or an empty string if
     there was
          no widget with that id. */
  std::string GetTextByID(const TBID& id);

  /** Get the value of a child widget with the given id, or 0 if there was no
     widget
          with that id. */
  int GetValueByID(const TBID& id);

  TBWidget* GetNextDeep(const TBWidget* bounding_ancestor = nullptr) const;
  TBWidget* GetPrevDeep() const;
  TBWidget* GetLastLeaf() const;
  inline TBWidget* GetFirstChild() const { return m_children.GetFirst(); }
  inline TBWidget* GetLastChild() const { return m_children.GetLast(); }
  TBLinkListOf<TBWidget>::Iterator GetIteratorForward() {
    return m_children.IterateForward();
  }
  TBLinkListOf<TBWidget>::Iterator GetIteratorBackward() {
    return m_children.IterateBackward();
  }

  /** Return true if this widget is the same or a ancestor of other_widget. */
  bool IsAncestorOf(TBWidget* other_widget) const;

  /** Return true if this widget is the same as other_widget or if other_widget
          events are going through this widget (See GetEventDestination()) */
  bool IsEventDestinationFor(TBWidget* other_widget) const;

  // == Callbacks ==============================================

  /** Add a listener to this widget. It should be removed again with
          RemoveListener before the widget is deleted. */
  void AddListener(WidgetListener* listener);
  void RemoveListener(WidgetListener* listener);
  bool HasListener(WidgetListener* listener) const;

  /** Callback for handling events.
          Return true if the event is handled and should not
          continue to be handled by any parent widgets. */
  virtual bool OnEvent(const TBWidgetEvent& ev) { return false; }

  /** Callback for doing anything that might be needed before paint.
          F.ex Updating invalid layout, formatting text etc. */
  virtual void OnProcess() {}

  /** Callback for doing anything that might be needed before paint.
          This is called after OnProcess has been called on this widgets
     children. */
  virtual void OnProcessAfterChildren() {}

  /** Callback for doing state updates that depend on your application state.
          F.ex setting the disabled state on a widget which action is currently
     not
          available. This callback is called for all widgets before OnProcess if
          something has called InvalidateStates().*/
  virtual void OnProcessStates() {}

  /** PaintProps contains properties needed for painting a widget.
          Properties may be inherited from the parent widget if not specified
          in the skin. */
  class PaintProps {
   public:
    PaintProps();

    /** Text color as specified in the skin element, or inherited from parent.
     */
    Color text_color;
  };

  /** Callback for painting this widget.
          The skin is already painted and the opacity set to reflect this
     widgets.
          This is only called for widgets with a opacity > 0 */
  virtual void OnPaint(const PaintProps& paint_props) {}

  /** Callback for painting child widgets.
          The default implementation is painting all children. */
  virtual void OnPaintChildren(const PaintProps& paint_props);

  /** Callback for when this widget or any of its children have
          called Invalidate() */
  virtual void OnInvalid() {}

  /** Called when the background skin changes by calling SetSkinBg(), or when
     the skin
          has changed indirectly after a skin condition changes in a way that
     may affect layout.

          For indirect skin changes, OnSkinChanged is called before validation
     of layouts is about
          to happen in InvokeProcess().
  */
  virtual void OnSkinChanged() {}

  /** Called when the font has changed. */
  virtual void OnFontChanged() {}

  /** Called when the focus has changed. */
  virtual void OnFocusChanged(bool focused) {}

  /** Called when the visibility has changed.
          Note: This is not called when combined visibility change, so it may
     change visibility
          because of ancestors without this being called. */
  virtual void OnVisibilityChanged() {}

  /** Called when the capture has changed. */
  virtual void OnCaptureChanged(bool captured) {}

  /** Called when a child widget has been added to this widget (before calling
   * OnAdded on child). */
  virtual void OnChildAdded(TBWidget* child) {}

  /** Called when a child widget is about to be removed from this widget (before
   * calling OnRemove on child). */
  virtual void OnChildRemove(TBWidget* child) {}

  /** Called when this widget has been added to a parent (after calling
   * OnChildAdded on parent). */
  virtual void OnAdded() {}

  /** Called when a this widget has been removed from its parent (after calling
   * OnChildRemove on parent). */
  virtual void OnRemove() {}

  /** Called when Die() is called on this widget. Note: Not called for children
     to the widget Die() was
          invoked on even though they are also dying. */
  virtual void OnDie() {}

  /** Called when this widget has been resized.
          The default implementation move and resize all children according to
     their gravity. */
  virtual void OnResized(int old_w, int old_h);

  /** Called when this widget has been scrolled. */
  virtual void OnScroll(int scroll_x, int scroll_y) {}

  /** Called just after a child has been inflated into this widget.
          The default implementation will resize the child to it's preferred
     size
          and position it according to the gravity. If you implement a layouting
          widget, you should override this to prevent doing unnecessary
     measuring. */
  virtual void OnInflateChild(TBWidget* child);

  /** Called when this widget is inflated from resources, before any children
          has been inflated. This will read generic widget properties and add
     the
          widget to the hierarchy if it's not already added. If overridden, you
          must call the super implementation. */
  virtual void OnInflate(const InflateInfo& info);

  /** Get hit status tests if this widget should be hit at the given coordinate.
          The default implementation checks the visibility, ignored input flag,
     rectangle,
          and disabled status. */
  virtual HitStatus GetHitStatus(int x, int y);

  /** Get if skin condition applies to this widget. This is called when a skin
     condition has the property
          SkinProperty::kCustom (not a generic one known by skin and the default
     widget condition context).
          This can be used to extend the skin conditions support with properties
     specific to different widgets. */
  virtual bool GetCustomSkinCondition(
      const SkinCondition::ConditionInfo& info) {
    return false;
  }

  /** Get this widget or a child widget that should be root for other children.
     This is useful
          for widgets having multiple children by default, to specify which one
     that should get the children. */
  virtual TBWidget* GetContentRoot() { return this; }

  /** Get this widget or a parent widget that is the absolute root parent. */
  TBWidget* GetParentRoot();

  /** Get the closest parent widget that is a Window or nullptr if there is
     none.
          If this widget is a window itself, this will be returned. */
  Window* GetParentWindow();

  /** Get the parent widget, or nullptr if this widget is not added. */
  inline TBWidget* GetParent() const { return m_parent; }

  /** Get the widget that should receive the events this widget invoke. By
   * default the parent. */
  virtual TBWidget* GetEventDestination() { return m_parent; }

  /** Return translation the children should have. Any scrolling of child
     widgets
          should be done with this method, by returning the wanted translation.

          When implementing this, you must also implement ScrollTo and
     GetScrollInfo
          so focus-scroll and panning will work automatically when dragging this
     or
          any child widget. Note: You can apply the translation on one widget
     and
          implement those methods on a parent, by returning this widget from the
          parents GetScrollRoot(). */
  virtual void GetChildTranslation(int& x, int& y) const { x = y = 0; }

  /** If this is a widget that scroll children (see GetChildTranslation), it
     should
          scroll to the coordinates x, y.
          This must result in calling OnScroll if scrolling occured. */
  virtual void ScrollTo(int x, int y) {}

  /** Start the Scroller for this widget and scroll it to the given position.
          Will cancel any on going smooth scroll operation. */
  void ScrollToSmooth(int x, int y);

  /** If this is a widget that scroll children (see GetChildTranslation), it
     will
          scroll by delta dx, dy relative to its current position. */
  void ScrollBy(int dx, int dy);

  /** Start the Scroller for this widget and scroll it by the given delta.
          Consecutive calls will accumulate the scroll speed. */
  void ScrollBySmooth(int dx, int dy);

  /** Information about scrolling for a widget at the time of calling
   * GetScrollInfo. */
  class ScrollInfo {
   public:
    ScrollInfo() : min_x(0), min_y(0), max_x(0), max_y(0), x(0), y(0) {}
    bool CanScrollX() const { return max_x > min_x; }
    bool CanScrollY() const { return max_y > min_y; }
    bool CanScrollLeft() const { return x > min_x; }
    bool CanScrollRight() const { return x < max_x; }
    bool CanScrollUp() const { return y > min_y; }
    bool CanScrollDown() const { return y < max_y; }
    bool CanScroll() const { return CanScrollX() || CanScrollY(); }
    int min_x, min_y;  ///< Minimum x and y scroll position.
    int max_x, max_y;  ///< Maximum x and y scroll position.
    int x, y;          ///< Current x and y scroll position.
  };

  /** If this is a widget that scroll children (see GetChildTranslation),
          it should return the current scroll information. */
  virtual ScrollInfo GetScrollInfo() { return ScrollInfo(); }

  /** If this widget is implementing ScrollTo and GetScrollInfo but
          the corresponding GetChildTranslation is implemented on a child,
          you should return that child from this method. */
  virtual TBWidget* GetScrollRoot() { return this; }

  /** Scroll this widget and/or any parent widgets by the given delta.
          dx and dy will be reduced by the amount that was successfully
          scrolled. */
  void ScrollByRecursive(int& dx, int& dy);

  /** Make this widget visible by calling ScrollIntoView on all parent widgets
   */
  void ScrollIntoViewRecursive();

  /** If this is a widget that scroll children (see GetChildTranslation), it
     will
          scroll so that rect is visible. Rect is relative to this widget. */
  void ScrollIntoView(const Rect& rect);

  /** Return the Scroller set up for this widget, or nullptr if creation
   * failed. */
  Scroller* GetScroller();

  // == Setter shared for many types of widgets ============

  /** Set along which axis the content should be layouted. */
  virtual void SetAxis(Axis axis) {}
  virtual Axis GetAxis() const { return Axis::kX; }

  /** Set the value of this widget. Implemented by most widgets (that has a
     value).
          Note: Some widgets also provide special setters with other types (such
     as double). */
  virtual void SetValue(int value) {}
  virtual int GetValue() { return 0; }

  /** Set the value in double precision. It only makes sense to use this instead
          of SetValue() on widgets that store the value as double. F.ex
     ScrollBar, Slider. */
  virtual void SetValueDouble(double value) { SetValue((int)value); }

  /** Return the value in double precision. It only makes sense to use this
     instead
          of GetValue() on widgets that store the value as double. F.ex
     ScrollBar, Slider. */
  virtual double GetValueDouble() { return (double)GetValue(); }

  /** Set the text of this widget. Implemented by most widgets (that has text).
   */
  virtual void SetText(const char* text) {}
  void SetText(const std::string& text) { SetText(text.c_str()); }

  /** Get the text of this widget. Implemented by most widgets (that has text).
   */
  virtual std::string GetText() {
    std::string str;
    str.clear();
    return str;
  }

  /** Get the description string of this widget. Used for tooltips. */
  virtual std::string GetDescription() { return m_desc_str; }

  /** Set the description string for this widget. Used for tooltips. */
  virtual void SetDescription(const char* desc) {
    m_desc_str = desc ? desc : "";
  }

  /** Connect this widget to a widget value.

              When this widget invoke EventType::kChanged, it will automatically
     update the
              connected widget value, and any other widgets that may be
     connected to it.

              On connection, the value of this widget will be updated to the
     value of the
              given TBWidgetValue. */
  void Connect(TBWidgetValue* value) { m_connection.Connect(value, this); }

  /** Unconnect, if this widget is connected to a TBWidgetValue. */
  void Unconnect() { m_connection.Unconnect(); }

  /** Get the rectangle inside any padding, relative to this widget. This is the
          rectangle in which the content should be rendered.

          This may be overridden to f.ex deduct space allocated by visible
     scrollbars
          managed by this widget. Anything that removes space from the content
     area. */
  virtual Rect GetPaddingRect();

  /** Calculate the preferred content size for this widget. This is the size of
     the actual
          content. Don't care about padding or other decoration. */
  virtual PreferredSize OnCalculatePreferredContentSize(
      const SizeConstraints& constraints);

  /** Calculate the preferred size for this widget. This is the full size of the
     widget,
          content + padding + eventual other decoration (but not skin
     expansion).
          This is the size that should be used for layouting a widget.
          The returned PreferredSize also contains minimal size and maximum
     size. */
  virtual PreferredSize OnCalculatePreferredSize(
      const SizeConstraints& constraints);

  /** Get the PreferredSize for this widget.
          This returns cached data if valid, or calls OnCalculatePreferredSize
     if needed. */
  PreferredSize GetPreferredSize(const SizeConstraints& constraints);
  PreferredSize GetPreferredSize() {
    return GetPreferredSize(SizeConstraints());
  }

  // Type used for InvalidateLayout.
  enum class InvalidationMode {
    kTargetOnly,  // InvalidationMode should not be recursively called on
                  // parents.
    kRecursive,   // InvalidationMode should recursively be called on parents
                  // too.
  };

  /** Invalidate layout for this widget so it will be scheduled for relayout.
          Any change to the size preferences for a widget should call this to
     let parent layout adjust to the change.

          Remarks for layout widgets:
          - When a layout widget get this, it should mark its layout as invalid
     and do the layout later
            (in GetPreferredContentSize/GetPreferredSize are called). If a
     layout knows that no parents will
            be affected, it may stop recursion to parents to avoid unnecessary
     relayout.
          - When setting the size of a layout widget (typically from another
     layout widget or from a OnResize),
            it should be called with InvalidationMode::kTargetOnly to avoid
     recursing back up to parents when
            already recursing down, to avoid unnecessary computation.
          */
  virtual void InvalidateLayout(InvalidationMode il);

  /** Set layout params. Calls InvalidateLayout. */
  void SetLayoutParams(const LayoutParams& lp);

  /** Get layout params, or nullptr if not specified.
          Note: The layout params has already been applied to the PreferredSize
     returned
          from GetPreferredSize so you normally don't need to check these
     params. */
  const LayoutParams* GetLayoutParams() const { return m_layout_params; }

  // == Misc methods for invoking events. Should normally be called only on the
  // root widget ===============

  /** Invoke OnProcess and OnProcessAfterChildren on this widget and its
   * children. */
  void InvokeProcess();

  /** Invoke OnProcessStates on all child widgets, if state processing
          is needed (InvalidateStates() has been called) */
  void InvokeProcessStates(bool force_update = false);

  /** Invoke paint on this widget and all its children */
  void InvokePaint(const PaintProps& parent_paint_props);

  /** Invoke OnFontChanged on this widget and recursively on any children that
   * inherit the font. */
  void InvokeFontChanged();

  /** Invoke a event on this widget.

          This will first check with all registered WidgetListener if the
     event should be dispatched.

          If the widgets OnEvent returns false (event not handled), it will
     continue traversing to
          GetEventDestination (by default the parent) until a widget handles the
     event.

          Note: When invoking event EventType::kChanged, this will update the
     value of other widgets connected
                    to the same group.

          Note: Some event types will automatically invalidate states (See
     InvalidateStates(), InvalidateSkinStates())

          Note: Remember that this widgets may be deleted after this call! So if
     you really must do something after
          this call and are not sure what the event will cause, use
     WeakWidgetPointer to detect self deletion. */
  bool InvokeEvent(TBWidgetEvent& ev);

  bool InvokePointerDown(int x, int y, int click_count,
                         ModifierKeys modifierkeys, bool touch);
  bool InvokePointerUp(int x, int y, ModifierKeys modifierkeys, bool touch);
  void InvokePointerMove(int x, int y, ModifierKeys modifierkeys, bool touch);
  bool InvokeWheel(int x, int y, int delta_x, int delta_y,
                   ModifierKeys modifierkeys);

  /** Invoke the EventType::kKeyDown and EventType::kKeyUp events on the
     currently focused widget.
          This will also do some generic key handling, such as cycling focus on
     tab etc. */
  bool InvokeKey(int key, SpecialKey special_key, ModifierKeys modifierkeys,
                 bool down);

  /** A widget that receive a EventType::kPointerDown event, will stay
     "captured" until EventType::kPointerUp
          is received. While captured, all EventType::kPointerMove are sent to
     it. This method can force release the capture,
          which may happen f.ex if the TBWidget is removed while captured. */
  void ReleaseCapture();

  /** Make x and y (relative to this widget) relative to the upper left corner
   * of the root widget. */
  void ConvertToRoot(int& x, int& y) const;

  /** Make x and y (relative to the upper left corner of the root widget)
   * relative to this widget. */
  void ConvertFromRoot(int& x, int& y) const;

  /** Set the font description for this widget and any children that inherit the
     font.

          Setting a unspecified FontDescription (no changes made since
     construction) means
          it will be inherited from parent (the default).

          Returns true and invokes OnFontChanged on all affected widgets, if the
          font was successfully set.

          Returns false and keep the font onchanged if it no matching font
     exists or fails creation. */
  bool SetFontDescription(const FontDescription& font_desc);

  /** Get the font description as set with SetFontDescription. Use
     GetCalculatedFontDescription()
          to get the calculated font description (Inherit from parent widget
     etc.) */
  FontDescription GetFontDescription() const { return m_font_desc; }

  /** Calculate the font description for this widget. If this widget have
     unspecified font
          description, it will be inheritted from parent. If no parent specify
     any font,
          the default font description will be returned. */
  FontDescription GetCalculatedFontDescription() const;

  /** Get the FontFace for this widget from the current font description
     (calculated
          by GetCalculatedFontDescription) */
  FontFace* GetFont() const;

 private:
  friend class WidgetListener;  ///< It does iteration of m_listeners for us.
  TBWidget* m_parent;           ///< The parent of this widget
  Rect m_rect;  ///< The rectangle of this widget, relative to the parent. See
  /// SetRect.
  TBID m_id;                ///< ID for GetWidgetByID and others.
  TBID m_group_id;          ///< ID for button groups (such as RadioButton)
  TBID m_skin_bg;           ///< ID for the background skin (0 for no skin).
  TBID m_skin_bg_expected;  ///< ID for the background skin after strong
  /// override,
  ///< used to indirect skin changes because of condition changes.
  TBLinkListOf<TBWidget> m_children;         ///< List of child widgets
  TBWidgetValueConnection m_connection;      ///< TBWidget value connection
  TBLinkListOf<WidgetListener> m_listeners;  ///< List of listeners
  float m_opacity;                           ///< Opacity 0-1. See SetOpacity.
  SkinState m_state;  ///< The widget state (excluding any auto states)
  Gravity m_gravity;  ///< The layout gravity setting.
  FontDescription m_font_desc;    ///< The font description.
  PreferredSize m_cached_ps;      ///< Cached preferred size.
  SizeConstraints m_cached_sc;    ///< Cached size constraints.
  LayoutParams* m_layout_params;  ///< Layout params, or nullptr.
  Scroller* m_scroller;
  TBLongClickTimer* m_long_click_timer;
  std::string m_desc_str;
  union {
    struct {
      uint16_t is_group_root : 1;
      uint16_t is_focusable : 1;
      uint16_t click_by_key : 1;
      uint16_t has_key_pressed_state : 1;
      uint16_t ignore_input : 1;
      uint16_t is_dying : 1;
      uint16_t is_cached_ps_valid : 1;
      uint16_t no_automatic_hover_state : 1;
      uint16_t is_panning : 1;
      uint16_t want_long_click : 1;
      uint16_t visibility : 2;
      uint16_t inflate_child_z : 1;  // Should have enough bits to hold WidgetZ
                                     // values.
    } m_packed;
    uint16_t m_packed_init;
  };

 public:
  /** This value is free to use for anything. It's not used by TBWidget itself.
   * Initially Type::kNull. */
  TBValue data;

// Debugging
#ifdef TB_RUNTIME_DEBUG_INFO
  uint64_t last_measure_time;
  uint64_t last_layout_time;
#endif  // TB_RUNTIME_DEBUG_INFO

  // TBWidget related globals
  static TBWidget*
      hovered_widget;  ///< The currently hovered widget, or nullptr.
  static TBWidget*
      captured_widget;  ///< The currently captured widget, or nullptr.
  static TBWidget*
      focused_widget;  ///< The currently focused widget, or nullptr.
  static int pointer_down_widget_x;  ///< Pointer x position on down event,
  /// relative to the captured widget.
  static int pointer_down_widget_y;  ///< Pointer y position on down event,
  /// relative to the captured widget.
  static int pointer_move_widget_x;  ///< Pointer x position on last pointer
  /// event, relative to the captured widget
  ///(if any) or hovered widget.
  static int pointer_move_widget_y;  ///< Pointer y position on last pointer
  /// event, relative to the captured widget
  ///(if any) or hovered widget.
  static bool cancel_click;  ///< true if the pointer up event should not
  /// generate a click event.
  static bool update_widget_states;  ///< true if something has called
  /// InvalidateStates() and it still hasn't
  /// been updated.
  static bool update_skin_states;  ///< true if something has called
  /// InvalidateStates() and skin still hasn't
  /// been updated.
  static bool show_focus_state;  ///< true if the focused state should be
                                 /// painted automatically.
 private:
  /** Return this widget or the nearest parent that is scrollable
          in the given axis, or nullptr if there is none. */
  TBWidget* FindScrollableWidget(bool scroll_x, bool scroll_y);
  Scroller* FindStartedScroller();
  Scroller* GetReadyScroller(bool scroll_x, bool scroll_y);
  TBWidget* GetWidgetByIDInternal(const TBID& id,
                                  const TB_TYPE_ID type_id = nullptr);
  void InvokeSkinUpdatesInternal(bool force_update);
  void InvokeProcessInternal();
  static void SetHoveredWidget(TBWidget* widget, bool touch);
  static void SetCapturedWidget(TBWidget* widget);
  void HandlePanningOnMove(int x, int y);
  void StartLongClickTimer(bool touch);
  void StopLongClickTimer();
  friend class TBLongClickTimer;
  void MaybeInvokeLongClickOrContextMenu(bool touch);
  /** Returns the opacity for this widget multiplied with its skin opacity and
   * state opacity. */
  float CalculateOpacityInternal(SkinState state,
                                 SkinElement* skin_element) const;
};

}  // namespace tb

#endif  // TB_WIDGETS_H
