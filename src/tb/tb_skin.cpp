/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#include "tb_skin.h"

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <string>

#include "tb_node_tree.h"

#include "tb/util/debug.h"
#include "tb/util/metrics.h"
#include "tb/util/string_builder.h"

namespace tb {

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

bool SkinCondition::GetCondition(SkinConditionContext& context) const {
  bool equal = context.GetCondition(m_target, m_info);
  return equal == (m_test == Test::kEqual);
}

Skin::Skin() {
  g_renderer->AddListener(this);

  // Avoid filtering artifacts at edges when we draw fragments stretched.
  m_frag_manager.SetAddBorder(true);
}

bool Skin::Load(const char* skin_file, const char* override_skin_file) {
  if (!LoadInternal(skin_file)) {
    return false;
  }
  if (override_skin_file && !LoadInternal(override_skin_file)) {
    return false;
  }
  return ReloadBitmaps();
}

bool Skin::LoadInternal(const char* skin_file) {
  Node node;
  if (!node.ReadFile(skin_file)) {
    return false;
  }

  StringBuilder skin_path;
  skin_path.AppendPath(skin_file);

  if (node.GetNode("description")) {
    // Check which DPI mode the dimension converter should use.
    // The base-dpi is the dpi in which the padding, spacing (and so on)
    // is specified in. If the skin supports a different DPI that is
    // closer to the screen DPI, all such dimensions will be scaled.
    int base_dpi = node.GetValueInt("description>base-dpi", 96);
    int supported_dpi = base_dpi;
    if (Node* supported_dpi_node = node.GetNode("description>supported-dpi")) {
      assert(supported_dpi_node->GetValue().IsArray() ||
             supported_dpi_node->GetValue().GetInt() == base_dpi);
      if (ValueArray* arr = supported_dpi_node->GetValue().GetArray()) {
        int screen_dpi = util::GetDPI();
        int best_supported_dpi = 0;
        for (int i = 0; i < arr->GetLength(); i++) {
          int candidate_dpi = arr->GetValue(i)->GetInt();
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
  Node* elements = node.GetNode("elements");
  if (elements) {
    Node* n = elements->GetFirstChild();
    while (n) {
      // If we have a "clone" node, clone all children from that node
      // into this node.
      while (Node* clone = n->GetNode("clone")) {
        n->Remove(clone);

        Node* clone_source = elements->GetNode(clone->GetValue().GetString());
        if (clone_source) {
          n->CloneChildren(clone_source);
        }

        delete clone;
      }

      // If the skin element already exist, we will call Load on it again.
      // This will patch the element with any new data from the node.
      TBID element_id(n->GetName());
      SkinElement* e = GetSkinElement(element_id);
      if (!e) {
        e = new SkinElement();
        m_elements.Add(element_id, e);
      }

      e->Load(n, this, skin_path.GetData());
      if (m_listener) {
        m_listener->OnSkinElementLoaded(this, e, n);
      }

      n = n->GetNext();
    }
  }
  return true;
}

void Skin::UnloadBitmaps() {
  // Unset all bitmap pointers.
  util::HashTableIteratorOf<SkinElement> it(&m_elements);
  while (SkinElement* element = it.GetNextContent()) {
    element->bitmap = nullptr;
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
  StringBuilder filename_dst_DPI;
  bool success = true;
  util::HashTableIteratorOf<SkinElement> it(&m_elements);
  while (SkinElement* element = it.GetNextContent()) {
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
            filename_dst_DPI.GetData(), dedicated_map);
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

Skin::~Skin() { g_renderer->RemoveListener(this); }

SkinElement* Skin::GetSkinElement(const TBID& skin_id) const {
  if (!skin_id) return nullptr;
  return m_elements.Get(skin_id);
}

SkinElement* Skin::GetSkinElementStrongOverride(
    const TBID& skin_id, SkinState state, SkinConditionContext& context) const {
  if (SkinElement* skin_element = GetSkinElement(skin_id)) {
    // Avoid eternal recursion when overrides refer to elements referring back.
    if (skin_element->is_getting) {
      return nullptr;
    }
    skin_element->is_getting = true;

    // Check if there's any strong overrides for this element with the given
    // state.
    SkinElementState* override_state =
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
                             SkinState state, SkinConditionContext& context) {
  return PaintSkin(dst_rect, GetSkinElement(skin_id), state, context);
}

SkinElement* Skin::PaintSkin(const Rect& dst_rect, SkinElement* element,
                             SkinState state, SkinConditionContext& context) {
  if (!element || element->is_painting) return nullptr;

  // Avoid potential endless recursion in evil skins.
  element->is_painting = true;

  // Return the override if we have one.
  SkinElement* return_element = element;

  TB_IF_DEBUG(bool paint_error_highlight = false);

  // If there's any override for this state, paint it.
  SkinElementState* override_state =
      element->m_override_elements.GetStateElement(state, context);
  if (override_state) {
    if (SkinElement* used_override =
            PaintSkin(dst_rect, override_state->element_id, state, context)) {
      return_element = used_override;
    } else {
      TB_IF_DEBUG(paint_error_highlight = true);
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
  if (element->m_child_elements.HasStateElements()) {
    const SkinElementState* state_element =
        element->m_child_elements.GetFirstElement();
    while (state_element) {
      if (state_element->IsMatch(state, context)) {
        PaintSkin(dst_rect, state_element->element_id,
                  state_element->state & state, context);
      }
      state_element = state_element->GetNext();
    }
  }

  // Paint ugly rectangles on invalid skin elements in debug builds.
  TB_IF_DEBUG(if (paint_error_highlight) g_renderer->DrawRect(
      dst_rect.Expand(1, 1), Color(255, 205, 0)));
  TB_IF_DEBUG(if (paint_error_highlight) g_renderer->DrawRect(
      dst_rect.Shrink(1, 1), Color(255, 0, 0)));

  element->is_painting = false;
  return return_element;
}

void Skin::PaintSkinOverlay(const Rect& dst_rect, SkinElement* element,
                            SkinState state, SkinConditionContext& context) {
  if (!element || element->is_painting) return;

  // Avoid potential endless recursion in evil skins.
  element->is_painting = true;

  // Paint all overlay elements that matches the state (or should be painted for
  // all states).
  const SkinElementState* state_element =
      element->m_overlay_elements.GetFirstElement();
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
  g_renderer->DrawRectFill(dst_rect, element->bg_color);
}

void Skin::PaintElementImage(const Rect& dst_rect, SkinElement* element) {
  Rect src_rect(0, 0, element->bitmap->Width(), element->bitmap->Height());
  Rect rect = dst_rect.Expand(element->expand, element->expand);
  rect.reset(rect.x + element->img_ofs_x +
                 (rect.w - src_rect.w) * element->img_position_x / 100,
             rect.y + element->img_ofs_y +
                 (rect.h - src_rect.h) * element->img_position_y / 100,
             src_rect.w, src_rect.h);
  g_renderer->DrawBitmap(rect, GetFlippedRect(src_rect, element),
                         element->bitmap);
}

void Skin::PaintElementTile(const Rect& dst_rect, SkinElement* element) {
  Rect rect = dst_rect.Expand(element->expand, element->expand);
  g_renderer->DrawBitmapTile(rect, element->bitmap->GetBitmap());
}

void Skin::PaintElementStretchImage(const Rect& dst_rect,
                                    SkinElement* element) {
  if (dst_rect.empty()) return;
  Rect rect = dst_rect.Expand(element->expand, element->expand);
  Rect src_rect = GetFlippedRect(
      Rect(0, 0, element->bitmap->Width(), element->bitmap->Height()), element);
  g_renderer->DrawBitmap(rect, src_rect, element->bitmap);
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
  int bw = element->bitmap->Width();
  int bh = element->bitmap->Height();

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
  g_renderer->DrawBitmap(Rect(rect.x, rect.y, dst_cut_w, dst_cut_h),
                         Rect(0, 0, cut, cut), element->bitmap);
  g_renderer->DrawBitmap(
      Rect(rect.x + rect.w - dst_cut_w, rect.y, dst_cut_w, dst_cut_h),
      Rect(bw - cut, 0, cut, cut), element->bitmap);
  g_renderer->DrawBitmap(
      Rect(rect.x, rect.y + rect.h - dst_cut_h, dst_cut_w, dst_cut_h),
      Rect(0, bh - cut, cut, cut), element->bitmap);
  g_renderer->DrawBitmap(
      Rect(rect.x + rect.w - dst_cut_w, rect.y + rect.h - dst_cut_h, dst_cut_w,
           dst_cut_h),
      Rect(bw - cut, bh - cut, cut, cut), element->bitmap);

  // Left & right edge.
  if (has_left_right_edges) {
    g_renderer->DrawBitmap(
        Rect(rect.x, rect.y + dst_cut_h, dst_cut_w, rect.h - dst_cut_h * 2),
        Rect(0, cut, cut, bh - cut * 2), element->bitmap);
    g_renderer->DrawBitmap(Rect(rect.x + rect.w - dst_cut_w, rect.y + dst_cut_h,
                                dst_cut_w, rect.h - dst_cut_h * 2),
                           Rect(bw - cut, cut, cut, bh - cut * 2),
                           element->bitmap);
  }

  // Top & bottom edge.
  if (has_top_bottom_edges) {
    g_renderer->DrawBitmap(
        Rect(rect.x + dst_cut_w, rect.y, rect.w - dst_cut_w * 2, dst_cut_h),
        Rect(cut, 0, bw - cut * 2, cut), element->bitmap);
    g_renderer->DrawBitmap(Rect(rect.x + dst_cut_w, rect.y + rect.h - dst_cut_h,
                                rect.w - dst_cut_w * 2, dst_cut_h),
                           Rect(cut, bh - cut, bw - cut * 2, cut),
                           element->bitmap);
  }

  // Center.
  if (fill_center && has_top_bottom_edges && has_left_right_edges) {
    g_renderer->DrawBitmap(Rect(rect.x + dst_cut_w, rect.y + dst_cut_h,
                                rect.w - dst_cut_w * 2, rect.h - dst_cut_h * 2),
                           Rect(cut, cut, bw - cut * 2, bh - cut * 2),
                           element->bitmap);
  }
}

#ifdef TB_RUNTIME_DEBUG_INFO
void Skin::Debug() { m_frag_manager.Debug(); }
#endif  // TB_RUNTIME_DEBUG_INFO

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

int Skin::GetPxFromNode(Node* node, int def_value) const {
  return node ? m_dim_conv.GetPxFromValue(&node->GetValue(), def_value)
              : def_value;
}

SkinElement::SkinElement() = default;

SkinElement::~SkinElement() = default;

int SkinElement::GetIntrinsicMinWidth() const {
  // Sizes below the skin cut size would start to shrink the skin below pretty,
  // so assume that's the default minimum size if it's not specified (minus
  // expansion).
  return cut * 2 - expand * 2;
}

int SkinElement::GetIntrinsicMinHeight() const {
  // Sizes below the skin cut size would start to shrink the skin below pretty,
  // so assume that's the default minimum size if it's not specified (minus
  // expansion).
  return cut * 2 - expand * 2;
}

int SkinElement::GetIntrinsicWidth() const {
  if (width != kSkinValueNotSpecified) {
    return width;
  }
  if (bitmap) {
    return bitmap->Width() - expand * 2;
  }
  // FIX: We may want to check child elements etc.
  return kSkinValueNotSpecified;
}

int SkinElement::GetIntrinsicHeight() const {
  if (height != kSkinValueNotSpecified) {
    return height;
  }
  if (bitmap) {
    return bitmap->Height() - expand * 2;
  }
  // FIX: We may want to check child elements etc.
  return kSkinValueNotSpecified;
}

void SkinElement::SetBitmapDPI(const DimensionConverter& dim_conv,
                               int bitmap_dpi) {
  if (this->bitmap_dpi) {
    // We have already applied the modifications so abort. This may
    // happen when we reload bitmaps without reloading the skin.
    return;
  }
  if (dim_conv.NeedConversion()) {
    if (bitmap_dpi == dim_conv.GetDstDPI()) {
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
  this->bitmap_dpi = bitmap_dpi;
}

bool SkinElement::HasState(SkinState state, SkinConditionContext& context) {
  return m_override_elements.GetStateElement(
             state, context, SkinElementState::MatchRule::kOnlySpecificState) ||
         m_child_elements.GetStateElement(
             state, context, SkinElementState::MatchRule::kOnlySpecificState) ||
         m_overlay_elements.GetStateElement(
             state, context, SkinElementState::MatchRule::kOnlySpecificState);
}

void SkinElement::Load(Node* n, Skin* skin, const char* skin_path) {
  if (const char* bitmap = n->GetValueString("bitmap", nullptr)) {
    bitmap_file.clear();
    bitmap_file.append(skin_path);
    bitmap_file.append(bitmap);
  }

  // Note: Always read cut and expand as pixels. These values might later be
  //       recalculated depending on the DPI the bitmaps are available in.
  cut = n->GetValueInt("cut", cut);
  expand = n->GetValueInt("expand", expand);

  name = n->GetName();
  id = n->GetName();

  const DimensionConverter* dim_conv = skin->GetDimensionConverter();

  if (Node* padding_node = n->GetNode("padding")) {
    Value& val = padding_node->GetValue();
    if (val.GetArrayLength() == 4) {
      padding_top = dim_conv->GetPxFromValue(val.GetArray()->GetValue(0), 0);
      padding_right = dim_conv->GetPxFromValue(val.GetArray()->GetValue(1), 0);
      padding_bottom = dim_conv->GetPxFromValue(val.GetArray()->GetValue(2), 0);
      padding_left = dim_conv->GetPxFromValue(val.GetArray()->GetValue(3), 0);
    } else if (val.GetArrayLength() == 2) {
      padding_top = padding_bottom =
          dim_conv->GetPxFromValue(val.GetArray()->GetValue(0), 0);
      padding_left = padding_right =
          dim_conv->GetPxFromValue(val.GetArray()->GetValue(1), 0);
    } else {
      padding_top = padding_right = padding_bottom = padding_left =
          dim_conv->GetPxFromValue(&val, 0);
    }
  }
  width = skin->GetPxFromNode(n->GetNode("width"), width);
  height = skin->GetPxFromNode(n->GetNode("height"), height);
  pref_width = skin->GetPxFromNode(n->GetNode("pref-width"), pref_width);
  pref_height = skin->GetPxFromNode(n->GetNode("pref-height"), pref_height);
  min_width = skin->GetPxFromNode(n->GetNode("min-width"), min_width);
  min_height = skin->GetPxFromNode(n->GetNode("min-height"), min_height);
  max_width = skin->GetPxFromNode(n->GetNode("max-width"), max_width);
  max_height = skin->GetPxFromNode(n->GetNode("max-height"), max_height);
  spacing = skin->GetPxFromNode(n->GetNode("spacing"), spacing);
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

bool SkinElementState::IsMatch(SkinState state, SkinConditionContext& context,
                               MatchRule rule) const {
  if (rule == MatchRule::kOnlySpecificState && this->state == SkinState::kAll) {
    return false;
  }
  if (any(state & this->state) || this->state == SkinState::kAll) {
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

bool SkinElementState::IsExactMatch(SkinState state,
                                    SkinConditionContext& context,
                                    MatchRule rule) const {
  if (rule == MatchRule::kOnlySpecificState && this->state == SkinState::kAll) {
    return false;
  }
  if (state == this->state || this->state == SkinState::kAll) {
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
    SkinState state, SkinConditionContext& context,
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
    SkinState state, SkinConditionContext& context,
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

void SkinElementStateList::Load(Node* n) {
  if (!n) return;

  // For each node, create a new state element.
  Node* element_node = n->GetFirstChild();
  while (element_node) {
    SkinElementState* state = new SkinElementState();

    // By default, a state element applies to all combinations of states.
    state->state = SkinState::kAll;
    state->element_id.Set(element_node->GetValue().GetString());

    // Loop through all nodes, read state and create all found conditions.
    for (Node* condition_node = element_node->GetFirstChild(); condition_node;
         condition_node = condition_node->GetNext()) {
      if (strcmp(condition_node->GetName(), "state") == 0) {
        state->state = StringToState(condition_node->GetValue().GetString());
      } else if (strcmp(condition_node->GetName(), "condition") == 0) {
        auto target = from_string(condition_node->GetValueString("target", ""),
                                  SkinTarget::kThis);

        const char* prop_str = condition_node->GetValueString("property", "");
        auto prop = from_string(prop_str, SkinProperty::kCustom);
        TBID custom_prop;
        if (prop == SkinProperty::kCustom) {
          custom_prop.Set(prop_str);
        }

        TBID value;
        if (Node* value_n = condition_node->GetNode("value")) {
          // Set the it to number or string. If it's a state, we must first
          // convert the state string to the SkinState state combo.
          if (prop == SkinProperty::kState) {
            value.Set(uint32_t(StringToState(value_n->GetValue().GetString())));
          } else if (value_n->GetValue().IsString()) {
            value.Set(value_n->GetValue().GetString());
          } else {
            value.Set(value_n->GetValue().GetInt());
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

}  // namespace tb
