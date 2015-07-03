/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#ifndef TB_SKIN_H
#define TB_SKIN_H

#include "tb_bitmap_fragment.h"
#include "tb_core.h"
#include "tb_dimension.h"
#include "tb_hashtable.h"
#include "tb_linklist.h"
#include "tb_renderer.h"
#include "tb_value.h"

namespace tb {

class TBNode;
class SkinConditionContext;

// Used for some values in SkinElement if they has not been specified in the
// skin.
constexpr int kSkinValueNotSpecified = kInvalidDimension;

// Skin state types (may be combined).
enum class SkinState {
  kNone = 0,
  kDisabled = 1,
  kFocused = 2,
  kPressed = 4,
  kSelected = 8,
  kHovered = 16,

  kAll = kDisabled | kFocused | kPressed | kSelected | kHovered,
};
MAKE_ENUM_FLAG_COMBO(SkinState);

// Type of painting that should be done for a SkinElement.
enum class SkinElementType {
  kStretchBox,
  kStretchBorder,
  kStretchImage,
  kTile,
  kImage,
};
MAKE_ORDERED_ENUM_STRING_UTILS(SkinElementType, "stretch box", "stretch border",
                               "stretch image", "tile", "image");

// Defines which target(s) relative to the context that should be tested for
// the condition.
enum class SkinTarget {
  kThis,         // The object painting the skin.
  kParent,       // The parent of the object painting the skin.
  kAncestors,    // All ancestors of the object painting the skin.
  kPrevSibling,  // The previous sibling of the object painting the skin.
  kNextSibling,  // The next sibling of the object painting the skin.
};
MAKE_ORDERED_ENUM_STRING_UTILS(SkinTarget, "this", "parent", "ancestors",
                               "prev sibling", "next sibling");

// Defines which property in the context that should be checked.
enum class SkinProperty {
  kSkin,          // The background skin id.
  kWindowActive,  // The window is active (no value required).
  kAxis,          // The axis of the content (x or y)
  kAlign,         // The alignment.
  kId,            // The id.
  kState,         // The state is set.
  kValue,         // The current value (integer).
  kHover,         // Focus is on the target or any child (no value required).
  kCapture,       // Capture is on the target or any child (no value required).
  kFocus,         // Focus is on the target or any child (no value required).
  kCustom,  // It's a property unknown to skin, that the SkinConditionContext
            // might know about.
};
MAKE_ORDERED_ENUM_STRING_UTILS(SkinProperty, "skin", "window active", "axis",
                               "align", "id", "state", "value", "hover",
                               "capture", "focus", "custom");

// Checks if a condition is true for a given SkinConditionContext.
// This is used to apply different state elements depending on what is currently
// painting the skin.
class SkinCondition : public TBLinkOf<SkinCondition> {
 public:
  // Defines if the condition tested should be equal or not for the condition to
  // be true.
  enum class Test {
    kEqual,     // Value should be equal for condition to be true.
    kNotEqual,  // Value should not be equal for condition to be true.
  };

  // Stores the information needed for checking a condition.
  struct ConditionInfo {
    SkinProperty prop;  // Which property.
    TBID custom_prop;   // Which property (if prop is SkinProperty::kCustom).
    TBID value;         // The value to compare.
  };

  SkinCondition(SkinTarget target, SkinProperty prop, const TBID& custom_prop,
                const TBID& value, Test test);

  // Returns true if the condition is true for the given context.
  bool GetCondition(SkinConditionContext& context) const;

 private:
  SkinTarget m_target;
  ConditionInfo m_info;
  Test m_test;
};

// Checks if a condition is true.
// It is passed to skin painting functions so different state elements can be
// applied depending on the current situation of the context.
// F.ex a widget may change appearance if it's under a parent with a certain
// skin.
class SkinConditionContext {
 public:
  // Returns true if the given target and property equals the given value.
  virtual bool GetCondition(SkinTarget target,
                            const SkinCondition::ConditionInfo& info) = 0;
};

// SkinElementState has a skin element id that should be used if its state and
// condition matches that which is being painted.
class SkinElementState : public TBLinkOf<SkinElementState> {
 public:
  // Defines how to match states.
  enum class MatchRule {
    // States with "all" (SkinState::kAll) will also be considered a match.
    kDefault,
    // States with "all" will not be considered a match.
    kOnlySpecificState,
  };

