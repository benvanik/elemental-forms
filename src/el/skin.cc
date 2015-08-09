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
#include <cstdio>
#include <string>

#include "el/parsing/parse_node.h"
#include "el/skin.h"
#include "el/util/debug.h"
#include "el/util/metrics.h"
#include "el/util/string_builder.h"

namespace el {

using graphics::Renderer;
using parsing::ParseNode;

std::unique_ptr<Skin> Skin::skin_singleton_;

SkinState StringToState(const char* state_str) {
  SkinState state = SkinState::kNone;
  if (strstr(state_str, "all")) state |= SkinState::kAll;
  if (strstr(state_str, "disabled")) state |= SkinState::kDisabled;
  if (strstr(state_str, "focused")) state |= SkinState::kFocused;
  if (strstr(state_str, "pressed")) state |= SkinState::kPressed;
  if (strstr(state_str, "selected")) state |= SkinState::kSelected;
  if (strstr(state_str, "hovered")) state |= SkinState::kHovered;
  return state;
}

SkinCondition::SkinCondition(SkinTarget target, SkinProperty prop,
                             const TBID& custom_prop, const TBID& value,
                             Test test)
    : m_target(target), m_test(test) {
  m_info.prop = prop;
  m_info.custom_prop = custom_prop;
  m_info.value = value;
}

bool SkinCondition::GetCondition(const SkinConditionContext& context) const {
  bool equal = context.GetCondition(m_target, m_info);
  return equal == (m_test == Test::kEqual);
}

Skin::Skin() {
  Renderer::get()->AddListener(this);

  // Avoid filtering artifacts at edges when we draw fragments stretched.
  m_frag_manager.set_has_border(true);
}

bool Skin::Load(const char* skin_file) {
  if (!LoadInternal(skin_file)) {
    return false;
  }
  return ReloadBitmaps();
}

bool Skin::LoadInternal(const char* skin_file) {
  ParseNode node;
  if (!node.ReadFile(skin_file)) {
    return false;
  }

  util::StringBuilder skin_path;
  skin_path.AppendPath(skin_file);

  if (node.GetNode("description")) {
    // Check which DPI mode the dimension converter should use.
    // The base-dpi is the dpi in which the padding, spacing (and so on)
    // is specified in. If the skin supports a different DPI that is
    // closer to the screen DPI, all such dimensions will be scaled.
    int base_dpi = node.GetValueInt("description>base-dpi", 96);
    int supported_dpi = base_dpi;
    if (ParseNode* supported_dpi_node =
            node.GetNode("description>supported-dpi")) {
      assert(supported_dpi_node->value().is_array() ||
             supported_dpi_node->value().as_integer() == base_dpi);
      if (ValueArray* arr = supported_dpi_node->value().as_array()) {
        int screen_dpi = util::GetDPI();
        int best_supported_dpi = 0;
        for (size_t i = 0; i < arr->size(); ++i) {
          int candidate_dpi = arr->at(i)->as_integer();
          if (!best_supported_dpi ||
              std::abs(candidate_dpi - screen_dpi) <
                  std::abs(best_supported_dpi - screen_dpi)) {
            best_supported_dpi = candidate_dpi;
          }
        }
        supported_dpi = best_supported_dpi;
      }
    }
    m_dim_conv.SetDPI(base_dpi, supported_dpi);
  }

  // Read skin constants.
  if (const char* color = node.GetValueString("defaults>text-color", nullptr)) {
    m_default_text_color.reset(color);
  }
  m_default_disabled_opacity = node.GetValueFloat("defaults>disabled>opacity",
                                                  m_default_disabled_opacity);
  m_default_placeholder_opacity = node.GetValueFloat(
      "defaults>placeholder>opacity", m_default_placeholder_opacity);
  m_default_spacing =
      GetPxFromNode(node.GetNode("defaults>spacing"), m_default_spacing);

  // Iterate through all elements nodes and add skin elements or patch already
  // existing elements.
  ParseNode* elements = node.GetNode("elements");
  if (!elements) {
    return true;
  }
  ParseNode* n = elements->first_child();
  while (n) {
    // If we have a "clone" node, clone all children from that node
    // into this node.
    while (ParseNode* clone = n->GetNode("clone")) {
      n->Remove(clone);

      ParseNode* clone_source = elements->GetNode(clone->value().as_string());
      if (clone_source) {
        n->CloneChildren(clone_source);
      }

      delete clone;
    }

    // If the skin element already exist, we will call Load on it again.
    // This will patch the element with any new data from the node.
    TBID element_id(n->name());
    SkinElement* element = GetSkinElementById(element_id);
    if (!element) {
      element = new SkinElement();
      m_elements.emplace(element_id, std::unique_ptr<SkinElement>(element));
    }

    element->Load(n, this, skin_path.c_str());
    if (m_listener) {
      m_listener->OnSkinElementLoaded(this, element, n);
    }

    n = n->GetNext();
  }
  return true;
}

void Skin::UnloadBitmaps() {
  // Unset all bitmap pointers.
  for (auto& it : m_elements) {
    it.second->bitmap = nullptr;
  }

  // Clear all fragments and bitmaps.
  m_frag_manager.Clear();
}

bool Skin::ReloadBitmaps() {
  UnloadBitmaps();
  bool success = ReloadBitmapsInternal();
  // Create all bitmaps for the bitmap fragment maps.
  if (success) {
    success = m_frag_manager.ValidateBitmaps();
  }

  TBDebugOut("Skin loaded using %dll bitmaps.\n", m_frag_manager.map_count());
  return success;
}

bool Skin::ReloadBitmapsInternal() {
  // Load all bitmap files into new bitmap fragments.
  util::StringBuilder filename_dst_DPI;
  bool success = true;
  for (auto& it : m_elements) {
    auto element = it.second.get();
    if (!element->bitmap_file.empty()) {
      assert(!element->bitmap);

      // FIX: dedicated_map is not needed for all backends (only deprecated
      // fixed function GL).
      // TODO(benvanik): fix shaders/etc to properly repeat subregions?
      // This will force a new, empty map to be created just for tiled textures.
      bool dedicated_map = element->type == SkinElementType::kTile;

      // Try to load bitmap fragment in the destination DPI (F.ex "foo.png"
      // becomes "foo@192.png")
      int bitmap_dpi = m_dim_conv.GetSrcDPI();
      if (m_dim_conv.NeedConversion()) {
        m_dim_conv.GetDstDPIFilename(element->bitmap_file, &filename_dst_DPI);
        element->bitmap = m_frag_manager.GetFragmentFromFile(
            filename_dst_DPI.c_str(), dedicated_map);
        if (element->bitmap) {
          bitmap_dpi = m_dim_conv.GetDstDPI();
        }
      }
      element->SetBitmapDPI(m_dim_conv, bitmap_dpi);

      // If we still have no bitmap fragment, load from default file.
      if (!element->bitmap) {
        element->bitmap = m_frag_manager.GetFragmentFromFile(
            element->bitmap_file, dedicated_map);
      }

      if (!element->bitmap) {
        success = false;
      }
    }
  }
  return success;
}

Skin::~Skin() { Renderer::get()->RemoveListener(this); }

SkinElement* Skin::GetSkinElementById(const TBID& skin_id) const {
  if (!skin_id) return nullptr;
  auto it = m_elements.find(skin_id);
  return it != m_elements.end() ? it->second.get() : nullptr;
}

SkinElement* Skin::GetSkinElementStrongOverride(
    const TBID& skin_id, SkinState state,
    const SkinConditionContext& context) const {
  if (SkinElement* skin_element = GetSkinElementById(skin_id)) {
    // Avoid eternal recursion when overrides refer to elements referring back.
    if (skin_element->is_getting) {
      return nullptr;
    }
    skin_element->is_getting = true;

    // Check if there's any strong overrides for this element with the given
    // state.
    auto override_state =
        skin_element->m_strong_override_elements.GetStateElement(state,
                                                                 context);
    if (override_state) {
      if (SkinElement* override_element = GetSkinElementStrongOverride(
              override_state->element_id, state, context)) {
        skin_element->is_getting = false;
        return override_element;
      }
    }

    skin_element->is_getting = false;
    return skin_element;
  }
  return nullptr;
}

SkinElement* Skin::PaintSkin(const Rect& dst_rect, const TBID& skin_id,
                             SkinState state,
                             const SkinConditionContext& context) {
  return PaintSkin(dst_rect, GetSkinElementById(skin_id), state, context);
}

SkinElement* Skin::PaintSkin(const Rect& dst_rect, SkinElement* element,
                             SkinState state,
                             const SkinConditionContext& context) {
  if (!element || element->is_painting) return nullptr;

  // Avoid potential endless recursion in evil skins.
  element->is_painting = true;

  // Return the override if we have one.
  SkinElement* return_element = element;

  EL_IF_DEBUG(bool paint_error_highlight = false);

  // If there's any override for this state, paint it.
  auto override_state =
      element->m_override_elements.GetStateElement(state, context);
  if (override_state) {
    if (SkinElement* used_override =
            PaintSkin(dst_rect, override_state->element_id, state, context)) {
      return_element = used_override;
    } else {
      EL_IF_DEBUG(paint_error_highlight = true);
      TBDebugOut(
          "Skin error: The skin references a missing element, or has a "
          "reference loop!\n");
      // Fall back to the standard skin.
      override_state = nullptr;
    }
  }

  // If there was no override, paint the standard skin element.
  if (!override_state) {
    PaintElement(dst_rect, element);
  }

  // Paint all child elements that matches the state (or should be painted for
  // all states).
  if (element->m_child_elements.has_state_elements()) {
    auto state_element = element->m_child_elements.first_element();
    while (state_element) {
      if (state_element->IsMatch(state, context)) {
        PaintSkin(dst_rect, state_element->element_id,
                  state_element->state & state, context);
      }
      state_element = state_element->GetNext();
    }
  }

  // Paint ugly rectangles on invalid skin elements in debug builds.
  EL_IF_DEBUG(if (paint_error_highlight) Renderer::get()->DrawRect(
      dst_rect.Expand(1, 1), Color(255, 205, 0)));
  EL_IF_DEBUG(if (paint_error_highlight) Renderer::get()->DrawRect(
      dst_rect.Shrink(1, 1), Color(255, 0, 0)));

  element->is_painting = false;
  return return_element;
}

void Skin::PaintSkinOverlay(const Rect& dst_rect, SkinElement* element,
                            SkinState state,
                            const SkinConditionContext& context) {
  if (!element || element->is_painting) return;

  // Avoid potential endless recursion in evil skins.
  element->is_painting = true;

  // Paint all overlay elements that matches the state (or should be painted for
  // all states).
  auto state_element = element->m_overlay_elements.first_element();
  while (state_element) {
    if (state_element->IsMatch(state, context)) {
      PaintSkin(dst_rect, state_element->element_id,
                state_element->state & state, context);
    }
    state_element = state_element->GetNext();
  }

  element->is_painting = false;
}

void Skin::PaintElement(const Rect& dst_rect, SkinElement* element) {
  PaintElementBGColor(dst_rect, element);
  if (!element->bitmap) return;
  if (element->type == SkinElementType::kImage) {
    PaintElementImage(dst_rect, element);
  } else if (element->type == SkinElementType::kTile) {
    PaintElementTile(dst_rect, element);
  } else if (element->type == SkinElementType::kStretchImage ||
             element->cut == 0) {
    PaintElementStretchImage(dst_rect, element);
  } else if (element->type == SkinElementType::kStretchBorder) {
    PaintElementStretchBox(dst_rect, element, false);
  } else {
    PaintElementStretchBox(dst_rect, element, true);
  }
}

Rect Skin::GetFlippedRect(const Rect& src_rect, SkinElement* element) const {
  // Turning the source rect "inside out" will flip the result when rendered.
  Rect tmp_rect = src_rect;
  if (element->flip_x) {
    tmp_rect.x += tmp_rect.w;
    tmp_rect.w = -tmp_rect.w;
  }
  if (element->flip_y) {
    tmp_rect.y += tmp_rect.h;
    tmp_rect.h = -tmp_rect.h;
  }
  return tmp_rect;
}

void Skin::PaintElementBGColor(const Rect& dst_rect, SkinElement* element) {
  if (element->bg_color == 0) return;
  Renderer::get()->DrawRectFill(dst_rect, element->bg_color);
}

void Skin::PaintElementImage(const Rect& dst_rect, SkinElement* element) {
  Rect src_rect(0, 0, element->bitmap->width(), element->bitmap->height());
  Rect rect = dst_rect.Expand(element->expand, element->expand);
  rect.reset(rect.x + element->img_ofs_x +
                 (rect.w - src_rect.w) * element->img_position_x / 100,
             rect.y + element->img_ofs_y +
                 (rect.h - src_rect.h) * element->img_position_y / 100,
             src_rect.w, src_rect.h);
  Renderer::get()->DrawBitmap(rect, GetFlippedRect(src_rect, element),
                              element->bitmap);
}

void Skin::PaintElementTile(const Rect& dst_rect, SkinElement* element) {
  Rect rect = dst_rect.Expand(element->expand, element->expand);
  Renderer::get()->DrawBitmapTile(rect, element->bitmap->GetBitmap());
}

void Skin::PaintElementStretchImage(const Rect& dst_rect,
                                    SkinElement* element) {
  if (dst_rect.empty()) return;
  Rect rect = dst_rect.Expand(element->expand, element->expand);
  Rect src_rect = GetFlippedRect(
      Rect(0, 0, element->bitmap->width(), element->bitmap->height()), element);
  Renderer::get()->DrawBitmap(rect, src_rect, element->bitmap);
}

void Skin::PaintElementStretchBox(const Rect& dst_rect, SkinElement* element,
                                  bool fill_center) {
  if (dst_rect.empty()) {
    return;
  }

  Rect rect = dst_rect.Expand(element->expand, element->expand);

  // Stretch the dst_cut (if rect is smaller than the skin size).
  // FIX: the expand should also be stretched!
  int cut = element->cut;
  int dst_cut_w = std::min(cut, rect.w / 2);
  int dst_cut_h = std::min(cut, rect.h / 2);
  int bw = element->bitmap->width();
  int bh = element->bitmap->height();

  bool has_left_right_edges = rect.h > dst_cut_h * 2;
  bool has_top_bottom_edges = rect.w > dst_cut_w * 2;

  rect = GetFlippedRect(rect, element);
  if (element->flip_x) {
    dst_cut_w = -dst_cut_w;
  }
  if (element->flip_y) {
    dst_cut_h = -dst_cut_h;
  }

  // Corners.
  Renderer::get()->DrawBitmap(Rect(rect.x, rect.y, dst_cut_w, dst_cut_h),
                              Rect(0, 0, cut, cut), element->bitmap);
  Renderer::get()->DrawBitmap(
      Rect(rect.x + rect.w - dst_cut_w, rect.y, dst_cut_w, dst_cut_h),
      Rect(bw - cut, 0, cut, cut), element->bitmap);
  Renderer::get()->DrawBitmap(
      Rect(rect.x, rect.y + rect.h - dst_cut_h, dst_cut_w, dst_cut_h),
      Rect(0, bh - cut, cut, cut), element->bitmap);
  Renderer::get()->DrawBitmap(
      Rect(rect.x + rect.w - dst_cut_w, rect.y + rect.h - dst_cut_h, dst_cut_w,
           dst_cut_h),
      Rect(bw - cut, bh - cut, cut, cut), element->bitmap);

  // Left & right edge.
  if (has_left_right_edges) {
    Renderer::get()->DrawBitmap(
        Rect(rect.x, rect.y + dst_cut_h, dst_cut_w, rect.h - dst_cut_h * 2),
        Rect(0, cut, cut, bh - cut * 2), element->bitmap);
    Renderer::get()->DrawBitmap(
        Rect(rect.x + rect.w - dst_cut_w, rect.y + dst_cut_h, dst_cut_w,
             rect.h - dst_cut_h * 2),
        Rect(bw - cut, cut, cut, bh - cut * 2), element->bitmap);
  }

  // Top & bottom edge.
  if (has_top_bottom_edges) {
    Renderer::get()->DrawBitmap(
        Rect(rect.x + dst_cut_w, rect.y, rect.w - dst_cut_w * 2, dst_cut_h),
        Rect(cut, 0, bw - cut * 2, cut), element->bitmap);
    Renderer::get()->DrawBitmap(
        Rect(rect.x + dst_cut_w, rect.y + rect.h - dst_cut_h,
             rect.w - dst_cut_w * 2, dst_cut_h),
        Rect(cut, bh - cut, bw - cut * 2, cut), element->bitmap);
  }

  // Center.
  if (fill_center && has_top_bottom_edges && has_left_right_edges) {
    Renderer::get()->DrawBitmap(
        Rect(rect.x + dst_cut_w, rect.y + dst_cut_h, rect.w - dst_cut_w * 2,
             rect.h - dst_cut_h * 2),
        Rect(cut, cut, bw - cut * 2, bh - cut * 2), element->bitmap);
  }
}

int GetFadeoutSize(int scrolled_distance, int fadeout_length) {
  // Make it appear gradually.
  // float factor = scrolled_distance / 10.f;
  // factor = Clamp(factor, 0.5f, 1);
  // return (int)(fadeout_length * factor);
  return scrolled_distance > 0 ? fadeout_length : 0;
}

void Skin::DrawEdgeFadeout(const Rect& dst_rect, TBID skin_x, TBID skin_y,
                           int left, int top, int right, int bottom) {
  if (auto skin = Skin::get()->GetSkinElementById(skin_x)) {
    if (skin->bitmap) {
      int bw = skin->bitmap->width();
      int bh = skin->bitmap->height();
      int dw;
      if ((dw = GetFadeoutSize(left, bw)) > 0) {
        Renderer::get()->DrawBitmap(
            Rect(dst_rect.x, dst_rect.y, dw, dst_rect.h), Rect(0, 0, bw, bh),
            skin->bitmap);
      }
      if ((dw = GetFadeoutSize(right, bw)) > 0) {
        Renderer::get()->DrawBitmap(
            Rect(dst_rect.x + dst_rect.w - dw, dst_rect.y, dw, dst_rect.h),
            Rect(bw, 0, -bw, bh), skin->bitmap);
      }
    }
  }
  if (auto skin = Skin::get()->GetSkinElementById(skin_y)) {
    if (skin->bitmap) {
      int bw = skin->bitmap->width();
      int bh = skin->bitmap->height();
      int dh;
      if ((dh = GetFadeoutSize(top, bh)) > 0) {
        Renderer::get()->DrawBitmap(
            Rect(dst_rect.x, dst_rect.y, dst_rect.w, dh), Rect(0, 0, bw, bh),
            skin->bitmap);
      }
      if ((dh = GetFadeoutSize(bottom, bh)) > 0) {
        Renderer::get()->DrawBitmap(
            Rect(dst_rect.x, dst_rect.y + dst_rect.h - dh, dst_rect.w, dh),
            Rect(0, bh, bw, -bh), skin->bitmap);
      }
    }
  }
}

#ifdef EL_RUNTIME_DEBUG_INFO
void Skin::Debug() { m_frag_manager.Debug(); }
#endif  // EL_RUNTIME_DEBUG_INFO

void Skin::OnContextLost() {
  // We could simply do: m_frag_manager.DeleteBitmaps() and then all bitmaps
  // would be recreated automatically when needed. But because it's easy,
  // we unload everything so we save some memory (by not keeping any image
  // data around).
  UnloadBitmaps();
}

void Skin::OnContextRestored() {
  // Reload bitmaps (since we unloaded everything in OnContextLost()).
  ReloadBitmaps();
}

int Skin::GetPxFromNode(ParseNode* node, int def_value) const {
  return node ? m_dim_conv.GetPxFromValue(&node->value(), def_value)
              : def_value;
}

SkinElement::SkinElement() = default;

SkinElement::~SkinElement() = default;

int SkinElement::intrinsic_min_width() const {
  // Sizes below the skin cut size would start to shrink the skin below pretty,
  // so assume that's the default minimum size if it's not specified (minus
  // expansion).
  return cut * 2 - expand * 2;
}

int SkinElement::intrinsic_min_height() const {
  // Sizes below the skin cut size would start to shrink the skin below pretty,
  // so assume that's the default minimum size if it's not specified (minus
  // expansion).
  return cut * 2 - expand * 2;
}

int SkinElement::intrinsic_width() const {
  if (width_ != kSkinValueNotSpecified) {
    return width_;
  }
  if (bitmap) {
    return bitmap->width() - expand * 2;
  }
  // FIX: We may want to check child elements etc.
  return kSkinValueNotSpecified;
}

int SkinElement::intrinsic_height() const {
  if (height_ != kSkinValueNotSpecified) {
    return height_;
  }
  if (bitmap) {
    return bitmap->height() - expand * 2;
  }
  // FIX: We may want to check child elements etc.
  return kSkinValueNotSpecified;
}

void SkinElement::SetBitmapDPI(const util::DimensionConverter& dim_conv,
                               int new_bitmap_dpi) {
  if (bitmap_dpi) {
    // We have already applied the modifications so abort. This may
    // happen when we reload bitmaps without reloading the skin.
    return;
  }
  if (dim_conv.NeedConversion()) {
    if (new_bitmap_dpi == dim_conv.GetDstDPI()) {
      // The bitmap was loaded in a different DPI than the base DPI so
      // we must scale the bitmap properties.
      expand = expand * dim_conv.GetDstDPI() / dim_conv.GetSrcDPI();
      cut = cut * dim_conv.GetDstDPI() / dim_conv.GetSrcDPI();
    } else {
      // The bitmap was loaded in the base DPI and we need to scale it.
      // Apply the DPI conversion to the skin element scale factor.
      // FIX: For this to work well, we would need to apply scale to both
      //      image and all the other types of drawing too.
      // scale_x = scale_x * dim_conv.GetDstDPI() / dim_conv.GetSrcDPI();
      // scale_y = scale_y * dim_conv.GetDstDPI() / dim_conv.GetSrcDPI();
    }
  }
  bitmap_dpi = new_bitmap_dpi;
}

bool SkinElement::has_state(SkinState state,
                            const SkinConditionContext& context) const {
  return m_override_elements.GetStateElement(
             state, context, SkinElementState::MatchRule::kOnlySpecificState) ||
         m_child_elements.GetStateElement(
             state, context, SkinElementState::MatchRule::kOnlySpecificState) ||
         m_overlay_elements.GetStateElement(
             state, context, SkinElementState::MatchRule::kOnlySpecificState);
}

void SkinElement::Load(ParseNode* n, Skin* skin, const char* skin_path) {
  if (auto bitmap_path = n->GetValueString("bitmap", nullptr)) {
    bitmap_file.clear();
    bitmap_file.append(skin_path);
    bitmap_file.append(bitmap_path);
  }

  // Note: Always read cut and expand as pixels. These values might later be
  //       recalculated depending on the DPI the bitmaps are available in.
  cut = n->GetValueInt("cut", cut);
  expand = n->GetValueInt("expand", expand);

  name = n->name();
  id = n->name();

  auto dim_conv = skin->dimension_converter();

  if (ParseNode* padding_node = n->GetNode("padding")) {
    Value& val = padding_node->value();
    if (val.array_size() == 4) {
      padding_top = dim_conv->GetPxFromValue(val.as_array()->at(0), 0);
      padding_right = dim_conv->GetPxFromValue(val.as_array()->at(1), 0);
      padding_bottom = dim_conv->GetPxFromValue(val.as_array()->at(2), 0);
      padding_left = dim_conv->GetPxFromValue(val.as_array()->at(3), 0);
    } else if (val.array_size() == 2) {
      padding_top = padding_bottom =
          dim_conv->GetPxFromValue(val.as_array()->at(0), 0);
      padding_left = padding_right =
          dim_conv->GetPxFromValue(val.as_array()->at(1), 0);
    } else {
      padding_top = padding_right = padding_bottom = padding_left =
          dim_conv->GetPxFromValue(&val, 0);
    }
  }
  width_ = skin->GetPxFromNode(n->GetNode("width"), width_);
  height_ = skin->GetPxFromNode(n->GetNode("height"), height_);
  pref_width_ = skin->GetPxFromNode(n->GetNode("pref-width"), pref_width_);
  pref_height_ = skin->GetPxFromNode(n->GetNode("pref-height"), pref_height_);
  min_width_ = skin->GetPxFromNode(n->GetNode("min-width"), min_width_);
  min_height_ = skin->GetPxFromNode(n->GetNode("min-height"), min_height_);
  max_width_ = skin->GetPxFromNode(n->GetNode("max-width"), max_width_);
  max_height_ = skin->GetPxFromNode(n->GetNode("max-height"), max_height_);
  spacing_ = skin->GetPxFromNode(n->GetNode("spacing"), spacing_);
  content_ofs_x =
      skin->GetPxFromNode(n->GetNode("content-ofs-x"), content_ofs_x);
  content_ofs_y =
      skin->GetPxFromNode(n->GetNode("content-ofs-y"), content_ofs_y);
  img_position_x = n->GetValueInt("img-position-x", img_position_x);
  img_position_y = n->GetValueInt("img-position-y", img_position_y);
  img_ofs_x = skin->GetPxFromNode(n->GetNode("img-ofs-x"), img_ofs_x);
  img_ofs_y = skin->GetPxFromNode(n->GetNode("img-ofs-y"), img_ofs_y);
  flip_x = n->GetValueInt("flip-x", flip_x);
  flip_y = n->GetValueInt("flip-y", flip_y);
  opacity = n->GetValueFloat("opacity", opacity);

  if (const char* color = n->GetValueString("text-color", nullptr)) {
    text_color.reset(color);
  }

  if (const char* color = n->GetValueString("background-color", nullptr)) {
    bg_color.reset(color);
  }

  if (const char* type_str = n->GetValueString("type", nullptr)) {
    type = from_string(type_str, type);
  }

  // Create all state elements.
  m_override_elements.Load(n->GetNode("overrides"));
  m_strong_override_elements.Load(n->GetNode("strong-overrides"));
  m_child_elements.Load(n->GetNode("children"));
  m_overlay_elements.Load(n->GetNode("overlays"));
}

bool SkinElementState::IsMatch(SkinState test_state,
                               const SkinConditionContext& context,
                               MatchRule rule) const {
  if (rule == MatchRule::kOnlySpecificState && state == SkinState::kAll) {
    return false;
  }
  if (any(test_state & state) || state == SkinState::kAll) {
    for (SkinCondition* condition = conditions.GetFirst(); condition;
         condition = condition->GetNext()) {
      if (!condition->GetCondition(context)) {
        return false;
      }
    }
    return true;
  }
  return false;
}

bool SkinElementState::IsExactMatch(SkinState test_state,
                                    const SkinConditionContext& context,
                                    MatchRule rule) const {
  if (rule == MatchRule::kOnlySpecificState && state == SkinState::kAll) {
    return false;
  }
  if (test_state == state || state == SkinState::kAll) {
    for (SkinCondition* condition = conditions.GetFirst(); condition;
         condition = condition->GetNext()) {
      if (!condition->GetCondition(context)) {
        return false;
      }
    }
    return true;
  }
  return false;
}

SkinElementStateList::~SkinElementStateList() {
  while (SkinElementState* state = m_state_elements.GetFirst()) {
    m_state_elements.Remove(state);
    delete state;
  }
}

SkinElementState* SkinElementStateList::GetStateElement(
    SkinState state, const SkinConditionContext& context,
    SkinElementState::MatchRule rule) const {
  // First try to get a state element with a exact match to the current state.
  if (SkinElementState* element_state =
          GetStateElementExactMatch(state, context, rule)) {
    return element_state;
  }
  // No exact state match. Get a state with a partly match if there is one.
  SkinElementState* state_element = m_state_elements.GetFirst();
  while (state_element) {
    if (state_element->IsMatch(state, context, rule)) {
      return state_element;
    }
    state_element = state_element->GetNext();
  }
  return nullptr;
}

SkinElementState* SkinElementStateList::GetStateElementExactMatch(
    SkinState state, const SkinConditionContext& context,
    SkinElementState::MatchRule rule) const {
  SkinElementState* state_element = m_state_elements.GetFirst();
  while (state_element) {
    if (state_element->IsExactMatch(state, context, rule)) {
      return state_element;
    }
    state_element = state_element->GetNext();
  }
  return nullptr;
}

void SkinElementStateList::Load(ParseNode* n) {
  if (!n) return;

  // For each node, create a new state element.
  ParseNode* element_node = n->first_child();
  while (element_node) {
    SkinElementState* state = new SkinElementState();

    // By default, a state element applies to all combinations of states.
    state->state = SkinState::kAll;
    state->element_id.reset(element_node->value().as_string());

    // Loop through all nodes, read state and create all found conditions.
    for (ParseNode* condition_node = element_node->first_child();
         condition_node; condition_node = condition_node->GetNext()) {
      if (strcmp(condition_node->name(), "state") == 0) {
        state->state = StringToState(condition_node->value().as_string());
      } else if (strcmp(condition_node->name(), "condition") == 0) {
        auto target = from_string(condition_node->GetValueString("target", ""),
                                  SkinTarget::kThis);

        const char* prop_str = condition_node->GetValueString("property", "");
        auto prop = from_string(prop_str, SkinProperty::kCustom);
        TBID custom_prop;
        if (prop == SkinProperty::kCustom) {
          custom_prop.reset(prop_str);
        }

        TBID value;
        if (ParseNode* value_n = condition_node->GetNode("value")) {
          // Set the it to number or string. If it's a state, we must first
          // convert the state string to the SkinState state combo.
          if (prop == SkinProperty::kState) {
            value.reset(uint32_t(StringToState(value_n->value().as_string())));
          } else if (value_n->value().is_string()) {
            value.reset(value_n->value().as_string());
          } else {
            value.reset(value_n->value().as_integer());
          }
        }

        auto test = SkinCondition::Test::kEqual;
        if (const char* test_str =
                condition_node->GetValueString("test", nullptr)) {
          if (strcmp(test_str, "!=") == 0) {
            test = SkinCondition::Test::kNotEqual;
          }
        }

        SkinCondition* condition =
            new SkinCondition(target, prop, custom_prop, value, test);
        state->conditions.AddLast(condition);
      }
    }

    // State is reado to add.
    m_state_elements.AddLast(state);
    element_node = element_node->GetNext();
  }
}

}  // namespace el
