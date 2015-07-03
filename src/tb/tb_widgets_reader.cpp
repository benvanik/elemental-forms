/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#include "tb_widgets_reader.h"

#include "tb_font_renderer.h"
#include "tb_image_widget.h"
#include "tb_inline_select.h"
#include "tb_node_tree.h"
#include "tb_scroll_container.h"
#include "tb_select.h"
#include "tb_tab_container.h"
#include "tb_text_box.h"
#include "tb_toggle_container.h"
#include "tb_widgets_common.h"

namespace tb {

TB_WIDGET_FACTORY(Element, Value::Type::kNull, ElementZ::kTop) {}
void Element::OnInflate(const InflateInfo& info) {
  ElementReader::SetIDFromNode(GetID(), info.node->GetNode("id"));
  ElementReader::SetIDFromNode(GetGroupID(), info.node->GetNode("group-id"));

  if (info.sync_type == Value::Type::kFloat) {
    SetValueDouble(info.node->GetValueFloat("value", 0));
  } else {
    SetValue(info.node->GetValueInt("value", 0));
  }

  if (Node* data_node = info.node->GetNode("data")) {
    data.Copy(data_node->GetValue());
  }

  SetIsGroupRoot(
      info.node->GetValueInt("is-group-root", GetIsGroupRoot()) ? true : false);

  SetIsFocusable(
      info.node->GetValueInt("is-focusable", GetIsFocusable()) ? true : false);

  SetWantLongClick(info.node->GetValueInt("want-long-click", GetWantLongClick())
                       ? true
                       : false);

  SetIgnoreInput(
      info.node->GetValueInt("ignore-input", GetIgnoreInput()) ? true : false);

  SetOpacity(info.node->GetValueFloat("opacity", GetOpacity()));

  if (const char* text = info.node->GetValueString("text", nullptr)) {
    SetText(text);
  }

  if (const char* connection =
          info.node->GetValueStringRaw("connection", nullptr)) {
    // If we already have a element value with this name, just connect to it and
    // the element will adjust its value to it. Otherwise create a new element
    // value, and give it the value we got from the resource.
    if (ElementValue* value = g_value_group.GetValue(connection)) {
      Connect(value);
    } else if (ElementValue* value = g_value_group.CreateValueIfNeeded(
                   connection, info.sync_type)) {
      value->SetFromElement(this);
      Connect(value);
    }
  }
  if (const char* axis = info.node->GetValueString("axis", nullptr)) {
    SetAxis(tb::from_string(axis, Axis::kY));
  }
  if (const char* gravity = info.node->GetValueString("gravity", nullptr)) {
    Gravity g = Gravity::kNone;
    if (strstr(gravity, "left")) g |= Gravity::kLeft;
    if (strstr(gravity, "top")) g |= Gravity::kTop;
    if (strstr(gravity, "right")) g |= Gravity::kRight;
    if (strstr(gravity, "bottom")) g |= Gravity::kBottom;
    if (strstr(gravity, "all")) g |= Gravity::kAll;
    if (!any(g & Gravity::kLeftRight)) g |= Gravity::kLeft;
    if (!any(g & Gravity::kTopBottom)) g |= Gravity::kTop;
    SetGravity(g);
  }
  if (const char* visibility =
          info.node->GetValueString("visibility", nullptr)) {
    SetVisibilility(from_string(visibility, GetVisibility()));
  }
  if (const char* state = info.node->GetValueString("state", nullptr)) {
    if (strstr(state, "disabled")) {
      SetState(SkinState::kDisabled, true);
    }
  }
  if (const char* skin = info.node->GetValueString("skin", nullptr)) {
    SetSkinBg(skin);
  }
  if (Node* lp = info.node->GetNode("lp")) {
    LayoutParams layout_params;
    if (GetLayoutParams()) {
      layout_params = *GetLayoutParams();
    }
    const DimensionConverter* dc = g_tb_skin->GetDimensionConverter();
    if (const char* str = lp->GetValueString("width", nullptr)) {
      layout_params.set_width(
          dc->GetPxFromString(str, LayoutParams::kUnspecified));
    }
    if (const char* str = lp->GetValueString("height", nullptr)) {
      layout_params.set_height(
          dc->GetPxFromString(str, LayoutParams::kUnspecified));
    }
    if (const char* str = lp->GetValueString("min-width", nullptr)) {
      layout_params.min_w =
          dc->GetPxFromString(str, LayoutParams::kUnspecified);
    }
    if (const char* str = lp->GetValueString("max-width", nullptr)) {
      layout_params.max_w =
          dc->GetPxFromString(str, LayoutParams::kUnspecified);
    }
    if (const char* str = lp->GetValueString("pref-width", nullptr)) {
      layout_params.pref_w =
          dc->GetPxFromString(str, LayoutParams::kUnspecified);
    }
    if (const char* str = lp->GetValueString("min-height", nullptr)) {
      layout_params.min_h =
          dc->GetPxFromString(str, LayoutParams::kUnspecified);
    }
    if (const char* str = lp->GetValueString("max-height", nullptr)) {
      layout_params.max_h =
          dc->GetPxFromString(str, LayoutParams::kUnspecified);
    }
    if (const char* str = lp->GetValueString("pref-height", nullptr)) {
      layout_params.pref_h =
          dc->GetPxFromString(str, LayoutParams::kUnspecified);
    }
    SetLayoutParams(layout_params);
  }

  SetTooltip(info.node->GetValueString("tooltip", nullptr));

  // Add the new element to the hiearchy if not already added.
  if (!GetParent()) info.target->AddChild(this, info.target->GetZInflate());

  // Read the font now when the element is in the hiearchy so inheritance works.
  if (Node* font = info.node->GetNode("font")) {
    FontDescription fd = GetCalculatedFontDescription();
    if (const char* size = font->GetValueString("size", nullptr)) {
      int new_size = g_tb_skin->GetDimensionConverter()->GetPxFromString(
          size, fd.GetSize());
      fd.SetSize(new_size);
    }
    if (const char* name = font->GetValueString("name", nullptr)) {
      fd.SetID(name);
    }
    SetFontDescription(fd);
  }

  info.target->OnInflateChild(this);

  if (Node* rect_node = info.node->GetNode("rect")) {
    const DimensionConverter* dc = g_tb_skin->GetDimensionConverter();
    Value& val = rect_node->GetValue();
    if (val.GetArrayLength() == 4) {
      set_rect({dc->GetPxFromValue(val.GetArray()->GetValue(0), 0),
                dc->GetPxFromValue(val.GetArray()->GetValue(1), 0),
                dc->GetPxFromValue(val.GetArray()->GetValue(2), 0),
                dc->GetPxFromValue(val.GetArray()->GetValue(3), 0)});
    }
  }
}

TB_WIDGET_FACTORY(Button, Value::Type::kNull, ElementZ::kBottom) {}
void Button::OnInflate(const InflateInfo& info) {
  SetToggleMode(info.node->GetValueInt("toggle-mode", GetToggleMode()) ? true
                                                                       : false);
  Element::OnInflate(info);
}

TB_WIDGET_FACTORY(SelectInline, Value::Type::kInt, ElementZ::kTop) {}
void SelectInline::OnInflate(const InflateInfo& info) {
  int min = info.node->GetValueInt("min", GetMinValue());
  int max = info.node->GetValueInt("max", GetMaxValue());
  SetLimits(min, max);
  Element::OnInflate(info);
}

TB_WIDGET_FACTORY(LabelContainer, Value::Type::kString, ElementZ::kBottom) {}

TB_WIDGET_FACTORY(TextBox, Value::Type::kString, ElementZ::kTop) {}
void TextBox::OnInflate(const InflateInfo& info) {
  SetMultiline(info.node->GetValueInt("multiline", 0) ? true : false);
  SetStyling(info.node->GetValueInt("styling", 0) ? true : false);
  SetReadOnly(info.node->GetValueInt("readonly", 0) ? true : false);
  SetWrapping(info.node->GetValueInt("wrap", GetWrapping()) ? true : false);
  SetAdaptToContentSize(
      info.node->GetValueInt("adapt-to-content", GetAdaptToContentSize())
          ? true
          : false);
  if (const char* virtual_width =
          info.node->GetValueString("virtual-width", nullptr)) {
    SetVirtualWidth(g_tb_skin->GetDimensionConverter()->GetPxFromString(
        virtual_width, GetVirtualWidth()));
  }
  if (const char* text = info.node->GetValueString("placeholder", nullptr)) {
    SetPlaceholderText(text);
  }
  if (const char* type = info.node->GetValueString("type", nullptr)) {
    SetEditType(from_string(type, GetEditType()));
  }
  Element::OnInflate(info);
}

TB_WIDGET_FACTORY(Layout, Value::Type::kNull, ElementZ::kTop) {}
void Layout::OnInflate(const InflateInfo& info) {
  if (const char* spacing = info.node->GetValueString("spacing", nullptr)) {
    SetSpacing(g_tb_skin->GetDimensionConverter()->GetPxFromString(
        spacing, kSpacingFromSkin));
  }
  SetGravity(Gravity::kAll);
  if (const char* size = info.node->GetValueString("size", nullptr)) {
    SetLayoutSize(from_string(size, LayoutSize::kPreferred));
  }
  if (const char* pos = info.node->GetValueString("position", nullptr)) {
    LayoutPosition lp = LayoutPosition::kCenter;
    if (strstr(pos, "left") || strstr(pos, "top")) {
      lp = LayoutPosition::kLeftTop;
    } else if (strstr(pos, "right") || strstr(pos, "bottom")) {
      lp = LayoutPosition::kRightBottom;
    } else if (strstr(pos, "gravity")) {
      lp = LayoutPosition::kGravity;
    }
    SetLayoutPosition(lp);
  }
  if (const char* pos = info.node->GetValueString("overflow", nullptr)) {
    SetLayoutOverflow(from_string(pos, LayoutOverflow::kClip));
  }
  if (const char* dist = info.node->GetValueString("distribution", nullptr)) {
    SetLayoutDistribution(from_string(dist, LayoutDistribution::kPreferred));
  }
  if (const char* dist =
          info.node->GetValueString("distribution-position", nullptr)) {
    LayoutDistributionPosition ld = LayoutDistributionPosition::kCenter;
    if (strstr(dist, "left") || strstr(dist, "top")) {
      ld = LayoutDistributionPosition::kLeftTop;
    } else if (strstr(dist, "right") || strstr(dist, "bottom")) {
      ld = LayoutDistributionPosition::kRightBottom;
    }
    SetLayoutDistributionPosition(ld);
  }
  Element::OnInflate(info);
}

TB_WIDGET_FACTORY(ScrollContainer, Value::Type::kNull, ElementZ::kTop) {}
void ScrollContainer::OnInflate(const InflateInfo& info) {
  SetGravity(Gravity::kAll);
  SetAdaptContentSize(
      info.node->GetValueInt("adapt-content", GetAdaptContentSize()) ? true
                                                                     : false);
  SetAdaptToContentSize(
      info.node->GetValueInt("adapt-to-content", GetAdaptToContentSize())
          ? true
          : false);
  if (const char* mode = info.node->GetValueString("scroll-mode", nullptr)) {
    SetScrollMode(from_string(mode, GetScrollMode()));
  }
  Element::OnInflate(info);
}

TB_WIDGET_FACTORY(TabContainer, Value::Type::kNull, ElementZ::kTop) {}
void TabContainer::OnInflate(const InflateInfo& info) {
  Element::OnInflate(info);

  if (const char* align = info.node->GetValueString("align", nullptr)) {
    SetAlignment(tb::from_string(align, GetAlignment()));
  }
  // Allow additional attributes to be specified for the "tabs", "content" and
  // "root" layouts by calling OnInflate.
  if (Node* tabs = info.node->GetNode("tabs")) {
    // Inflate the tabs elements into the tab layout.
    Layout* tab_layout = GetTabLayout();
    info.reader->LoadNodeTree(tab_layout, tabs);

    InflateInfo inflate_info(info.reader, tab_layout->GetContentRoot(), tabs,
                             Value::Type::kNull);
    tab_layout->OnInflate(inflate_info);
  }
  if (Node* tabs = info.node->GetNode("content")) {
    InflateInfo inflate_info(info.reader, GetContentRoot(), tabs,
                             Value::Type::kNull);
    GetContentRoot()->OnInflate(inflate_info);
  }
  if (Node* tabs = info.node->GetNode("root")) {
    InflateInfo inflate_info(info.reader, &m_root_layout, tabs,
                             Value::Type::kNull);
    m_root_layout.OnInflate(inflate_info);
  }
}

TB_WIDGET_FACTORY(ScrollBar, Value::Type::kFloat, ElementZ::kTop) {}
void ScrollBar::OnInflate(const InflateInfo& info) {
  auto axis = tb::from_string(info.node->GetValueString("axis", "x"), Axis::kY);
  SetAxis(axis);
  SetGravity(axis == Axis::kX ? Gravity::kLeftRight : Gravity::kTopBottom);
  Element::OnInflate(info);
}

TB_WIDGET_FACTORY(Slider, Value::Type::kFloat, ElementZ::kTop) {}
void Slider::OnInflate(const InflateInfo& info) {
  auto axis = tb::from_string(info.node->GetValueString("axis", "x"), Axis::kY);
  SetAxis(axis);
  SetGravity(axis == Axis::kX ? Gravity::kLeftRight : Gravity::kTopBottom);
  double min = double(info.node->GetValueFloat("min", (float)GetMinValue()));
  double max = double(info.node->GetValueFloat("max", (float)GetMaxValue()));
  SetLimits(min, max);
  Element::OnInflate(info);
}

void ReadItems(Node* node, GenericStringItemSource* target_source) {
  // If there is a items node, loop through all its children and add
  // items to the target item source.
  if (Node* items = node->GetNode("items")) {
    for (Node* n = items->GetFirstChild(); n; n = n->GetNext()) {
      if (strcmp(n->GetName(), "item") != 0) {
        continue;
      }
      const char* item_str = n->GetValueString("text", "");
      TBID item_id;
      if (Node* n_id = n->GetNode("id")) {
        ElementReader::SetIDFromNode(item_id, n_id);
      }

      auto item = new GenericStringItem(item_str, item_id);
      target_source->AddItem(item);
    }
  }
}

TB_WIDGET_FACTORY(SelectList, Value::Type::kInt, ElementZ::kTop) {}
void SelectList::OnInflate(const InflateInfo& info) {
  // Read items (if there is any) into the default source.
  ReadItems(info.node, GetDefaultSource());
  Element::OnInflate(info);
}

TB_WIDGET_FACTORY(SelectDropdown, Value::Type::kInt, ElementZ::kTop) {}
void SelectDropdown::OnInflate(const InflateInfo& info) {
  // Read items (if there is any) into the default source.
  ReadItems(info.node, GetDefaultSource());
  Element::OnInflate(info);
}

TB_WIDGET_FACTORY(CheckBox, Value::Type::kInt, ElementZ::kTop) {}
TB_WIDGET_FACTORY(RadioButton, Value::Type::kInt, ElementZ::kTop) {}

TB_WIDGET_FACTORY(Label, Value::Type::kString, ElementZ::kTop) {}
void Label::OnInflate(const InflateInfo& info) {
  if (const char* text_align =
          info.node->GetValueString("text-align", nullptr)) {
    SetTextAlign(from_string(text_align, GetTextAlign()));
  }
  Element::OnInflate(info);
}

TB_WIDGET_FACTORY(SkinImage, Value::Type::kNull, ElementZ::kTop) {}
TB_WIDGET_FACTORY(Separator, Value::Type::kNull, ElementZ::kTop) {}
TB_WIDGET_FACTORY(ProgressSpinner, Value::Type::kInt, ElementZ::kTop) {}
TB_WIDGET_FACTORY(Container, Value::Type::kNull, ElementZ::kTop) {}
TB_WIDGET_FACTORY(SectionHeader, Value::Type::kInt, ElementZ::kTop) {}
TB_WIDGET_FACTORY(Section, Value::Type::kInt, ElementZ::kTop) {}

TB_WIDGET_FACTORY(ToggleContainer, Value::Type::kInt, ElementZ::kTop) {}
void ToggleContainer::OnInflate(const InflateInfo& info) {
  if (const char* toggle = info.node->GetValueString("toggle", nullptr)) {
    SetToggleAction(from_string(toggle, GetToggleAction()));
  }
  SetInvert(info.node->GetValueInt("invert", GetInvert()) ? true : false);
  Element::OnInflate(info);
}

TB_WIDGET_FACTORY(ImageElement, Value::Type::kNull, ElementZ::kTop) {}
void ImageElement::OnInflate(const InflateInfo& info) {
  if (const char* filename = info.node->GetValueString("filename", nullptr)) {
    SetImage(filename);
  }
  Element::OnInflate(info);
}

// We can't use a linked list object since we don't know if its constructor
// would run before of after any element factory constructor that add itself
// to it. Using a manual one way link list is very simple.
ElementFactory* g_registered_factories = nullptr;

ElementFactory::ElementFactory(const char* name, Value::Type sync_type)
    : name(name), sync_type(sync_type) {}

void ElementFactory::Register() {
  next_registered_wf = g_registered_factories;
  g_registered_factories = this;
}

ElementReader* ElementReader::Create() {
  ElementReader* w_reader = new ElementReader();
  if (!w_reader->Init()) {
    delete w_reader;
    return nullptr;
  }
  return w_reader;
}

bool ElementReader::Init() {
  for (ElementFactory* wf = g_registered_factories; wf;
       wf = wf->next_registered_wf) {
    if (!AddFactory(wf)) {
      return false;
    }
  }
  return true;
}

ElementReader::~ElementReader() {}

bool ElementReader::LoadFile(Element* target, const char* filename) {
  Node node;
  if (!node.ReadFile(filename)) {
    return false;
  }
  LoadNodeTree(target, &node);
  return true;
}

bool ElementReader::LoadData(Element* target, const char* data) {
  Node node;
  node.ReadData(data);
  LoadNodeTree(target, &node);
  return true;
}

bool ElementReader::LoadData(Element* target, const char* data,
                             size_t data_len) {
  Node node;
  node.ReadData(data, data_len);
  LoadNodeTree(target, &node);
  return true;
}

void ElementReader::LoadNodeTree(Element* target, Node* node) {
  // Iterate through all nodes and create elements.
  for (Node* child = node->GetFirstChild(); child; child = child->GetNext()) {
    CreateElement(target, child);
  }
}

void ElementReader::SetIDFromNode(TBID& id, Node* node) {
  if (!node) return;
  if (node->GetValue().IsString()) {
    id.Set(node->GetValue().GetString());
  } else {
    id.Set(node->GetValue().GetInt());
  }
}

bool ElementReader::CreateElement(Element* target, Node* node) {
  // Find a element creator from the node name.
  ElementFactory* wc = nullptr;
  for (wc = factories.GetFirst(); wc; wc = wc->GetNext()) {
    if (strcmp(node->GetName(), wc->name) == 0) {
      break;
    }
  }
  if (!wc) {
    return false;
  }

  // Create the element.
  InflateInfo info(this, target->GetContentRoot(), node, wc->sync_type);
  Element* new_element = wc->Create(&info);
  if (!new_element) {
    return false;
  }

  // Read properties and add i to the hierarchy.
  new_element->OnInflate(info);

  // If this assert is trigged, you probably forgot to call Element::OnInflate
  // from an overridden version.
  assert(new_element->GetParent());

  // Iterate through all nodes and create elements.
  for (Node* n = node->GetFirstChild(); n; n = n->GetNext()) {
    CreateElement(new_element, n);
  }

  if (node->GetValueInt("autofocus", 0)) {
    new_element->SetFocus(FocusReason::kUnknown);
  }

  return true;
}

}  // namespace tb