  bool IsMatch(SkinState state, SkinConditionContext& context,
               MatchRule rule = MatchRule::kDefault) const;

  bool IsExactMatch(SkinState state, SkinConditionContext& context,
                    MatchRule rule = MatchRule::kDefault) const;

  TBID element_id;
  SkinState state;
  TBLinkListAutoDeleteOf<SkinCondition> conditions;
};

// List of state elements in a SkinElement.
class SkinElementStateList {
 public:
  ~SkinElementStateList();

  SkinElementState* GetStateElement(
      SkinState state, SkinConditionContext& context,
      SkinElementState::MatchRule rule =
          SkinElementState::MatchRule::kDefault) const;

  SkinElementState* GetStateElementExactMatch(
      SkinState state, SkinConditionContext& context,
      SkinElementState::MatchRule rule =
          SkinElementState::MatchRule::kDefault) const;

  bool HasStateElements() const { return m_state_elements.HasLinks(); }
  const SkinElementState* GetFirstElement() const {
    return m_state_elements.GetFirst();
  }

  void Load(TBNode* n);

 private:
  TBLinkListOf<SkinElementState> m_state_elements;
};

// Skin element.
// Contains a bitmap fragment (or nullptr) and info specifying how it should be
// painted.
// Also contains padding and other look-specific widget properties.
class SkinElement {
 public:
  SkinElement();
  ~SkinElement();

  TBID id;           // ID of the skin element.
  std::string name;  // Name of the skin element, f.ex "SelectDropdown.arrow".
  std::string bitmap_file;  // File name of the bitmap (might be empty).
  BitmapFragment* bitmap =
      nullptr;      // Bitmap fragment containing the graphics, or nullptr.
  uint8_t cut = 0;  // How the bitmap should be sliced using StretchBox.
  int16_t expand =
      0;  // How much the skin should expand outside the widgets rect.
  SkinElementType type = SkinElementType::kStretchBox;  // Skin element type.
  bool is_painting =
      false;  // If the skin is being painted (avoiding eternal recursing).
  bool is_getting =
      false;  // If the skin is being got (avoiding eternal recursion).
  int16_t padding_left = 0;    // Left padding for any content in the element.
  int16_t padding_top = 0;     // Top padding for any content in the element.
  int16_t padding_right = 0;   // Right padding for any content in the element.
  int16_t padding_bottom = 0;  // Bottom padding for any content in the element.
  int16_t width =
      kSkinValueNotSpecified;  // Intrinsic width or kSkinValueNotSpecified
  int16_t height =
      kSkinValueNotSpecified;  // Intrinsic height or kSkinValueNotSpecified
  int16_t pref_width =
      kSkinValueNotSpecified;  // Preferred width or kSkinValueNotSpecified
  int16_t pref_height =
      kSkinValueNotSpecified;  // Preferred height or kSkinValueNotSpecified
  int16_t min_width =
      kSkinValueNotSpecified;  // Minimum width or kSkinValueNotSpecified
  int16_t min_height =
      kSkinValueNotSpecified;  // Minimum height or kSkinValueNotSpecified
  int16_t max_width =
      kSkinValueNotSpecified;  // Maximum width or kSkinValueNotSpecified
  int16_t max_height =
      kSkinValueNotSpecified;  // Maximum height or kSkinValueNotSpecified
  int16_t spacing = kSkinValueNotSpecified;  // Spacing used on layout or
                                             // kSkinValueNotSpecified.
  int16_t content_ofs_x = 0;  // X offset of the content in the widget.
  int16_t content_ofs_y = 0;  // Y offset of the content in the widget.
  int16_t img_ofs_x = 0;  // X offset for type image. Relative to image position
                          // (img_position_x).
  int16_t img_ofs_y = 0;  // Y offset for type image. Relative to image position
                          // (img_position_y).
  int8_t img_position_x = 50;  // Horizontal position for type image. 0-100
                               // (left to right in available space). Default
                               // 50.
  int8_t img_position_y = 50;  // Vertical position for type image. 0-100 (top
                               // to bottom in available space). Default 50.
  int8_t flip_x = 0;           // The skin is flipped horizontally.
  int8_t flip_y = 0;           // The skin is flipped vertically.
  float opacity =
      1.0f;  // Opacity that should be used for the whole widget (0.f - 1.f).
  Color text_color = Color(0, 0, 0, 0);  // Color of the text in the widget.
  Color bg_color = Color(0, 0, 0, 0);  // Color of the background in the widget.
  int16_t bitmap_dpi = 0;              // The DPI of the bitmap that was loaded.
  TBValue
      tag;  // This value is free to use for anything. It's not used internally.

