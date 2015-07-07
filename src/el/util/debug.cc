/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include <cstdio>

#include "el/elements/check_box.h"
#include "el/elements/label_container.h"
#include "el/elements/text_box.h"
#include "el/graphics/image_manager.h"
#include "el/text/font_manager.h"
#include "el/util/debug.h"
#include "el/util/string.h"
#include "el/window.h"

#ifdef EL_RUNTIME_DEBUG_INFO

namespace el {
namespace util {

DebugInfo DebugInfo::debug_info_singleton_;

DebugInfo::DebugInfo() = default;

// Window showing runtime debug settings.
class DebugSettingsWindow : public Window, public ElementListener {
 public:
  TBOBJECT_SUBCLASS(DebugSettingsWindow, Window);

  elements::TextBox* output;

  DebugSettingsWindow(Element* root) {
    set_text("Debug settings");
    LoadData(
        "LayoutBox: axis: y, distribution: available, position: left\n"
        "	LayoutBox: id: 'container', axis: y, size: available\n"
        "	Label: text: 'Event output:'\n"
        "	TextBox: id: 'output', gravity: all, multiline: 1, wrap: "
        "0\n"
        "		lp: pref-height: 100dp");

    AddCheckbox(DebugInfo::Setting::kLayoutBounds, "Layout bounds");
    AddCheckbox(DebugInfo::Setting::kLayoutClipping, "Layout clipping");
    AddCheckbox(DebugInfo::Setting::kLayoutSizing, "Layout size calculation");
    AddCheckbox(DebugInfo::Setting::kDrawRenderBatches, "Render batches");
    AddCheckbox(DebugInfo::Setting::kDrawSkinBitmapFragments,
                "Render skin bitmap fragments");
    AddCheckbox(DebugInfo::Setting::kDrawFontBitmapFragments,
                "Render font bitmap fragments");
    AddCheckbox(DebugInfo::Setting::kDrawImageBitmapFragments,
                "Render image bitmap fragments");

    output = GetElementById<elements::TextBox>(TBIDC("output"));

    Rect bounds(0, 0, root->rect().w, root->rect().h);
    set_rect(GetResizeToFitContentRect().CenterIn(bounds).MoveIn(bounds).Clip(
        bounds));

    root->AddChild(this);

    ElementListener::AddGlobalListener(this);
  }

  ~DebugSettingsWindow() { ElementListener::RemoveGlobalListener(this); }

  void AddCheckbox(DebugInfo::Setting setting, const char* str) {
    auto check = new elements::CheckBox();
    check->set_value(DebugInfo::get()->settings[int(setting)]);
    check->data.set_integer(int(setting));
    check->set_id(TBIDC("check"));

    auto label = new elements::LabelContainer();
    label->set_text(str);
    label->content_root()->AddChild(check, ElementZ::kBottom);

    GetElementById<Element>(TBIDC("container"))->AddChild(label);
  }

  bool OnEvent(const Event& ev) override {
    if (ev.type == EventType::kClick && ev.target->id() == TBIDC("check")) {
      // Update setting and invalidate.
      DebugInfo::get()->settings[ev.target->data.as_integer()] =
          ev.target->value();
      parent_root()->Invalidate();
      return true;
    }
    return Window::OnEvent(ev);
  }

  void OnPaint(const PaintProps& paint_props) override {
    // Draw stuff to the right of the debug window.
    graphics::Renderer::get()->Translate(rect().w, 0);

    // Draw skin bitmap fragments.
    if (EL_DEBUG_SETTING(util::DebugInfo::Setting::kDrawSkinBitmapFragments)) {
      Skin::get()->Debug();
    }

    // Draw font glyph fragments (the font of the hovered element).
    if (EL_DEBUG_SETTING(util::DebugInfo::Setting::kDrawFontBitmapFragments)) {
      Element* element = Element::hovered_element ? Element::hovered_element
                                                  : Element::focused_element;
      auto font_face = text::FontManager::get()->GetFontFace(
          element ? element->computed_font_description()
                  : text::FontManager::get()->default_font_description());
      font_face->Debug();
    }

    // Draw image manager fragments.
    if (EL_DEBUG_SETTING(util::DebugInfo::Setting::kDrawImageBitmapFragments)) {
      graphics::ImageManager::get()->Debug();
    }

    graphics::Renderer::get()->Translate(-rect().w, 0);
  }

  std::string GetIDString(const TBID& id) {
    std::string str;
#ifdef EL_RUNTIME_DEBUG_INFO
    str = "\"" + id.debug_string + "\"";
#else
    str = format_string("%u", uint32_t(id));
#endif  // EL_RUNTIME_DEBUG_INFO
    return str;
  }

  bool OnElementInvokeEvent(Element* element, const Event& ev) override {
    // Skip these events for now.
    if (ev.is_pointer_event()) {
      return false;
    }

    // Always ignore activity in this window (or we might get endless
    // recursion).
    if (Window* window = element->parent_window()) {
      if (SafeCast<DebugSettingsWindow>(window)) {
        return false;
      }
    }

    StringBuilder buf;
    buf.AppendString(to_string(ev.type));
    buf.AppendString(" (");
    buf.AppendString(element->GetTypeName());
    buf.AppendString(")");

    buf.AppendString(" id: ");
    buf.AppendString(GetIDString(ev.target->id()));

    if (ev.ref_id) {
      buf.AppendString(", ref_id: ");
      buf.AppendString(GetIDString(ev.ref_id));
    }

    if (ev.type == EventType::kChanged) {
      auto text = ev.target->text();
      if (text.size() > 24) {
        text.erase(20);
        text.append("...");
      }
      auto extra = util::format_string(", value: %.2f (\"%s\")",
                                       ev.target->double_value(), text.c_str());
      buf.AppendString(extra);
    }
    buf.AppendString("\n");

    // Append the line to the output textfield.
    auto se = output->text_view();
    se->selection.SelectNothing();
    se->AppendText(buf.c_str(), std::string::npos, true);
    se->ScrollIfNeeded(false, true);

    // Remove lines from the top if we exceed the height limit.
    const int height_limit = 2000;
    int current_height = se->GetContentHeight();
    if (current_height > height_limit) {
      se->caret.Place(Point(0, current_height - height_limit));
      se->selection.SelectToCaret(se->blocks.GetFirst(), 0);
      se->Delete();
    }
    return false;
  }
};

void ShowDebugInfoSettingsWindow(Element* root) {
  new DebugSettingsWindow(root);
}

}  // namespace util
}  // namespace el

#endif  // EL_RUNTIME_DEBUG_INFO
