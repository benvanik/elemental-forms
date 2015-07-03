/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#include "tb_text_box.h"

#include <algorithm>

#include "tb_font_renderer.h"
#include "tb_language.h"
#include "tb_menu_window.h"
#include "tb_select.h"
#include "tb_skin_util.h"
#include "tb_style_edit_content.h"
#include "tb_system.h"
#include "tb_widget_skin_condition_context.h"
#include "tb_widgets_reader.h"

namespace tb {

const int CARET_BLINK_TIME = 500;
const int SELECTION_SCROLL_DELAY = 1000 / 30;

// Gets the delta that should be scrolled if dragging the pointer outside the
// range min-max.
int GetSelectionScrollSpeed(int pointerpos, int min, int max) {
  int d = 0;
  if (pointerpos < min)
    d = pointerpos - min;
  else if (pointerpos > max)
    d = pointerpos - max;
  d *= d;
  d /= 40;
  return (pointerpos < min) ? -d : d;
}

TextBox::TextBox() {
  SetIsFocusable(true);
  SetWantLongClick(true);
  AddChild(&m_scrollbar_x);
  AddChild(&m_scrollbar_y);
  AddChild(&m_root);
  m_root.SetGravity(Gravity::kAll);
  m_scrollbar_x.SetGravity(Gravity::kBottom | Gravity::kLeftRight);
  m_scrollbar_y.SetGravity(Gravity::kRight | Gravity::kTopBottom);
  m_scrollbar_y.SetAxis(Axis::kY);
  int scrollbar_y_w = m_scrollbar_y.GetPreferredSize().pref_w;
  int scrollbar_x_h = m_scrollbar_x.GetPreferredSize().pref_h;
  m_scrollbar_x.set_rect({0, -scrollbar_x_h, -scrollbar_y_w, scrollbar_x_h});
  m_scrollbar_y.set_rect({-scrollbar_y_w, 0, scrollbar_y_w, 0});
  m_scrollbar_x.SetOpacity(0);
  m_scrollbar_y.SetOpacity(0);

  SetSkinBg(TBIDC("TextBox"), InvokeInfo::kNoCallbacks);
  m_style_edit.SetListener(this);

  m_root.set_rect(GetVisibleRect());

  m_placeholder.SetTextAlign(TextAlign::kLeft);

  m_content_factory.text_box = this;
  m_style_edit.SetContentFactory(&m_content_factory);
}

TextBox::~TextBox() {
  RemoveChild(&m_root);
  RemoveChild(&m_scrollbar_y);
  RemoveChild(&m_scrollbar_x);
}

Rect TextBox::GetVisibleRect() {
  Rect rect = GetPaddingRect();
  if (m_scrollbar_y.GetOpacity()) {
    rect.w -= m_scrollbar_y.rect().w;
  }
  if (m_scrollbar_x.GetOpacity()) {
    rect.h -= m_scrollbar_x.rect().h;
  }
  return rect;
}

void TextBox::UpdateScrollbarVisibility(bool multiline) {
  bool enable_vertical = multiline && !m_adapt_to_content_size;
  m_scrollbar_y.SetOpacity(enable_vertical ? 1.f : 0.f);
  m_root.set_rect(GetVisibleRect());
}

void TextBox::SetAdaptToContentSize(bool adapt) {
  if (m_adapt_to_content_size == adapt) return;
  m_adapt_to_content_size = adapt;
  UpdateScrollbarVisibility(GetMultiline());
}

void TextBox::SetVirtualWidth(int virtual_width) {
  if (m_virtual_width == virtual_width) return;
  m_virtual_width = virtual_width;

  if (m_adapt_to_content_size && m_style_edit.packed.wrapping) {
    InvalidateLayout(InvalidationMode::kRecursive);
  }
}

void TextBox::SetMultiline(bool multiline) {
  if (multiline == GetMultiline()) return;
  UpdateScrollbarVisibility(multiline);
  m_style_edit.SetMultiline(multiline);
  SetWrapping(multiline);
  InvalidateSkinStates();
  Element::Invalidate();
}

void TextBox::SetStyling(bool styling) { m_style_edit.SetStyling(styling); }

void TextBox::SetReadOnly(bool readonly) {
  if (readonly == GetReadOnly()) return;
  m_style_edit.SetReadOnly(readonly);
  InvalidateSkinStates();
  Element::Invalidate();
}

void TextBox::SetWrapping(bool wrapping) {
  if (wrapping == GetWrapping()) return;

  m_style_edit.SetWrapping(wrapping);

  // Invalidate the layout when the wrap mode change and we should adapt our
  // size to it.
  if (m_adapt_to_content_size) InvalidateLayout(InvalidationMode::kRecursive);
}

void TextBox::SetEditType(EditType type) {
  if (m_edit_type == type) return;
  m_edit_type = type;
  m_style_edit.SetPassword(type == EditType::kPassword);
  InvalidateSkinStates();
  Element::Invalidate();
}

bool TextBox::GetCustomSkinCondition(const SkinCondition::ConditionInfo& info) {
  if (info.custom_prop == TBIDC("edit-type")) {
    switch (m_edit_type) {
      case EditType::kText:
        return info.value == TBIDC("text");
      case EditType::kSearch:
        return info.value == TBIDC("search");
      case EditType::kPassword:
        return info.value == TBIDC("password");
      case EditType::kEmail:
        return info.value == TBIDC("email");
      case EditType::kPhone:
        return info.value == TBIDC("phone");
      case EditType::kUrl:
        return info.value == TBIDC("url");
      case EditType::kNumber:
        return info.value == TBIDC("number");
    };
  } else if (info.custom_prop == TBIDC("multiline")) {
    return !((uint32_t)info.value) == !GetMultiline();
  } else if (info.custom_prop == TBIDC("readonly")) {
    return !((uint32_t)info.value) == !GetReadOnly();
  }
  return false;
}

void TextBox::ScrollTo(int x, int y) {
  int old_x = m_scrollbar_x.GetValue();
  int old_y = m_scrollbar_y.GetValue();
  m_style_edit.SetScrollPos(x, y);
  if (old_x != m_scrollbar_x.GetValue() || old_y != m_scrollbar_y.GetValue()) {
    Element::Invalidate();
  }
}

Element::ScrollInfo TextBox::GetScrollInfo() {
  ScrollInfo info;
  info.min_x = static_cast<int>(m_scrollbar_x.GetMinValue());
  info.min_y = static_cast<int>(m_scrollbar_y.GetMinValue());
  info.max_x = static_cast<int>(m_scrollbar_x.GetMaxValue());
  info.max_y = static_cast<int>(m_scrollbar_y.GetMaxValue());
  info.x = m_scrollbar_x.GetValue();
  info.y = m_scrollbar_y.GetValue();
  return info;
}

bool TextBox::OnEvent(const ElementEvent& ev) {
  if (ev.type == EventType::kChanged && ev.target == &m_scrollbar_x) {
    m_style_edit.SetScrollPos(m_scrollbar_x.GetValue(), m_style_edit.scroll_y);
    OnScroll(m_scrollbar_x.GetValue(), m_style_edit.scroll_y);
    return true;
  } else if (ev.type == EventType::kChanged && ev.target == &m_scrollbar_y) {
    m_style_edit.SetScrollPos(m_style_edit.scroll_x, m_scrollbar_y.GetValue());
    OnScroll(m_style_edit.scroll_x, m_scrollbar_y.GetValue());
    return true;
  } else if (ev.type == EventType::kWheel &&
             ev.modifierkeys == ModifierKeys::kNone) {
    int old_val = m_scrollbar_y.GetValue();
    m_scrollbar_y.SetValue(old_val + ev.delta_y * TBSystem::GetPixelsPerLine());
    return m_scrollbar_y.GetValue() != old_val;
  } else if (ev.type == EventType::kPointerDown && ev.target == this) {
    Rect padding_rect = GetPaddingRect();
    if (m_style_edit.MouseDown(
            Point(ev.target_x - padding_rect.x, ev.target_y - padding_rect.y),
            1, ev.count, ModifierKeys::kNone, ev.touch)) {
      // Post a message to start selection scroll
      PostMessageDelayed(TBIDC("selscroll"), nullptr, SELECTION_SCROLL_DELAY);
      return true;
    }
  } else if (ev.type == EventType::kPointerMove && ev.target == this) {
    Rect padding_rect = GetPaddingRect();
    return m_style_edit.MouseMove(
        Point(ev.target_x - padding_rect.x, ev.target_y - padding_rect.y));
  } else if (ev.type == EventType::kPointerUp && ev.target == this) {
    Rect padding_rect = GetPaddingRect();
    return m_style_edit.MouseUp(
        Point(ev.target_x - padding_rect.x, ev.target_y - padding_rect.y), 1,
        ModifierKeys::kNone, ev.touch);
  } else if (ev.type == EventType::kKeyDown) {
    return m_style_edit.KeyDown(ev.key, ev.special_key, ev.modifierkeys);
  } else if (ev.type == EventType::kKeyUp) {
    return true;
  } else if ((ev.type == EventType::kClick &&
              ev.target->GetID() == TBIDC("popupmenu")) ||
             (ev.type == EventType::kShortcut)) {
    if (ev.ref_id == TBIDC("cut") && !m_style_edit.packed.read_only) {
      m_style_edit.Cut();
    } else if (ev.ref_id == TBIDC("copy")) {
      m_style_edit.Copy();
    } else if (ev.ref_id == TBIDC("paste") && !m_style_edit.packed.read_only) {
      m_style_edit.Paste();
    } else if (ev.ref_id == TBIDC("delete") && !m_style_edit.packed.read_only) {
      m_style_edit.Delete();
    } else if (ev.ref_id == TBIDC("undo") && !m_style_edit.packed.read_only) {
      m_style_edit.Undo();
    } else if (ev.ref_id == TBIDC("redo") && !m_style_edit.packed.read_only) {
      m_style_edit.Redo();
    } else if (ev.ref_id == TBIDC("selectall")) {
      m_style_edit.selection.SelectAll();
    } else {
      return false;
    }
    return true;
  } else if (ev.type == EventType::kContextMenu && ev.target == this) {
    Point pos_in_root(ev.target_x, ev.target_y);
    ev.target->ConvertToRoot(pos_in_root.x, pos_in_root.y);

    MenuWindow* menu = new MenuWindow(ev.target, TBIDC("popupmenu"));
    GenericStringItemSource* source = menu->GetList()->GetDefaultSource();
    source->AddItem(
        new GenericStringItem(g_tb_lng->GetString(TBIDC("cut")), TBIDC("cut")));
    source->AddItem(new GenericStringItem(g_tb_lng->GetString(TBIDC("copy")),
                                          TBIDC("copy")));
    source->AddItem(new GenericStringItem(g_tb_lng->GetString(TBIDC("paste")),
                                          TBIDC("paste")));
    source->AddItem(new GenericStringItem(g_tb_lng->GetString(TBIDC("delete")),
                                          TBIDC("delete")));
    source->AddItem(new GenericStringItem("-"));
    source->AddItem(new GenericStringItem(
        g_tb_lng->GetString(TBIDC("selectall")), TBIDC("selectall")));
    menu->Show(source, PopupAlignment(pos_in_root), -1);
    return true;
  }
  return false;
}

void TextBox::OnPaint(const PaintProps& paint_props) {
  Rect visible_rect = GetVisibleRect();

  bool clip = m_scrollbar_x.CanScroll() || m_scrollbar_y.CanScroll();
  Rect old_clip;
  if (clip) {
    old_clip = g_renderer->SetClipRect(visible_rect, true);
  }

  int trans_x = visible_rect.x, trans_y = visible_rect.y;
  g_renderer->Translate(trans_x, trans_y);

  // Draw text content, caret etc.
  visible_rect.x = visible_rect.y = 0;
  m_style_edit.Paint(visible_rect, GetCalculatedFontDescription(),
                     paint_props.text_color);

  // If empty, draw placeholder text with some opacity.
  if (m_style_edit.empty()) {
    float old_opacity = g_renderer->GetOpacity();
    g_renderer->SetOpacity(old_opacity *
                           g_tb_skin->GetDefaultPlaceholderOpacity());
    Rect placeholder_rect(visible_rect.x, visible_rect.y, visible_rect.w,
                          GetFont()->height());
    m_placeholder.Paint(this, placeholder_rect, paint_props.text_color);
    g_renderer->SetOpacity(old_opacity);
  }
  g_renderer->Translate(-trans_x, -trans_y);

  if (clip) {
    g_renderer->SetClipRect(old_clip, false);
  }
}

void TextBox::OnPaintChildren(const PaintProps& paint_props) {
  Element::OnPaintChildren(paint_props);

  // Draw fadeout skin at the needed edges.
  DrawEdgeFadeout(
      GetVisibleRect(), TBIDC("TextBox.fadeout_x"), TBIDC("TextBox.fadeout_y"),
      m_scrollbar_x.GetValue(), m_scrollbar_y.GetValue(),
      int(m_scrollbar_x.GetMaxValue() - m_scrollbar_x.GetValueDouble()),
      int(m_scrollbar_y.GetMaxValue() - m_scrollbar_y.GetValueDouble()));
}

void TextBox::OnAdded() {
  m_style_edit.SetFont(GetCalculatedFontDescription());
}

void TextBox::OnFontChanged() {
  m_style_edit.SetFont(GetCalculatedFontDescription());
}

void TextBox::OnFocusChanged(bool focused) { m_style_edit.Focus(focused); }

void TextBox::OnResized(int old_w, int old_h) {
  // Make the scrollbars move.
  Element::OnResized(old_w, old_h);

  Rect visible_rect = GetVisibleRect();
  m_style_edit.SetLayoutSize(visible_rect.w, visible_rect.h, false);

  UpdateScrollbars();
}

PreferredSize TextBox::OnCalculatePreferredContentSize(
    const SizeConstraints& constraints) {
  int font_height = GetFont()->height();
  PreferredSize ps;
  if (m_adapt_to_content_size) {
    int old_layout_width = m_style_edit.layout_width;
    int old_layout_height = m_style_edit.layout_height;
    if (m_style_edit.packed.wrapping) {
      // If we have wrapping enabled, we have to set a virtual width and format
      // the text so we can get the actual content width with a constant result
      // every time.
      // If the layouter does not respect our size constraints in the end, we
      // may get a completly different content height due to different wrapping.
      // To fix that, we need to layout in 2 passes.

      // A hacky fix is to do something we probably shouldn't: use the old
      // layout width as virtual width for the new.
      // int layout_width = old_layout_width > 0 ? std::max(old_layout_width,
      // m_virtual_width) : m_virtual_width;
      int layout_width = m_virtual_width;
      if (constraints.available_w != SizeConstraints::kNoRestriction) {
        layout_width = constraints.available_w;
        if (SkinElement* bg_skin = GetSkinBgElement()) {
          layout_width -= bg_skin->padding_left + bg_skin->padding_right;
        }
      }

      m_style_edit.SetLayoutSize(layout_width, old_layout_height, true);
      ps.size_dependency = SizeDependency::kHeightOnWidth;
    }
    int width = m_style_edit.GetContentWidth();
    int height = m_style_edit.GetContentHeight();
    if (m_style_edit.packed.wrapping) {
      m_style_edit.SetLayoutSize(old_layout_width, old_layout_height, true);
    }
    height = std::max(height, font_height);

    ps.min_w = ps.pref_w /*= ps.max_w*/ =
        width;  // should go with the hack above.
    // ps.min_w = ps.pref_w = ps.max_w = width;
    ps.min_h = ps.pref_h = ps.max_h = height;
  } else {
    ps.pref_h = ps.min_h = font_height;
    if (m_style_edit.packed.multiline_on) {
      ps.pref_w = font_height * 10;
      ps.pref_h = font_height * 5;
    } else {
      ps.max_h = ps.pref_h;
    }
  }
  return ps;
}

void TextBox::OnMessageReceived(Message* msg) {
  if (msg->message == TBIDC("blink")) {
    m_style_edit.caret.on = !m_style_edit.caret.on;
    m_style_edit.caret.Invalidate();

    // Post another blink message so we blink again.
    PostMessageDelayed(TBIDC("blink"), nullptr, CARET_BLINK_TIME);
  } else if (msg->message == TBIDC("selscroll") && captured_element == this) {
    // Get scroll speed from where mouse is relative to the padding rect.
    Rect padding_rect = GetVisibleRect().Shrink(2, 2);
    int dx = GetSelectionScrollSpeed(pointer_move_element_x, padding_rect.x,
                                     padding_rect.x + padding_rect.w);
    int dy = GetSelectionScrollSpeed(pointer_move_element_y, padding_rect.y,
                                     padding_rect.y + padding_rect.h);
    m_scrollbar_x.SetValue(m_scrollbar_x.GetValue() + dx);
    m_scrollbar_y.SetValue(m_scrollbar_y.GetValue() + dy);

    // Handle mouse move at the new scroll position, so selection is updated
    if (dx || dy) {
      m_style_edit.MouseMove(
          Point(pointer_move_element_x, pointer_move_element_y));
    }

    // Post another setscroll message so we continue scrolling if we still
    // should.
    if (m_style_edit.select_state) {
      PostMessageDelayed(TBIDC("selscroll"), nullptr, SELECTION_SCROLL_DELAY);
    }
  }
}

void TextBox::OnChange() {
  // Invalidate the layout when the content change and we should adapt our size
  // to it.
  if (m_adapt_to_content_size) {
    InvalidateLayout(InvalidationMode::kRecursive);
  }

  ElementEvent ev(EventType::kChanged);
  InvokeEvent(ev);
}

bool TextBox::OnEnter() { return false; }

void TextBox::Invalidate(const Rect& rect) { Element::Invalidate(); }

void TextBox::DrawString(int32_t x, int32_t y, FontFace* font,
                         const Color& color, const char* str, size_t len) {
  font->DrawString(x, y, color, str, len);
}

void TextBox::DrawRect(const Rect& rect, const Color& color) {
  g_renderer->DrawRect(rect, color);
}

void TextBox::DrawRectFill(const Rect& rect, const Color& color) {
  g_renderer->DrawRectFill(rect, color);
}

void TextBox::DrawTextSelectionBg(const Rect& rect) {
  ElementSkinConditionContext context(this);
  g_tb_skin->PaintSkin(rect, TBIDC("TextBox.selection"),
                       static_cast<SkinState>(GetAutoState()), context);
}

void TextBox::DrawContentSelectionFg(const Rect& rect) {
  ElementSkinConditionContext context(this);
  g_tb_skin->PaintSkin(rect, TBIDC("TextBox.selection"),
                       static_cast<SkinState>(GetAutoState()), context);
}

void TextBox::DrawCaret(const Rect& rect) {
  if (GetIsFocused() && !m_style_edit.packed.read_only) {
    DrawTextSelectionBg(rect);
  }
}

void TextBox::Scroll(int32_t dx, int32_t dy) {
  Element::Invalidate();
  m_scrollbar_x.SetValue(m_style_edit.scroll_x);
  m_scrollbar_y.SetValue(m_style_edit.scroll_y);
}

void TextBox::UpdateScrollbars() {
  int32_t w = m_style_edit.layout_width;
  int32_t h = m_style_edit.layout_height;
  m_scrollbar_x.SetLimits(0, m_style_edit.GetContentWidth() - w, w);
  m_scrollbar_y.SetLimits(0, m_style_edit.GetContentHeight() - h, h);
}

void TextBox::CaretBlinkStart() {
  // Post the delayed blink message if we don't already have one
  if (!GetMessageByID(TBIDC("blink"))) {
    PostMessageDelayed(TBIDC("blink"), nullptr, CARET_BLINK_TIME);
  }
}

void TextBox::CaretBlinkStop() {
  // Remove the blink message if we have one
  if (Message* msg = GetMessageByID(TBIDC("blink"))) {
    DeleteMessage(msg);
  }
}

void TextBoxScrollRoot::OnPaintChildren(const PaintProps& paint_props) {
  // Avoid setting clipping (can be expensive) if we have no children to paint
  // anyway.
  if (!GetFirstChild()) return;
  // Clip children.
  Rect old_clip_rect = g_renderer->SetClipRect(GetPaddingRect(), true);
  Element::OnPaintChildren(paint_props);
  g_renderer->SetClipRect(old_clip_rect, false);
}

void TextBoxScrollRoot::GetChildTranslation(int& x, int& y) const {
  TextBox* edit_field = static_cast<TextBox*>(GetParent());
  x = -edit_field->GetStyleEdit()->scroll_x;
  y = -edit_field->GetStyleEdit()->scroll_y;
}

HitStatus TextBoxScrollRoot::GetHitStatus(int x, int y) {
  // Return no hit on this element, but maybe on any of the children.
  if (Element::GetHitStatus(x, y) != HitStatus::kNoHit &&
      GetElementAt(x, y, false)) {
    return HitStatus::kHit;
  }
  return HitStatus::kNoHit;
}

class TextFragmentContentElement : public TextFragmentContent {
 public:
  TextFragmentContentElement(Element* parent, Element* element);
  ~TextFragmentContentElement() override;

