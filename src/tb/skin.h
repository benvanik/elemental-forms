/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#ifndef TB_SKIN_H_
#define TB_SKIN_H_

#include <memory>
#include <unordered_map>

#include "tb/graphics/bitmap_fragment.h"
#include "tb/graphics/bitmap_fragment_manager.h"
#include "tb/graphics/renderer.h"
#include "tb/types.h"
#include "tb/util/dimension_converter.h"
#include "tb/util/intrusive_list.h"
#include "tb/value.h"

namespace tb {

class Skin;
class SkinConditionContext;
namespace parsing {
class ParseNode;
}  // namespace parsing

// Used for some values in SkinElement if they has not been specified in the
// skin.
constexpr int kSkinValueNotSpecified = util::kInvalidDimension;

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
inline std::string to_string(SkinState value) {
  if (value == SkinState::kNone) {
    return "none";
  } else if (value == SkinState::kAll) {
    return "all";
  }
  std::string result;
  if (any(value & SkinState::kDisabled)) {
    result += " disabled";
  }
  if (any(value & SkinState::kFocused)) {
    result += " focused";
  }
  if (any(value & SkinState::kPressed)) {
    result += " pressed";
  }
  if (any(value & SkinState::kSelected)) {
    result += " selected";
  }
  if (any(value & SkinState::kHovered)) {
    result += " hovered";
  }
  return result.substr(1);
}

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
class SkinCondition : public util::IntrusiveListEntry<SkinCondition> {
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
// F.ex a element may change appearance if it's under a parent with a certain
// skin.
class SkinConditionContext {
 public:
  // Returns true if the given target and property equals the given value.
  virtual bool GetCondition(SkinTarget target,
                            const SkinCondition::ConditionInfo& info) = 0;
};

// SkinElementState has a skin element id that should be used if its state and
// condition matches that which is being painted.
class SkinElementState : public util::IntrusiveListEntry<SkinElementState> {
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
  util::AutoDeleteIntrusiveList<SkinCondition> conditions;
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

  bool has_state_elements() const { return m_state_elements.HasLinks(); }
  const SkinElementState* first_element() const {
    return m_state_elements.GetFirst();
  }

  void Load(parsing::ParseNode* n);

 private:
  util::IntrusiveList<SkinElementState> m_state_elements;
};

// Skin element.
// Contains a bitmap fragment (or nullptr) and info specifying how it should be
// painted.
// Also contains padding and other look-specific element properties.
class SkinElement {
 public:
  SkinElement();
  ~SkinElement();

  TBID id;           // ID of the skin element.
  std::string name;  // Name of the skin element, f.ex "DropDownButton.arrow".
  std::string bitmap_file;  // File name of the bitmap (might be empty).
  graphics::BitmapFragment* bitmap =
      nullptr;      // Bitmap fragment containing the graphics, or nullptr.
  uint8_t cut = 0;  // How the bitmap should be sliced using StretchBox.
  int16_t expand =
      0;  // How much the skin should expand outside the elements rect.
  SkinElementType type = SkinElementType::kStretchBox;  // Skin element type.
  bool is_painting =
      false;  // If the skin is being painted (avoiding eternal recursing).
  bool is_getting =
      false;  // If the skin is being got (avoiding eternal recursion).
  int16_t padding_left = 0;    // Left padding for any content in the element.
  int16_t padding_top = 0;     // Top padding for any content in the element.
  int16_t padding_right = 0;   // Right padding for any content in the element.
  int16_t padding_bottom = 0;  // Bottom padding for any content in the element.
  int16_t content_ofs_x = 0;   // X offset of the content in the element.
  int16_t content_ofs_y = 0;   // Y offset of the content in the element.
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
      1.0f;  // Opacity that should be used for the whole element (0.f - 1.f).
  Color text_color = Color(0, 0, 0, 0);  // Color of the text in the element.
  Color bg_color =
      Color(0, 0, 0, 0);   // Color of the background in the element.
  int16_t bitmap_dpi = 0;  // The DPI of the bitmap that was loaded.
  Value
      tag;  // This value is free to use for anything. It's not used internally.

  // Gets the minimum width, or kSkinValueNotSpecified if not specified.
  int min_width() const { return min_width_; }
  // Gets the minimum height, or kSkinValueNotSpecified if not specified.
  int min_height() const { return min_height_; }

  // Gets the intrinsic minimum width. It will be calculated based on the skin
  // properties.
  int intrinsic_min_width() const;
  // Gets the intrinsic minimum height. It will be calculated based on the skin
  // properties.
  int intrinsic_min_height() const;

  // Gets the maximum width, or kSkinValueNotSpecified if not specified.
  int max_width() const { return max_width_; }
  // Gets the maximum height, or kSkinValueNotSpecified if not specified.
  int max_height() const { return max_height_; }

  // Gets the preferred width, or kSkinValueNotSpecified if not specified.
  int preferred_width() const { return pref_width_; }
  // Gets the preferred height, or kSkinValueNotSpecified if not specified.
  int preferred_height() const { return pref_height_; }

  // Gets the intrinsic width. If not specified using the "width" attribute, it
  // will be calculated based on the skin properties. If it can't be calculated
  // it will return kSkinValueNotSpecified.
  int intrinsic_width() const;
  // Gets the intrinsic height. If not specified using the "height" attribute,
  // it will be calculated based on the skin properties. If it can't be
  // calculated it will return kSkinValueNotSpecified.
  int intrinsic_height() const;