  // Gets the minimum width, or kSkinValueNotSpecified if not specified.
  int GetMinWidth() const { return min_width; }

  // Gets the minimum height, or kSkinValueNotSpecified if not specified.
  int GetMinHeight() const { return min_height; }

  // Gets the intrinsic minimum width. It will be calculated based on the skin
  // properties.
  int GetIntrinsicMinWidth() const;

  // Gets the intrinsic minimum height. It will be calculated based on the skin
  // properties.
  int GetIntrinsicMinHeight() const;

  // Gets the maximum width, or kSkinValueNotSpecified if not specified.
  int GetMaxWidth() const { return max_width; }

  // Gets the maximum height, or kSkinValueNotSpecified if not specified.
  int GetMaxHeight() const { return max_height; }

  // Gets the preferred width, or kSkinValueNotSpecified if not specified.
  int GetPrefWidth() const { return pref_width; }

  // Gets the preferred height, or kSkinValueNotSpecified if not specified.
  int GetPrefHeight() const { return pref_height; }

  // Gets the intrinsic width. If not specified using the "width" attribute, it
  // will be calculated based on the skin properties. If it can't be calculated
  // it will return kSkinValueNotSpecified.
  int GetIntrinsicWidth() const;

  // Gets the intrinsic height. If not specified using the "height" attribute,
  // it will be calculated based on the skin properties. If it can't be
  // calculated it will return kSkinValueNotSpecified.
  int GetIntrinsicHeight() const;

  // Sets the DPI that the bitmap was loaded in. This may modify properties to
  // compensate for the bitmap resolution.
  void SetBitmapDPI(const DimensionConverter& dim_conv, int bitmap_dpi);

  // List of override elements (See Skin::PaintSkin).
  SkinElementStateList m_override_elements;

  // List of strong-override elements (See Skin::PaintSkin).
  SkinElementStateList m_strong_override_elements;

  // List of child elements (See Skin::PaintSkin).
  SkinElementStateList m_child_elements;

  // List of overlay elements (See Skin::PaintSkin).
  SkinElementStateList m_overlay_elements;

  // Checks if there's a exact or partial match for the given state in either
  // override, child or overlay element list. State elements with state "all"
  // will be ignored.
  bool HasState(SkinState state, SkinConditionContext& context);

  // Returns true if this element has overlay elements.
  bool HasOverlayElements() const {
    return m_overlay_elements.HasStateElements();
  }

  void Load(TBNode* n, Skin* skin, const char* skin_path);
};

class SkinListener {
 public:
  // Called when a skin element has been loaded from the given TBNode.
  // NOTE: this may be called multiple times on elements that occur multiple
  // times in the skin or is overridden in an override skin.
  // This method can be used to f.ex feed custom properties into element->tag.
  virtual void OnSkinElementLoaded(Skin* skin, SkinElement* element,
                                   TBNode* node) = 0;
};

// Skin contains a list of SkinElement.
class Skin : private RendererListener {
 public:
  Skin();
  ~Skin() override;

  void SetListener(SkinListener* listener) { m_listener = listener; }
  SkinListener* GetListener() const { return m_listener; }

  // Loads the skin file and the bitmaps it refers to.
  // If override_skin_file is specified, it will also be loaded into this skin
  // after loading skin_file. Elements using the same name will override any
  // previosly read data for the same element.
  // Known limitation: Clone can currently only clone elements in the same file!
  // Returns true on success, and all bitmaps referred to also loaded
  // successfully.
  bool Load(const char* skin_file, const char* override_skin_file = nullptr);

  // Unloads all bitmaps used in this skin.
  void UnloadBitmaps();

  // Reloads all bitmaps used in this skin. Calls UnloadBitmaps first to ensure
  // no bitmaps are loaded before loading new ones.
  bool ReloadBitmaps();

