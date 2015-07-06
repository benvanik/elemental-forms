/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#include "tb/elements/menu_window.h"
#include "tb/elements/text_box.h"
#include "tb/parsing/element_inflater.h"
#include "tb/text/font_face.h"
#include "tb/text/text_fragment_content.h"
#include "tb/skin.h"
#include "tb/util/metrics.h"
#include "tb/util/string_table.h"

namespace tb {
namespace elements {

using graphics::Renderer;

const int kCaretBlinkTimeMillis = 500;
const int kSelectionScrollDelayMillis = 1000 / 30;

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

void TextBox::RegisterInflater() {
  TB_REGISTER_ELEMENT_INFLATER(TextBox, Value::Type::kString, ElementZ::kTop);
}

TextBox::TextBox() {
  set_focusable(true);
  set_long_clickable(true);
  AddChild(&m_scrollbar_x);
  AddChild(&m_scrollbar_y);
  AddChild(&m_root);
  m_root.set_gravity(Gravity::kAll);
  m_scrollbar_x.set_gravity(Gravity::kBottom | Gravity::kLeftRight);
  m_scrollbar_y.set_gravity(Gravity::kRight | Gravity::kTopBottom);
  m_scrollbar_y.set_axis(Axis::kY);
  int scrollbar_y_w = m_scrollbar_y.GetPreferredSize().pref_w;
  int scrollbar_x_h = m_scrollbar_x.GetPreferredSize().pref_h;
  m_scrollbar_x.set_rect({0, -scrollbar_x_h, -scrollbar_y_w, scrollbar_x_h});
  m_scrollbar_y.set_rect({-scrollbar_y_w, 0, scrollbar_y_w, 0});
  m_scrollbar_x.set_opacity(0);
  m_scrollbar_y.set_opacity(0);

  set_background_skin(TBIDC("TextBox"), InvokeInfo::kNoCallbacks);
  m_style_edit.SetListener(this);

  m_root.set_rect(visible_rect());

  m_placeholder.set_text_align(TextAlign::kLeft);

  m_content_factory.text_box = this;
  m_style_edit.SetContentFactory(&m_content_factory);
}

TextBox::~TextBox() {
  RemoveChild(&m_root);
  RemoveChild(&m_scrollbar_y);
  RemoveChild(&m_scrollbar_x);
}

void TextBox::OnInflate(const parsing::InflateInfo& info) {
  set_multiline(info.node->GetValueInt("multiline", 0) ? true : false);
  set_styled(info.node->GetValueInt("styling", 0) ? true : false);
  set_read_only(info.node->GetValueInt("readonly", 0) ? true : false);
  set_wrapping(info.node->GetValueInt("wrap", is_wrapping()) ? true : false);
  set_adapt_to_content_size(
      info.node->GetValueInt("adapt-to-content", is_adapting_to_content_size())
          ? true
          : false);
  if (const char* virtual_width_str =
          info.node->GetValueString("virtual-width", nullptr)) {
    set_virtual_width(Skin::get()->dimension_converter()->GetPxFromString(
        virtual_width_str, virtual_width()));
  }
  if (const char* text = info.node->GetValueString("placeholder", nullptr)) {
    set_placeholder_text(text);
  }
  if (const char* type = info.node->GetValueString("type", nullptr)) {
    set_edit_type(from_string(type, edit_type()));
  }
  Element::OnInflate(info);
}

Rect TextBox::visible_rect() {
  Rect rect = padding_rect();
  if (m_scrollbar_y.opacity()) {
    rect.w -= m_scrollbar_y.rect().w;
  }
  if (m_scrollbar_x.opacity()) {
    rect.h -= m_scrollbar_x.rect().h;
  }
  return rect;
}

void TextBox::UpdateScrollbarVisibility(bool multiline) {
  bool enable_vertical = multiline && !m_adapt_to_content_size;
  m_scrollbar_y.set_opacity(enable_vertical ? 1.f : 0.f);
  m_root.set_rect(visible_rect());
}

void TextBox::set_adapt_to_content_size(bool adapt) {
  if (m_adapt_to_content_size == adapt) return;
  m_adapt_to_content_size = adapt;
  UpdateScrollbarVisibility(is_multiline());
}

void TextBox::set_virtual_width(int virtual_width) {
  if (m_virtual_width == virtual_width) return;
  m_virtual_width = virtual_width;

  if (m_adapt_to_content_size && m_style_edit.packed.wrapping) {
    InvalidateLayout(InvalidationMode::kRecursive);
  }
}

void TextBox::set_multiline(bool multiline) {
  if (multiline == is_multiline()) return;
  UpdateScrollbarVisibility(multiline);
  m_style_edit.set_multiline(multiline);
  set_wrapping(multiline);
  InvalidateSkinStates();
  Element::Invalidate();
}

void TextBox::set_styled(bool styling) { m_style_edit.set_styled(styling); }

void TextBox::set_read_only(bool readonly) {
  if (readonly == is_read_only()) return;
  m_style_edit.set_read_only(readonly);
  InvalidateSkinStates();
  Element::Invalidate();
}

void TextBox::set_wrapping(bool wrapping) {
  if (wrapping == is_wrapping()) return;

  m_style_edit.set_wrapping(wrapping);

  // Invalidate the layout when the wrap mode change and we should adapt our
  // size to it.
  if (m_adapt_to_content_size) InvalidateLayout(InvalidationMode::kRecursive);
}

void TextBox::set_edit_type(EditType type) {
  if (m_edit_type == type) return;
  m_edit_type = type;
  m_style_edit.set_password(type == EditType::kPassword);
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
    return !((uint32_t)info.value) == !is_multiline();
  } else if (info.custom_prop == TBIDC("readonly")) {
    return !((uint32_t)info.value) == !is_read_only();
  }
  return false;
}

void TextBox::ScrollTo(int x, int y) {
  int old_x = m_scrollbar_x.value();
  int old_y = m_scrollbar_y.value();
  m_style_edit.SetScrollPos(x, y);
  if (old_x != m_scrollbar_x.value() || old_y != m_scrollbar_y.value()) {
    Element::Invalidate();
  }
}

Element::ScrollInfo TextBox::scroll_info() {
  ScrollInfo info;
  info.min_x = static_cast<int>(m_scrollbar_x.min_value());
  info.min_y = static_cast<int>(m_scrollbar_y.min_value());
  info.max_x = static_cast<int>(m_scrollbar_x.max_value());
  info.max_y = static_cast<int>(m_scrollbar_y.max_value());
  info.x = m_scrollbar_x.value();
  info.y = m_scrollbar_y.value();
  return info;
}

bool TextBox::OnEvent(const Event& ev) {
  if (ev.type == EventType::kChanged && ev.target == &m_scrollbar_x) {
    m_style_edit.SetScrollPos(m_scrollbar_x.value(), m_style_edit.scroll_y);
    OnScroll(m_scrollbar_x.value(), m_style_edit.scroll_y);
    return true;
  } else if (ev.type == EventType::kChanged && ev.target == &m_scrollbar_y) {
    m_style_edit.SetScrollPos(m_style_edit.scroll_x, m_scrollbar_y.value());
    OnScroll(m_style_edit.scroll_x, m_scrollbar_y.value());
    return true;
  } else if (ev.type == EventType::kWheel &&
             ev.modifierkeys == ModifierKeys::kNone) {
    int old_val = m_scrollbar_y.value();
    m_scrollbar_y.set_value(old_val + ev.delta_y * util::GetPixelsPerLine());
    if (m_scrollbar_y.value() != old_val) {
      return true;
    }
  } else if (ev.type == EventType::kPointerDown && ev.target == this) {
    Rect padding_rect = this->padding_rect();
    if (m_style_edit.MouseDown(
            Point(ev.target_x - padding_rect.x, ev.target_y - padding_rect.y),
            1, ev.count, ModifierKeys::kNone, ev.touch)) {
      // Post a message to start selection scroll
      PostMessageDelayed(TBIDC("selscroll"), nullptr,
                         kSelectionScrollDelayMillis);
      return true;
    }
  } else if (ev.type == EventType::kPointerMove && ev.target == this) {
    Rect padding_rect = this->padding_rect();
    if (m_style_edit.MouseMove(Point(ev.target_x - padding_rect.x,
                                     ev.target_y - padding_rect.y))) {
      return true;
    }
  } else if (ev.type == EventType::kPointerUp && ev.target == this) {
    Rect padding_rect = this->padding_rect();
    if (m_style_edit.MouseUp(
            Point(ev.target_x - padding_rect.x, ev.target_y - padding_rect.y),
            1, ModifierKeys::kNone, ev.touch)) {
      return true;
    }
  } else if (ev.type == EventType::kKeyDown) {
    if (m_style_edit.KeyDown(ev.key, ev.special_key, ev.modifierkeys)) {
      return true;
    }
  } else if (ev.type == EventType::kKeyUp) {
    return true;
  } else if ((ev.type == EventType::kClick &&
              ev.target->id() == TBIDC("popupmenu")) ||
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
      return Element::OnEvent(ev);
    }
    return true;
  } else if (ev.type == EventType::kContextMenu && ev.target == this) {
    Point pos_in_root(ev.target_x, ev.target_y);
    ev.target->ConvertToRoot(pos_in_root.x, pos_in_root.y);

    MenuWindow* menu = new MenuWindow(ev.target, TBIDC("popupmenu"));
    GenericStringItemSource* source = menu->list_box()->default_source();
    source->push_back(std::make_unique<GenericStringItem>(
        util::GetString(TBIDC("cut")), TBIDC("cut")));
    source->push_back(std::make_unique<GenericStringItem>(
        util::GetString(TBIDC("copy")), TBIDC("copy")));
    source->push_back(std::make_unique<GenericStringItem>(
        util::GetString(TBIDC("paste")), TBIDC("paste")));
    source->push_back(std::make_unique<GenericStringItem>(
        util::GetString(TBIDC("delete")), TBIDC("delete")));
    source->push_back(std::make_unique<GenericStringItem>("-"));
    source->push_back(std::make_unique<GenericStringItem>(
        util::GetString(TBIDC("selectall")), TBIDC("selectall")));
    menu->Show(source, PopupAlignment(pos_in_root), -1);
    return true;
  }
  return Element::OnEvent(ev);
}