  // Gets the spacing used on layout or kSkinValueNotSpecified.
  int spacing() const { return spacing_; }

  // Sets the DPI that the bitmap was loaded in. This may modify properties to
  // compensate for the bitmap resolution.
  void SetBitmapDPI(const util::DimensionConverter& dim_conv, int bitmap_dpi);

  // Checks if there's a exact or partial match for the given state in either
  // override, child or overlay element list. State elements with state "all"
  // will be ignored.
  bool has_state(SkinState state, SkinConditionContext& context);

  // Returns true if this element has overlay elements.
  bool has_overlay_elements() const {
    return m_overlay_elements.has_state_elements();
  }

  void Load(parsing::ParseNode* n, Skin* skin, const char* skin_path);

 private:
  int16_t width_ = kSkinValueNotSpecified;
  int16_t height_ = kSkinValueNotSpecified;
  int16_t pref_width_ = kSkinValueNotSpecified;
  int16_t pref_height_ = kSkinValueNotSpecified;
  int16_t min_width_ = kSkinValueNotSpecified;
  int16_t min_height_ = kSkinValueNotSpecified;
  int16_t max_width_ = kSkinValueNotSpecified;
  int16_t max_height_ = kSkinValueNotSpecified;
  int16_t spacing_ = kSkinValueNotSpecified;

 private:
  friend class Skin;

  // List of override elements (See Skin::PaintSkin).
  SkinElementStateList m_override_elements;

  // List of strong-override elements (See Skin::PaintSkin).
  SkinElementStateList m_strong_override_elements;

  // List of child elements (See Skin::PaintSkin).
  SkinElementStateList m_child_elements;

  // List of overlay elements (See Skin::PaintSkin).
  SkinElementStateList m_overlay_elements;
};

class SkinListener {
 public:
  // Called when a skin element has been loaded from the given ParseNode.
  // NOTE: this may be called multiple times on elements that occur multiple
  // times in the skin or is overridden in an override skin.
  // This method can be used to f.ex feed custom properties into element->tag.
  virtual void OnSkinElementLoaded(Skin* skin, SkinElement* element,
                                   parsing::ParseNode* node) = 0;
};

// Skin contains a list of SkinElement.
class Skin : private graphics::RendererListener {
 public:
  static Skin* get() { return skin_singleton_.get(); }
  static void set(std::unique_ptr<Skin> value) {
    skin_singleton_ = std::move(value);
  }

  Skin();
  ~Skin() override;

  SkinListener* listener() const { return m_listener; }
  void set_listener(SkinListener* listener) { m_listener = listener; }

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
  const util::DimensionConverter* dimension_converter() const {
    return &m_dim_conv;
  }

  // Gets the skin element with the given id.
  // Returns nullptr if there's no match.
  SkinElement* GetSkinElementById(const TBID& skin_id) const;

  // Gets the skin element with the given id and state.
  // This is like calling GetSkinElement and also following any strong overrides
  // that match the current state (if any). See details about strong overrides
  // in PaintSkin.
  // Returns nullptr if there's no match.
  SkinElement* GetSkinElementStrongOverride(
      const TBID& skin_id, SkinState state,
      SkinConditionContext& context) const;

  Color default_text_color() const { return m_default_text_color; }
  float default_disabled_opacity() const { return m_default_disabled_opacity; }
  float default_placeholder_opacity() const {
    return m_default_placeholder_opacity;
  }
  int default_spacing() const { return m_default_spacing; }

  // Paints the skin at dst_rect.
  //
  // Strong override elements:
  //   - Strong override elements are like override elements, but they don't
  //     only apply when painting. They also override padding and other things
  //     that might affect the layout of the element having the skin set.
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
  //   - There's some special generic skin elements used by Element (see
  //     Element::SetSkinBg).
  //
  // Overlay elements:
  //   - Overlay elements are painted separately, from PaintSkinOverlay (when
  //     all sibling elements has been painted). As with child elements, all
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

  // Draw fade out skin elements at the edges of dst_rect if needed.
  // It indicates to the user that there is hidden content.
  // left, top, right, bottom specifies the (positive) distance scrolled from
  // the limit.
  static void DrawEdgeFadeout(const Rect& dst_rect, TBID skin_x, TBID skin_y,
                              int left, int top, int right, int bottom);

#ifdef TB_RUNTIME_DEBUG_INFO
  // Renders the skin bitmaps on screen, to analyze fragment positioning.
  void Debug();
#endif  // TB_RUNTIME_DEBUG_INFO

  graphics::BitmapFragmentManager* fragment_manager() {
    return &m_frag_manager;
  }

  void OnContextLost() override;
  void OnContextRestored() override;

 private:
  friend class SkinElement;

  static std::unique_ptr<Skin> skin_singleton_;

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
  int GetPxFromNode(parsing::ParseNode* node, int def_value) const;

  SkinListener* m_listener = nullptr;
  std::unordered_map<uint32_t, std::unique_ptr<SkinElement>> m_elements;
  graphics::BitmapFragmentManager m_frag_manager;
  util::DimensionConverter m_dim_conv;
  Color m_default_text_color;
  float m_default_disabled_opacity = 0.3f;
  float m_default_placeholder_opacity = 0.2f;
  int16_t m_default_spacing = 0;
};

}  // namespace tb

#endif  // TB_SKIN_H_
