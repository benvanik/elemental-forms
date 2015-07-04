/**
******************************************************************************
* xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
******************************************************************************
* Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
* See tb_core.h and LICENSE in the root for more information.                *
******************************************************************************
*/

#include <cstdio>

#include "tb/util/debug.h"
#include "tb/util/string.h"

#include "tb_font_renderer.h"
#include "tb_text_box.h"
#include "tb_widgets_reader.h"
#include "tb_window.h"

#ifdef TB_RUNTIME_DEBUG_INFO

namespace tb {
namespace util {

DebugInfo g_tb_debug;

DebugInfo::DebugInfo() = default;

// Window showing runtime debug settings.
class DebugSettingsWindow : public Window, public ElementListener {
 public:
  TBOBJECT_SUBCLASS(DebugSettingsWindow, Window);

  TextBox* output;

  DebugSettingsWindow(Element* root) {
    SetText("Debug settings");
    g_elements_reader->LoadData(
        this,
        "Layout: axis: y, distribution: available, position: left\n"
        "	Layout: id: 'container', axis: y, size: available\n"
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

    output = GetElementByIDAndType<TextBox>(TBIDC("output"));

    Rect bounds(0, 0, root->rect().w, root->rect().h);
    set_rect(GetResizeToFitContentRect().CenterIn(bounds).MoveIn(bounds).Clip(
        bounds));

    root->AddChild(this);

    ElementListener::AddGlobalListener(this);
  }

  ~DebugSettingsWindow() { ElementListener::RemoveGlobalListener(this); }

  void AddCheckbox(DebugInfo::Setting setting, const char* str) {
    CheckBox* check = new CheckBox();
    check->SetValue(g_tb_debug.settings[int(setting)]);
    check->data.SetInt(int(setting));
    check->SetID(TBIDC("check"));

    LabelContainer* label = new LabelContainer();
    label->SetText(str);
    label->GetContentRoot()->AddChild(check, ElementZ::kBottom);

    GetElementByID(TBIDC("container"))->AddChild(label);
  }

  bool OnEvent(const ElementEvent& ev) override {
    if (ev.type == EventType::kClick && ev.target->GetID() == TBIDC("check")) {
      // Update setting and invalidate
      g_tb_debug.settings[ev.target->data.GetInt()] = ev.target->GetValue();
      GetParentRoot()->Invalidate();
      return true;
    }
    return Window::OnEvent(ev);
  }

  void OnPaint(const PaintProps& paint_props) override {
    // Draw stuff to the right of the debug window.
    g_renderer->Translate(rect().w, 0);

    // Draw skin bitmap fragments.
    if (TB_DEBUG_SETTING(util::DebugInfo::Setting::kDrawSkinBitmapFragments)) {
      g_tb_skin->Debug();
    }

    // Draw font glyph fragments (the font of the hovered element).
    if (TB_DEBUG_SETTING(util::DebugInfo::Setting::kDrawFontBitmapFragments)) {
      Element* element = Element::hovered_element ? Element::hovered_element
                                                  : Element::focused_element;
      auto font_face = g_font_manager->GetFontFace(
          element ? element->GetCalculatedFontDescription()
                  : g_font_manager->GetDefaultFontDescription());
      font_face->Debug();
    }

    g_renderer->Translate(-rect().w, 0);
  }

  std::string GetIDString(const TBID& id) {
    std::string str;
#ifdef TB_RUNTIME_DEBUG_INFO
    str = "\"" + id.debug_string + "\"";
#else
    str = format_string("%u", uint32_t(id));
#endif  // TB_RUNTIME_DEBUG_INFO
    return str;
  }

  bool OnElementInvokeEvent(Element* element, const ElementEvent& ev) override {
    // Skip these events for now.
    if (ev.IsPointerEvent()) {
      return false;
    }

    // Always ignore activity in this window (or we might get endless
    // recursion).
    if (Window* window = element->GetParentWindow()) {
      if (TBSafeCast<DebugSettingsWindow>(window)) {
        return false;
      }
    }

    StringBuilder buf;
    buf.AppendString(to_string(ev.type));
    buf.AppendString(" (");
    buf.AppendString(element->GetClassName());
    buf.AppendString(")");

    buf.AppendString(" id: ");
    buf.AppendString(GetIDString(ev.target->GetID()));

    if (ev.ref_id) {
      buf.AppendString(", ref_id: ");
      buf.AppendString(GetIDString(ev.ref_id));
    }

    if (ev.type == EventType::kChanged) {
      auto text = ev.target->GetText();
      if (text.size() > 24) {
        text.erase(20);
        text.append("...");
      }
      auto extra = util::format_string(
          ", value: %.2f (\"%s\")", ev.target->GetValueDouble(), text.c_str());
      buf.AppendString(extra);
    }
    buf.AppendString("\n");

    // Append the line to the output textfield.
    StyleEdit* se = output->GetStyleEdit();
    se->selection.SelectNothing();
    se->AppendText(buf.GetData(), std::string::npos, true);
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
}  // namespace tb

#endif  // TB_RUNTIME_DEBUG_INFO