void TextBox::OnPaint(const PaintProps& paint_props) {
  Rect visible_rect = this->visible_rect();

  bool clip = m_scrollbar_x.can_scroll() || m_scrollbar_y.can_scroll();
  Rect old_clip;
  if (clip) {
    old_clip = Renderer::get()->set_clip_rect(visible_rect, true);
  }

  int trans_x = visible_rect.x, trans_y = visible_rect.y;
  Renderer::get()->Translate(trans_x, trans_y);

  // Draw text content, caret etc.
  visible_rect.x = visible_rect.y = 0;
  m_style_edit.Paint(visible_rect, computed_font_description(),
                     paint_props.text_color);

  // If empty, draw placeholder text with some opacity.
  if (m_style_edit.empty()) {
    float old_opacity = Renderer::get()->opacity();
    Renderer::get()->set_opacity(old_opacity *
                                 Skin::get()->default_placeholder_opacity());
    Rect placeholder_rect(visible_rect.x, visible_rect.y, visible_rect.w,
                          computed_font()->height());
    m_placeholder.Paint(this, placeholder_rect, paint_props.text_color);
    Renderer::get()->set_opacity(old_opacity);
  }
  Renderer::get()->Translate(-trans_x, -trans_y);

  if (clip) {
    Renderer::get()->set_clip_rect(old_clip, false);
  }
}