  void UpdatePos(int x, int y) override;
  int32_t GetWidth(FontFace* font, TextFragment* fragment) override;
  int32_t GetHeight(FontFace* font, TextFragment* fragment) override;
  int32_t GetBaseline(FontFace* font, TextFragment* fragment) override;

 private:
  Element* m_element;
};

TextFragmentContentElement::TextFragmentContentElement(Element* parent,
                                                       Element* element)
    : m_element(element) {
  parent->GetContentRoot()->AddChild(element);
}

TextFragmentContentElement::~TextFragmentContentElement() {
  m_element->GetParent()->RemoveChild(m_element);
  delete m_element;
}

void TextFragmentContentElement::UpdatePos(int x, int y) {
  m_element->set_rect(
      {x, y, GetWidth(nullptr, nullptr), GetHeight(nullptr, nullptr)});
}

int32_t TextFragmentContentElement::GetWidth(FontFace* font,
                                             TextFragment* fragment) {
  return m_element->rect().w ? m_element->rect().w
                             : m_element->GetPreferredSize().pref_w;
}

int32_t TextFragmentContentElement::GetHeight(FontFace* font,
                                              TextFragment* fragment) {
  return m_element->rect().h ? m_element->rect().h
                             : m_element->GetPreferredSize().pref_h;
}

int32_t TextFragmentContentElement::GetBaseline(FontFace* font,
                                                TextFragment* fragment) {
  int height = GetHeight(font, fragment);
  return (height + fragment->block->CalculateBaseline(font)) / 2;
}

int TextBoxContentFactory::GetContent(const char* text) {
  return TextFragmentContentFactory::GetContent(text);
}

TextFragmentContent* TextBoxContentFactory::CreateFragmentContent(
    const char* text, size_t text_len) {
  if (strncmp(text, "<element ", std::min(text_len, 8ull)) == 0) {
    // Create a wrapper for the generated element.
    // Its size will adapt to the content.
    auto element = new Element();
    auto cw = new TextFragmentContentElement(text_box, element);
    g_elements_reader->LoadData(element, text + 8, text_len - 9);
    return cw;
  }

  return TextFragmentContentFactory::CreateFragmentContent(text, text_len);
}

}  // namespace tb