  // Gets the dimension converter used for the current skin. This dimension
  // converter converts to px by the same factor as the skin (based on the skin
  // DPI settings).
  const DimensionConverter* GetDimensionConverter() const {
    return &m_dim_conv;
  }

  // Gets the skin element with the given id.
  // Returns nullptr if there's no match.
  SkinElement* GetSkinElement(const TBID& skin_id) const;

  // Gets the skin element with the given id and state.
  // This is like calling GetSkinElement and also following any strong overrides
  // that match the current state (if any). See details about strong overrides
  // in PaintSkin.
  // Returns nullptr if there's no match.
  SkinElement* GetSkinElementStrongOverride(
      const TBID& skin_id, SkinState state,
      SkinConditionContext& context) const;

  Color GetDefaultTextColor() const { return m_default_text_color; }
  float GetDefaultDisabledOpacity() const { return m_default_disabled_opacity; }
  float GetDefaultPlaceholderOpacity() const {
    return m_default_placeholder_opacity;
  }
  int GetDefaultSpacing() const { return m_default_spacing; }

  // Paints the skin at dst_rect.
  //
  // Strong override elements:
  //   - Strong override elements are like override elements, but they don't
  //     only apply when painting. They also override padding and other things
  //     that might affect the layout of the widget having the skin set.
  //
  // Override elements:
  //   - If there is a override element with the exact matching state, it will
  //     paint the override *instead* if the base skin. If no exact match was
  //     found, it will check for a partial match and paint that *instead* of
  //     the base skin.
  //
  // Child elements:
  //   - It will paint *all* child elements that match the current state ("all"
  //     can be specified as state so it will always be painted). The elements
  //     are painted in the order they are specified in the skin.
  //
  // Special elements:
  //   - There's some special generic skin elements used by Widget (see
  //     Widget::SetSkinBg).
  //
  // Overlay elements:
  //   - Overlay elements are painted separately, from PaintSkinOverlay (when
  //     all sibling widgets has been painted). As with child elements, all
  //     overlay elements that match the current state will be painted in the
  //     order they are specified in the skin.
  //
  // Returns the skin element used (after following override elements), or
  // nullptr if no skin element was found matching the skin_id.
  SkinElement* PaintSkin(const Rect& dst_rect, const TBID& skin_id,
                         SkinState state, SkinConditionContext& context);

  // Paints the skin at dst_rect. Just like the PaintSkin above, but takes a
  // specific skin element instead of looking it up from the id.
  SkinElement* PaintSkin(const Rect& dst_rect, SkinElement* element,
                         SkinState state, SkinConditionContext& context);

  // Paints the overlay elements for the given skin element and state.
  void PaintSkinOverlay(const Rect& dst_rect, SkinElement* element,
                        SkinState state, SkinConditionContext& context);

#ifdef TB_RUNTIME_DEBUG_INFO
  // Renders the skin bitmaps on screen, to analyze fragment positioning.
  void Debug();
#endif  // TB_RUNTIME_DEBUG_INFO

  BitmapFragmentManager* GetFragmentManager() { return &m_frag_manager; }

  void OnContextLost() override;
  void OnContextRestored() override;

 private:
  friend class SkinElement;

  bool LoadInternal(const char* skin_file);
  bool ReloadBitmapsInternal();
  void PaintElement(const Rect& dst_rect, SkinElement* element);
  void PaintElementBGColor(const Rect& dst_rect, SkinElement* element);
  void PaintElementImage(const Rect& dst_rect, SkinElement* element);
  void PaintElementTile(const Rect& dst_rect, SkinElement* element);
  void PaintElementStretchImage(const Rect& dst_rect, SkinElement* element);
  void PaintElementStretchBox(const Rect& dst_rect, SkinElement* element,
                              bool fill_center);
  Rect GetFlippedRect(const Rect& src_rect, SkinElement* element) const;
  int GetPxFromNode(TBNode* node, int def_value) const;

  SkinListener* m_listener = nullptr;
  TBHashTableAutoDeleteOf<SkinElement> m_elements;
  BitmapFragmentManager m_frag_manager;
  DimensionConverter m_dim_conv;
  Color m_default_text_color;
  float m_default_disabled_opacity = 0.3f;
  float m_default_placeholder_opacity = 0.2f;
  int16_t m_default_spacing = 0;
};

}  // namespace tb

#endif  // TB_SKIN_H