void TextBox::OnPaintChildren(const PaintProps& paint_props) {
  Element::OnPaintChildren(paint_props);

  // Draw fadeout skin at the needed edges.
  Skin::DrawEdgeFadeout(
      visible_rect(), TBIDC("TextBox.fadeout_x"), TBIDC("TextBox.fadeout_y"),
      m_scrollbar_x.value(), m_scrollbar_y.value(),
      int(m_scrollbar_x.max_value() - m_scrollbar_x.double_value()),
      int(m_scrollbar_y.max_value() - m_scrollbar_y.double_value()));
}

void TextBox::OnAdded() { m_style_edit.SetFont(computed_font_description()); }

void TextBox::OnFontChanged() {
  m_style_edit.SetFont(computed_font_description());
}

void TextBox::OnFocusChanged(bool focused) { m_style_edit.Focus(focused); }

void TextBox::OnResized(int old_w, int old_h) {
  // Make the scrollbars move.
  Element::OnResized(old_w, old_h);

  Rect visible_rect = this->visible_rect();
  m_style_edit.SetLayoutSize(visible_rect.w, visible_rect.h, false);

  UpdateScrollbars();
}

PreferredSize TextBox::OnCalculatePreferredContentSize(
    const SizeConstraints& constraints) {
  int font_height = computed_font()->height();
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
        if (auto bg_skin = background_skin_element()) {
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
  if (msg->message_id() == TBIDC("blink")) {
    m_style_edit.caret.on = !m_style_edit.caret.on;
    m_style_edit.caret.Invalidate();

    // Post another blink message so we blink again.
    PostMessageDelayed(TBIDC("blink"), nullptr, kCaretBlinkTimeMillis);
  } else if (msg->message_id() == TBIDC("selscroll") &&
             captured_element == this) {
    // Get scroll speed from where mouse is relative to the padding rect.
    Rect padding_rect = visible_rect().Shrink(2, 2);
    int dx = GetSelectionScrollSpeed(pointer_move_element_x, padding_rect.x,
                                     padding_rect.x + padding_rect.w);
    int dy = GetSelectionScrollSpeed(pointer_move_element_y, padding_rect.y,
                                     padding_rect.y + padding_rect.h);
    m_scrollbar_x.set_value(m_scrollbar_x.value() + dx);
    m_scrollbar_y.set_value(m_scrollbar_y.value() + dy);

    // Handle mouse move at the new scroll position, so selection is updated
    if (dx || dy) {
      m_style_edit.MouseMove(
          Point(pointer_move_element_x, pointer_move_element_y));
    }

    // Post another setscroll message so we continue scrolling if we still
    // should.
    if (m_style_edit.select_state) {
      PostMessageDelayed(TBIDC("selscroll"), nullptr,
                         kSelectionScrollDelayMillis);
    }
  }
}

void TextBox::OnChange() {
  // Invalidate the layout when the content change and we should adapt our size
  // to it.
  if (m_adapt_to_content_size) {
    InvalidateLayout(InvalidationMode::kRecursive);
  }

  Event ev(EventType::kChanged);
  InvokeEvent(ev);
}

bool TextBox::OnEnter() { return false; }

void TextBox::Invalidate(const Rect& rect) { Element::Invalidate(); }

void TextBox::DrawString(int32_t x, int32_t y, text::FontFace* font,
                         const Color& color, const char* str, size_t len) {
  font->DrawString(x, y, color, str, len);
}

void TextBox::DrawRect(const Rect& rect, const Color& color) {
  Renderer::get()->DrawRect(rect, color);
}

void TextBox::DrawRectFill(const Rect& rect, const Color& color) {
  Renderer::get()->DrawRectFill(rect, color);
}

void TextBox::DrawTextSelectionBg(const Rect& rect) {
  ElementSkinConditionContext context(this);
  Skin::get()->PaintSkin(rect, TBIDC("TextBox.selection"),
                         static_cast<Element::State>(computed_state()),
                         context);
}

void TextBox::DrawContentSelectionFg(const Rect& rect) {
  ElementSkinConditionContext context(this);
  Skin::get()->PaintSkin(rect, TBIDC("TextBox.selection"),
                         static_cast<Element::State>(computed_state()),
                         context);
}

void TextBox::DrawCaret(const Rect& rect) {
  if (is_focused() && !m_style_edit.packed.read_only) {
    DrawTextSelectionBg(rect);
  }
}

void TextBox::Scroll(int32_t dx, int32_t dy) {
  Element::Invalidate();
  m_scrollbar_x.set_value(m_style_edit.scroll_x);
  m_scrollbar_y.set_value(m_style_edit.scroll_y);
}

void TextBox::UpdateScrollbars() {
  int32_t w = m_style_edit.layout_width;
  int32_t h = m_style_edit.layout_height;
  m_scrollbar_x.set_limits(0, m_style_edit.GetContentWidth() - w, w);
  m_scrollbar_y.set_limits(0, m_style_edit.GetContentHeight() - h, h);
}

void TextBox::CaretBlinkStart() {
  // Post the delayed blink message if we don't already have one
  if (!GetMessageById(TBIDC("blink"))) {
    PostMessageDelayed(TBIDC("blink"), nullptr, kCaretBlinkTimeMillis);
  }
}

void TextBox::CaretBlinkStop() {
  // Remove the blink message if we have one
  if (Message* msg = GetMessageById(TBIDC("blink"))) {
    DeleteMessage(msg);
  }
}

void TextBox::TextBoxScrollRoot::OnPaintChildren(
    const PaintProps& paint_props) {
  // Avoid setting clipping (can be expensive) if we have no children to paint
  // anyway.
  if (!first_child()) return;
  // Clip children.
  Rect old_clip_rect = Renderer::get()->set_clip_rect(padding_rect(), true);
  Element::OnPaintChildren(paint_props);
  Renderer::get()->set_clip_rect(old_clip_rect, false);
}

void TextBox::TextBoxScrollRoot::GetChildTranslation(int& x, int& y) const {
  TextBox* edit_field = static_cast<TextBox*>(parent());
  x = -edit_field->text_view()->scroll_x;
  y = -edit_field->text_view()->scroll_y;
}

HitStatus TextBox::TextBoxScrollRoot::GetHitStatus(int x, int y) {
  // Return no hit on this element, but maybe on any of the children.
  if (Element::GetHitStatus(x, y) != HitStatus::kNoHit &&
      GetElementAt(x, y, false)) {
    return HitStatus::kHit;
  }
  return HitStatus::kNoHit;
}

class TextFragmentContentElement : public text::TextFragmentContent {
 public:
  TextFragmentContentElement(Element* parent, Element* element);
  ~TextFragmentContentElement() override;

  void UpdatePos(int x, int y) override;
  int32_t GetWidth(text::FontFace* font, text::TextFragment* fragment) override;
  int32_t GetHeight(text::FontFace* font,
                    text::TextFragment* fragment) override;
  int32_t GetBaseline(text::FontFace* font,
                      text::TextFragment* fragment) override;

 private:
  Element* m_element;
};

TextFragmentContentElement::TextFragmentContentElement(Element* parent,
                                                       Element* element)
    : m_element(element) {
  parent->content_root()->AddChild(element);
}

TextFragmentContentElement::~TextFragmentContentElement() {
  m_element->parent()->DeleteChild(m_element);
}

void TextFragmentContentElement::UpdatePos(int x, int y) {
  m_element->set_rect(
      {x, y, GetWidth(nullptr, nullptr), GetHeight(nullptr, nullptr)});
}

int32_t TextFragmentContentElement::GetWidth(text::FontFace* font,
                                             text::TextFragment* fragment) {
  return m_element->rect().w ? m_element->rect().w
                             : m_element->GetPreferredSize().pref_w;
}

int32_t TextFragmentContentElement::GetHeight(text::FontFace* font,
                                              text::TextFragment* fragment) {
  return m_element->rect().h ? m_element->rect().h
                             : m_element->GetPreferredSize().pref_h;
}

int32_t TextFragmentContentElement::GetBaseline(text::FontFace* font,
                                                text::TextFragment* fragment) {
  int height = GetHeight(font, fragment);
  return (height + fragment->block->CalculateBaseline(font)) / 2;
}

int TextBox::TextBoxContentFactory::GetContent(const char* text) {
  return TextFragmentContentFactory::GetContent(text);
}

text::TextFragmentContent*
TextBox::TextBoxContentFactory::CreateFragmentContent(const char* text,
                                                      size_t text_len) {
  if (strncmp(text, "<element ", std::min(text_len, 8ull)) == 0) {
    // Create a wrapper for the generated element.
    // Its size will adapt to the content.
    auto element = new Element();
    auto cw = new TextFragmentContentElement(text_box, element);
    element->LoadData(text + 8, text_len - 9);
    return cw;
  }

  return TextFragmentContentFactory::CreateFragmentContent(text, text_len);
}

}  // namespace elements
}  // namespace tb
